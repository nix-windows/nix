#include "buildenv.hh"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <algorithm>

namespace nix {

struct State
{
    std::map<Path, int> priorities;
    unsigned long symlinks = 0;
};

/* For each activated package, create symlinks */
// TODO: make Windows native version
static void createLinks(State & state, const Path & srcDir, const Path & dstDir, int priority)
{
    DirEntries srcFiles;

    try {
        srcFiles = readDirectory(srcDir);
    } catch (PosixError & e) {
        if (e.errNo == ENOTDIR) {
            logWarning({
                .name = "Create links - directory",
                .hint = hintfmt("not including '%s' in the user environment because it's not a directory", srcDir)
            });
            return;
        }
        throw;
#ifdef _WIN32
    } catch (WinError & e) {
        throw e; // TODO
#endif
    }

    for (const auto & ent : srcFiles) {
        auto name = ent.name();
        if (name[0] == '.')
            /* not matched by glob */
            continue;
        auto srcFile = srcDir + "/" + name;
        auto dstFile = dstDir + "/" + name;
#ifndef _WIN32
        struct stat srcSt;
        try {
            if (stat(srcFile.c_str(), &srcSt) == -1)
                throw PosixError("getting status-4 of '%1%'", srcFile);
        } catch (PosixError & e) {
            if (e.errNo == ENOENT || e.errNo == ENOTDIR) {
                logWarning({
                    .name = "Create links - skipping symlink",
                    .hint = hintfmt("skipping dangling symlink '%s'", dstFile)
                });
                continue;
            }
            throw;
        }
#else
        WIN32_FILE_ATTRIBUTE_DATA wfad;
        if (!GetFileAttributesExW(pathW(srcFile).c_str(), GetFileExInfoStandard, &wfad)) {
            WinError winError("GetFileAttributesExW when createLinks '%1%'", srcFile);
            if (winError.lastError == ERROR_FILE_NOT_FOUND || winError.lastError == ERROR_PATH_NOT_FOUND) {
                printError("warning: skipping dangling symlink '%s'", dstFile);
                continue;
            }
            throw winError;
        }
#endif

        /* The files below are special-cased to that they don't show up
         * in user profiles, either because they are useless, or
         * because they would cauase pointless collisions (e.g., each
         * Python package brings its own
         * `$out/lib/pythonX.Y/site-packages/easy-install.pth'.)
         */
        if (hasSuffix(srcFile, "/propagated-build-inputs") ||
            hasSuffix(srcFile, "/nix-support") ||
            hasSuffix(srcFile, "/perllocal.pod") ||
            hasSuffix(srcFile, "/info/dir") ||
            hasSuffix(srcFile, "/log"))
            continue;

#ifndef _WIN32
        else if (S_ISDIR(srcSt.st_mode)) {
#else
        else if ((wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
#endif
            unsigned char dt = getFileType(dstFile);

            if (dt != DT_UNKNOWN) {
                if (dt == DT_DIR) {
                    createLinks(state, srcFile, dstFile, priority);
                    continue;
                } else if (dt == DT_LNK) {
                    auto target = canonPath(dstFile, true);
                    if (getFileType(target) != DT_DIR)
                        throw Error("collision between '%1%' and non-directory '%2%'", srcFile, target);
#ifndef _WIN32
                    if (unlink(dstFile.c_str()) == -1)
                        throw PosixError("unlinking '%1%'", dstFile);
                    if (mkdir(dstFile.c_str(), 0755) == -1)
                        throw PosixError("creating directory '%1%'", dstFile);
#else
                    if (!DeleteFileW(pathW(dstFile).c_str()))
                        throw WinError("DeleteFileW when createLinks '%1%'", dstFile);
                    if (!CreateDirectoryW(pathW(dstFile).c_str(), NULL))
                        throw WinError("CreateDirectoryW when createLinks '%1%'", dstFile);
#endif
                    createLinks(state, target, dstFile, state.priorities[dstFile]);
                    createLinks(state, srcFile, dstFile, priority);
                    continue;
                }
            } //else if (errno != ENOENT)
                //throw PosixError("getting status-5 of '%1%'", dstFile);
        }

        else {
            unsigned char dt = getFileType(dstFile);
            if (dt != DT_UNKNOWN) {
                if (dt == DT_LNK) {
                    auto prevPriority = state.priorities[dstFile];
                    if (prevPriority == priority)
                        throw Error(
                                "packages '%1%' and '%2%' have the same priority %3%; "
                                "use 'nix-env --set-flag priority NUMBER INSTALLED_PKGNAME' "
                                "to change the priority of one of the conflicting packages"
                                " (0 being the highest priority)",
                                srcFile, readLink(dstFile), priority);
                    if (prevPriority < priority)
                        continue;
#ifndef _WIN32
                    if (unlink(dstFile.c_str()) == -1)
                        throw PosixError("unlinking '%1%'", dstFile);
#else
                    if (!DeleteFileW(pathW(dstFile).c_str()))
                        throw WinError("DeleteFileW when createLinks '%1%'", dstFile);
#endif
                } else
                if (dt == DT_DIR)
                    throw Error("collision between non-directory '%1%' and directory '%2%'", srcFile, dstFile);
            } //else if (errno != ENOENT)
                //throw PosixError("getting status-6 of '%1%'", dstFile);
        }

        createSymlink(srcFile, dstFile);
        state.priorities[dstFile] = priority;
        state.symlinks++;
    }
}

void buildProfile(const Path & out, Packages && pkgs)
{
    State state;

    std::set<Path> done, postponed;

    auto addPkg = [&](const Path & pkgDir, int priority) {
        if (!done.insert(pkgDir).second) return;
        createLinks(state, pkgDir, out, priority);

        try {
            for (const auto & p : tokenizeString<std::vector<string>>(
                    readFile(pkgDir + "/nix-support/propagated-user-env-packages"), " \n"))
                if (!done.count(p))
                    postponed.insert(p);
        } catch (PosixError & e) {
            if (e.errNo != ENOENT && e.errNo != ENOTDIR) throw;
#ifdef _WIN32
        } catch (WinError & e) {
            throw e; // TODO
#endif
        }
    };

    /* Symlink to the packages that have been installed explicitly by the
     * user. Process in priority order to reduce unnecessary
     * symlink/unlink steps.
     */
    std::sort(pkgs.begin(), pkgs.end(), [](const Package & a, const Package & b) {
        return a.priority < b.priority || (a.priority == b.priority && a.path < b.path);
    });
    for (const auto & pkg : pkgs)
        if (pkg.active)
            addPkg(pkg.path, pkg.priority);

    /* Symlink to the packages that have been "propagated" by packages
     * installed by the user (i.e., package X declares that it wants Y
     * installed as well). We do these later because they have a lower
     * priority in case of collisions.
     */
    auto priorityCounter = 1000;
    while (!postponed.empty()) {
        std::set<Path> pkgDirs;
        postponed.swap(pkgDirs);
        for (const auto & pkgDir : pkgDirs)
            addPkg(pkgDir, priorityCounter++);
    }

    debug("created %d symlinks in user environment", state.symlinks);
}

void builtinBuildenv(const BasicDerivation & drv)
{
    auto getAttr = [&](const string & name) {
        auto i = drv.env.find(name);
        if (i == drv.env.end()) throw Error("attribute '%s' missing", name);
        return i->second;
    };

    Path out = getAttr("out");
    createDirs(out);

    /* Convert the stuff we get from the environment back into a
     * coherent data type. */
    Packages pkgs;
    auto derivations = tokenizeString<Strings>(getAttr("derivations"));
    while (!derivations.empty()) {
        /* !!! We're trusting the caller to structure derivations env var correctly */
        auto active = derivations.front(); derivations.pop_front();
        auto priority = stoi(derivations.front()); derivations.pop_front();
        auto outputs = stoi(derivations.front()); derivations.pop_front();
        for (auto n = 0; n < outputs; n++) {
            auto path = derivations.front(); derivations.pop_front();
            pkgs.emplace_back(path, active != "false", priority);
        }
    }

    buildProfile(out, std::move(pkgs));

    createSymlink(getAttr("manifest"), out + "/manifest.nix");
}

}

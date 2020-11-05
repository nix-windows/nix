#include "derivations.hh"
#include "globals.hh"
#include "local-store.hh"
#include "local-fs-store.hh"
#include "finally.hh"

#include <functional>
#include <queue>
#include <algorithm>
#include <regex>
#include <random>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_STATVFS
#include <sys/statvfs.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <climits>

#ifdef _WIN32
#include <iostream>
#endif

namespace nix {


static string gcLockName = "gc.lock";
static string gcRootsDir = "gcroots";


/* Acquire the global GC lock.  This is used to prevent new Nix
   processes from starting after the temporary root files have been
   read.  To be precise: when they try to create a new temporary root
   file, they will block until the garbage collector has finished /
   yielded the GC lock. */
#ifndef _WIN32
AutoCloseFD
#else
AutoCloseWindowsHandle
#endif
LocalStore::openGCLock(LockType lockType)
{
    Path fnGCLock = (format("%1%/%2%")
        % stateDir % gcLockName).str();

    debug(format("acquiring global GC lock '%1%' '%2%'") % fnGCLock % lockType);

#ifndef _WIN32
    AutoCloseFD fdGCLock = open(fnGCLock.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (!fdGCLock)
        throw PosixError("opening global GC lock '%1%'", fnGCLock);
#else
    AutoCloseWindowsHandle fdGCLock = CreateFileW(pathW(fnGCLock).c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS, NULL);
    if (!fdGCLock)
        throw WinError("opening global GC lock '%1%'", fnGCLock);
#endif

    if (!lockFile(fdGCLock.get(), lockType, false)) {
        printInfo("waiting for the big garbage collector lock...");
        lockFile(fdGCLock.get(), lockType, true);
    }

    /* !!! Restrict read permission on the GC root.  Otherwise any
       process that can open the file for reading can DoS the
       collector. */

    return fdGCLock;
}


static void makeSymlink(const Path & link, const Path & target)
{
    /* Create directories up to `gcRoot'. */
    createDirs(dirOf(link));

#ifndef _WIN32
    /* Create the new symlink. */
    Path tempLink = (format("%1%.tmp-%2%-%3%")
        % link % getpid() % random()).str();
    createSymlink(target, tempLink);

    /* Atomically replace the old one. */
    if (rename(tempLink.c_str(), link.c_str()) == -1)
        throw PosixError("cannot rename '%1%' to '%2%'",
            tempLink , link);
#else
//std::cerr << "MoveFileExW '"<<tempLink<<"' -> '"<<link<<"'"<<std::endl;
    Path tempLink = (format("%1%.tmp~%2%~%3%")
        % link % GetCurrentProcessId() % rand()).str();

    SymlinkType st = createSymlink(target, tempLink);

    std::wstring wtempLink = pathW(absPath(tempLink));
    std::wstring wlink     = pathW(absPath(link)); // already absolute?
    if (!MoveFileExW(wtempLink.c_str(), wlink.c_str(), MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        // so try once more harder (atomicity suffers here)
        std::wstring old = pathW(absPath((format("%1%.old~%2%~%3%") % link % GetCurrentProcessId() % rand()).str()));
        if (!MoveFileExW(wlink.c_str(), old.c_str(), MOVEFILE_WRITE_THROUGH))
            throw WinError("MoveFileExW '%1%' -> '%2%'", to_bytes(wlink), to_bytes(old));
        // repeat
        if (!MoveFileExW(wtempLink.c_str(), wlink.c_str(), MOVEFILE_WRITE_THROUGH))
            throw WinError("MoveFileExW '%1%' -> '%2%'", tempLink, to_bytes(wlink));

        DWORD dw = GetFileAttributesW(old.c_str());
        if (dw == 0xFFFFFFFF)
            throw WinError("GetFileAttributesW '%1%'", to_bytes(old));
        if ((dw & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            if (!RemoveDirectoryW(old.c_str()))
                printError(WinError("RemoveDirectoryW '%1%'", to_bytes(old)).msg());
        } else {
            if (!DeleteFileW(old.c_str()))
                printError(WinError("DeleteFileW '%1%'", to_bytes(old)).msg());
        }
    }
#endif
}


void LocalStore::syncWithGC()
{
#ifndef _WIN32
    AutoCloseFD fdGCLock = openGCLock(ltRead);
#else
    AutoCloseWindowsHandle fdGCLock = openGCLock(ltRead);
#endif
}


void LocalStore::addIndirectRoot(const Path & path)
{
    string hash = hashString(htSHA1, path).to_string(Base32, false);
    Path realRoot = canonPath((format("%1%/%2%/auto/%3%")
        % stateDir % gcRootsDir % hash).str());
    makeSymlink(realRoot, path);
}


Path LocalFSStore::addPermRoot(const StorePath & storePath, const Path & _gcRoot)
{
    Path gcRoot(canonPath(_gcRoot));

    if (isInStore(gcRoot))
        throw Error(
                "creating a garbage collector root (%1%) in the Nix store is forbidden "
                "(are you running nix-build inside the store?)", gcRoot);

    /* Don't clobber the link if it already exists and doesn't
       point to the Nix store. */
    if (pathExists(gcRoot) && (!isLink(gcRoot) || !isInStore(readLink(gcRoot))))
        throw Error("cannot create symlink '%1%'; already exists", gcRoot);
    makeSymlink(gcRoot, printStorePath(storePath));
    addIndirectRoot(gcRoot);

    /* Grab the global GC root, causing us to block while a GC is in
       progress.  This prevents the set of permanent roots from
       increasing while a GC is in progress. */
    syncWithGC();

    return gcRoot;
}


void LocalStore::addTempRoot(const StorePath & path)
{
    auto state(_state.lock());

    /* Create the temporary roots file for this process. */
    if (!state->fdTempRoots) {

        while (1) {
#ifndef _WIN32
            AutoCloseFD fdGCLock = openGCLock(ltRead);
#else
            AutoCloseWindowsHandle fdGCLock = openGCLock(ltRead);
#endif
            if (pathExists(fnTempRoots))
                /* It *must* be stale, since there can be no two
                   processes with the same pid. */
                unlink(fnTempRoots.c_str());

            state->fdTempRoots = openLockFile(fnTempRoots, true);
#ifndef _WIN32
            fdGCLock = -1;

            debug(format("acquiring read lock on '%1%'") % fnTempRoots);
            lockFile(state->fdTempRoots.get(), ltRead, true);
            /* Check whether the garbage collector didn't get in our
               way. */
            struct stat st;
            if (fstat(state->fdTempRoots.get(), &st) == -1)
                throw PosixError("statting '%1%'", fnTempRoots);
            if (st.st_size == 0) break;

            /* The garbage collector deleted this file before we could
               get a lock.  (It won't delete the file after we get a
               lock.)  Try again.

               It should not be the case on Windows because other process would fail deleting the file.
               Perhaps GCLock is not needed here too
             */
#else
            fdGCLock = INVALID_HANDLE_VALUE;

            debug(format("acquiring read lock on '%1%'") % fnTempRoots);
            assert(lockFile(state->fdTempRoots.get(), ltRead, true));
            break;
#endif
        }
    }

    /* Upgrade the lock to a write lock.  This will cause us to block
       if the garbage collector is holding our lock. */
    debug(format("acquiring write lock on '%1%'") % fnTempRoots);
    lockFile(state->fdTempRoots.get(), ltWrite, true);

    string s = printStorePath(path) + '\0';
    writeFull(state->fdTempRoots.get(), s);

    /* Downgrade to a read lock. */
    debug(format("downgrading to read lock on '%1%'") % fnTempRoots);
    lockFile(state->fdTempRoots.get(), ltRead, true);
}

static std::string censored = "{censored}";

#ifndef _WIN32
void LocalStore::findTempRoots(FDs & fds, Roots & tempRoots, bool censor)
{
    /* Read the `temproots' directory for per-process temporary root
       files. */
    for (auto & i : readDirectory(tempRootsDir)) {
        if (i.name()[0] == '.') {
            // Ignore hidden files. Some package managers (notably portage) create
            // those to keep the directory alive.
            continue;
        }
        Path path = tempRootsDir + "/" + i.name();

        pid_t pid = std::stoi(i.name());

        debug(format("reading temporary root file '%1%'") % path);
        FDPtr fd(new AutoCloseFD(open(path.c_str(), O_CLOEXEC | O_RDWR, 0666)));
        if (!*fd) {
            /* It's okay if the file has disappeared. */
            if (errno == ENOENT) continue;
            throw PosixError("opening temporary roots file '%1%'", path);
        }

        /* This should work, but doesn't, for some reason. */
        //FDPtr fd(new AutoCloseFD(openLockFile(path, false)));
        //if (*fd == -1) continue;

        /* Try to acquire a write lock without blocking.  This can
           only succeed if the owning process has died.  In that case
           we don't care about its temporary roots. */
        if (lockFile(fd->get(), ltWrite, false)) {
            printInfo("removing stale temporary roots file '%1%'", path);
            unlink(path.c_str());
            writeFull(fd->get(), "d");
            continue;
        }

        /* Acquire a read lock.  This will prevent the owning process
           from upgrading to a write lock, therefore it will block in
           addTempRoot(). */
        debug(format("waiting for read lock on '%1%'") % path);
        lockFile(fd->get(), ltRead, true);

        /* Read the entire file. */
        string contents = readFile(fd->get());

        /* Extract the roots. */
        string::size_type pos = 0, end;

        while ((end = contents.find((char) 0, pos)) != string::npos) {
            Path root(contents, pos, end - pos);
            debug("got temporary root '%s'", root);
            tempRoots[parseStorePath(root)].emplace(censor ? censored : fmt("{temp:%d}", pid));
            pos = end + 1;
        }

        fds.push_back(fd); /* keep open */
    }
}
#endif


void LocalStore::findRoots(const Path & path, unsigned char type, Roots & roots)
{
    auto foundRoot = [&](const Path & path, const Path & target) {
        try {
            auto storePath = toStorePath(target).first;
            if (isValidPath(storePath))
                roots[std::move(storePath)].emplace(path);
            else
                printInfo("skipping invalid root from '%1%' to '%2%'", path, target);
        } catch (BadStorePath &) { }
    };

    try {

        if (type == DT_UNKNOWN)
            type = getFileType(path);

        if (type == DT_DIR) {
            for (auto & i : readDirectory(path))
                findRoots(path + "/" + i.name(), i.type(), roots);
        }

        else if (type == DT_LNK) {
            Path target = readLink(path);
            if (isInStore(target))
                foundRoot(path, target);

            /* Handle indirect roots. */
            else {
                target = absPath(target, dirOf(path));
                if (!pathExists(target)) {
                    if (isInDir(path, stateDir + "/" + gcRootsDir + "/auto")) {
                        printInfo(format("removing stale link from '%1%' to '%2%'") % path % target);
#ifndef _WIN32
                        unlink(path.c_str());
#else // unlink() cannot delete symlink to directory
                        DWORD dw = GetFileAttributesW(pathW(path).c_str());
                        if (dw == 0xFFFFFFFF)
                            throw WinError("GetFileAttributesW '%1%'", path);
                        if ((dw & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                            if (!RemoveDirectoryW(pathW(path).c_str()))
                                printError(WinError("RemoveDirectoryW '%1%'", path).msg());
                        } else {
                            if (!DeleteFileW(pathW(path).c_str()))
                                printError(WinError("DeleteFileW '%1%'", path).msg());
                        }
#endif
                    }
                } else {
                    if (!isLink(target)) return;
                    Path target2 = readLink(target);
                    if (isInStore(target2)) foundRoot(target, target2);
                }
            }
        }

        else if (type == DT_REG) {
            auto storePath = maybeParseStorePath(storeDir + "/" + std::string(baseNameOf(path)));
            if (storePath && isValidPath(*storePath))
                roots[std::move(*storePath)].emplace(path);
        }

    }

    catch (PosixError & e) {
        /* We only ignore permanent failures. */
        if (e.errNo == EACCES || e.errNo == ENOENT || e.errNo == ENOTDIR)
            printInfo("cannot read potential root '%1%'", path);
        else
            throw;
#ifdef _WIN32
    } catch (WinError & e) {
        throw e; // TODO
#endif
    }
}


void LocalStore::findRootsNoTemp(Roots & roots, bool censor)
{
    /* Process direct roots in {gcroots,profiles}. */
    findRoots(stateDir + "/" + gcRootsDir, DT_UNKNOWN, roots);
    findRoots(stateDir + "/profiles", DT_UNKNOWN, roots);

    /* Add additional roots returned by different platforms-specific
       heuristics.  This is typically used to add running programs to
       the set of roots (to prevent them from being garbage collected). */
    findRuntimeRoots(roots, censor);
}


Roots LocalStore::findRoots(bool censor)
{
    Roots roots;
    findRootsNoTemp(roots, censor);

#ifndef _WIN32
    FDs fds;
    findTempRoots(fds, roots, censor);
#endif

    return roots;
}

typedef std::unordered_map<Path, std::unordered_set<std::string>> UncheckedRoots;

#ifndef _WIN32
static void readProcLink(const string & file, UncheckedRoots & roots)
{
    /* 64 is the starting buffer size gnu readlink uses... */
    auto bufsiz = ssize_t{64};
try_again:
    char buf[bufsiz];
    auto res = readlink(file.c_str(), buf, bufsiz);
    if (res == -1) {
        if (errno == ENOENT || errno == EACCES || errno == ESRCH)
            return;
        throw PosixError("reading symlink");
    }
    if (res == bufsiz) {
        if (SSIZE_MAX / 2 < bufsiz)
            throw Error("stupidly long symlink");
        bufsiz *= 2;
        goto try_again;
    }
    if (res > 0 && buf[0] == '/')
        roots[std::string(static_cast<char *>(buf), res)]
            .emplace(file);
}
#endif

static string quoteRegexChars(const string & raw)
{
    static auto specialRegex = std::regex(R"([.^$\\*+?()\[\]{}|])");
    return std::regex_replace(raw, specialRegex, R"(\$&)");
}

#ifndef _WIN32
static void readFileRoots(const char * path, UncheckedRoots & roots)
{
    try {
        roots[readFile(path)].emplace(path);
    } catch (PosixError & e) {
        if (e.errNo != ENOENT && e.errNo != EACCES)
            throw;
    }
}
#endif

void LocalStore::findRuntimeRoots(Roots & roots, bool censor)
{
    UncheckedRoots unchecked;
#ifndef _WIN32

    auto procDir = AutoCloseDir{opendir("/proc")};
    if (procDir) {
        struct dirent * ent;
        auto digitsRegex = std::regex(R"(^\d+$)");
        auto mapRegex = std::regex(R"(^\s*\S+\s+\S+\s+\S+\s+\S+\s+\S+\s+(/\S+)\s*$)");
        auto storePathRegex = std::regex(quoteRegexChars(storeDir) + R"(/[0-9a-z]+[0-9a-zA-Z\+\-\._\?=]*)");
        while (errno = 0, ent = readdir(procDir.get())) {
            checkInterrupt();
            if (std::regex_match(ent->d_name, digitsRegex)) {
                readProcLink(fmt("/proc/%s/exe" ,ent->d_name), unchecked);
                readProcLink(fmt("/proc/%s/cwd", ent->d_name), unchecked);

                auto fdStr = fmt("/proc/%s/fd", ent->d_name);
                auto fdDir = AutoCloseDir(opendir(fdStr.c_str()));
                if (!fdDir) {
                    if (errno == ENOENT || errno == EACCES)
                        continue;
                    throw PosixError("opening %1%", fdStr);
                }
                struct dirent * fd_ent;
                while (errno = 0, fd_ent = readdir(fdDir.get())) {
                    if (fd_ent->d_name[0] != '.')
                        readProcLink(fmt("%s/%s", fdStr, fd_ent->d_name), unchecked);
                }
                if (errno) {
                    if (errno == ESRCH)
                        continue;
                    throw PosixError("iterating /proc/%1%/fd", ent->d_name);
                }
                fdDir.reset();

                try {
                    auto mapFile = fmt("/proc/%s/maps", ent->d_name);
                    auto mapLines = tokenizeString<std::vector<string>>(readFile(mapFile), "\n");
                    for (const auto & line : mapLines) {
                        auto match = std::smatch{};
                        if (std::regex_match(line, match, mapRegex))
                            unchecked[match[1]].emplace(mapFile);
                    }

                    auto envFile = fmt("/proc/%s/environ", ent->d_name);
                    auto envString = readFile(envFile);
                    auto env_end = std::sregex_iterator{};
                    for (auto i = std::sregex_iterator{envString.begin(), envString.end(), storePathRegex}; i != env_end; ++i)
                        unchecked[i->str()].emplace(envFile);
                } catch (SysError & e) {
                    if (errno == ENOENT || errno == EACCES || errno == ESRCH)
                        continue;
                    throw;
                }
            }
        }
        if (errno)
            throw PosixError("iterating /proc");
    }

#if !defined(__linux__)
    // lsof is really slow on OS X. This actually causes the gc-concurrent.sh test to fail.
    // See: https://github.com/NixOS/nix/issues/3011
    // Because of this we disable lsof when running the tests.
    if (getEnv("_NIX_TEST_NO_LSOF") != "1") {
        try {
            std::regex lsofRegex(R"(^n(/.*)$)");
            auto lsofLines =
                tokenizeString<std::vector<string>>(runProgramGetStdout(LSOF, true, { "-n", "-w", "-F", "n" }), "\n");
            for (const auto & line : lsofLines) {
                std::smatch match;
                if (std::regex_match(line, match, lsofRegex))
                    unchecked[match[1]].emplace("{lsof}");
            }
        } catch (ExecError & e) {
            /* lsof not installed, lsof failed */
        }
    }
#endif

#if defined(__linux__)
    readFileRoots("/proc/sys/kernel/modprobe", unchecked);
    readFileRoots("/proc/sys/kernel/fbsplash", unchecked);
    readFileRoots("/proc/sys/kernel/poweroff_cmd", unchecked);
#endif

    for (auto & [target, links] : unchecked) {
        if (!isInStore(target)) continue;
        try {
            auto path = toStorePath(target).first;
            if (!isValidPath(path)) continue;
            debug("got additional root '%1%'", printStorePath(path));
            if (censor)
                roots[path].insert(censored);
            else
                roots[path].insert(links.begin(), links.end());
        } catch (BadStorePath &) { }
    }
#endif
}


struct GCLimitReached { };


struct LocalStore::GCState
{
    const GCOptions & options;
    GCResults & results;
    StorePathSet roots;
    StorePathSet tempRoots;
    StorePathSet dead;
    StorePathSet alive;
    bool gcKeepOutputs;
    bool gcKeepDerivations;
    uint64_t bytesInvalidated;
    bool moveToTrash = true;
    bool shouldDelete;
    GCState(const GCOptions & options, GCResults & results)
        : options(options), results(results), bytesInvalidated(0) { }
};


bool LocalStore::isActiveTempFile(const GCState & state,
    const Path & path, const string & suffix)
{
    return hasSuffix(path, suffix)
        && state.tempRoots.count(parseStorePath(string(path, 0, path.size() - suffix.size())));
}


void LocalStore::deleteGarbage(GCState & state, const Path & path)
{
    uint64_t bytesFreed;
    deletePath(path, bytesFreed);
    state.results.bytesFreed += bytesFreed;
}

void LocalStore::deletePathRecursive(GCState & state, const Path & path)
{
    checkInterrupt();

    uint64_t size = 0;

    auto storePath = maybeParseStorePath(path);
    if (storePath && isValidPath(*storePath)) {
        StorePathSet referrers;
        queryReferrers(*storePath, referrers);
        for (auto & i : referrers)
            if (printStorePath(i) != path) deletePathRecursive(state, printStorePath(i));
        size = queryPathInfo(*storePath)->narSize;
        invalidatePathChecked(*storePath);
    }

    Path realPath = realStoreDir + "/" + std::string(baseNameOf(path));

#ifndef _WIN32
    struct stat st;
    if (lstat(realPath.c_str(), &st)) {
        if (errno == ENOENT) return;
        throw PosixError("getting status of %1%", realPath);
    }
#else
    WIN32_FILE_ATTRIBUTE_DATA wfad;
    if (!GetFileAttributesExW(pathW(realPath).c_str(), GetFileExInfoStandard, &wfad)) {
        WinError winError("GetFileAttributesExW when deletePathRecursive '%1%'", realPath);
        if (winError.lastError == ERROR_FILE_NOT_FOUND)
            return;
        throw winError;
    }
#endif

    printInfo(format("deleting '%1%'") % path);

    state.results.paths.insert(path);

    /* If the path is not a regular file or symlink, move it to the
       trash directory.  The move is to ensure that later (when we're
       not holding the global GC lock) we can delete the path without
       being afraid that the path has become alive again.  Otherwise
       delete it right away. */
#ifndef _WIN32
    if (state.moveToTrash && S_ISDIR(st.st_mode)) {
        // Estimate the amount freed using the narSize field.  FIXME:
        // if the path was not valid, need to determine the actual
        // size.
        try {
            if (chmod(realPath.c_str(), st.st_mode | S_IWUSR) == -1)
                throw PosixError("making '%1%' writable", realPath);
            Path tmp = trashDir + "/" + std::string(baseNameOf(path));
            if (rename(realPath.c_str(), tmp.c_str()))
                throw PosixError("unable to rename '%1%' to '%2%'", realPath, tmp);
            state.bytesInvalidated += size;
        } catch (PosixError & e) {
            if (e.errNo == ENOSPC) {
                printInfo(format("note: can't create move '%1%': %2%") % realPath % e.msg());
                deleteGarbage(state, realPath);
            }
        }
    } else
        deleteGarbage(state, realPath);
#else
#if 0
    runProgramWithStatus(RunOptions("icacls", { realPath, "/reset" /*, "/C", "/T", "/L"*/ }));
#endif

    if (state.moveToTrash && (wfad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        try {
            Path tmp = trashDir + "/" + baseNameOf(path);
            if (!MoveFileExW(pathW(realPath).c_str(), pathW(tmp).c_str(), MOVEFILE_WRITE_THROUGH))
                throw WinError(format("MoveFileExW('%1%', '%2%')") % realPath % tmp);
            state.bytesInvalidated += size;
        } catch (WinError & e) {
            throw e; // TODO
        }
    } else
        deleteGarbage(state, realPath);
#endif

    if (state.results.bytesFreed + state.bytesInvalidated > state.options.maxFreed) {
        printInfo(format("deleted or invalidated more than %1% bytes; stopping") % state.options.maxFreed);
        throw GCLimitReached();
    }
}


bool LocalStore::canReachRoot(GCState & state, StorePathSet & visited, const StorePath & path)
{
    if (visited.count(path)) return false;

    if (state.alive.count(path)) return true;

    if (state.dead.count(path)) return false;

    if (state.roots.count(path)) {
        debug("cannot delete '%1%' because it's a root", printStorePath(path));
        state.alive.insert(path);
        return true;
    }

    visited.insert(path);

    if (!isValidPath(path)) return false;

    StorePathSet incoming;

    /* Don't delete this path if any of its referrers are alive. */
    queryReferrers(path, incoming);

    /* If keep-derivations is set and this is a derivation, then
       don't delete the derivation if any of the outputs are alive. */
    if (state.gcKeepDerivations && path.isDerivation()) {
        for (auto & [name, maybeOutPath] : queryPartialDerivationOutputMap(path))
            if (maybeOutPath &&
                isValidPath(*maybeOutPath) &&
                queryPathInfo(*maybeOutPath)->deriver == path
                )
                incoming.insert(*maybeOutPath);
    }

    /* If keep-outputs is set, then don't delete this path if there
       are derivers of this path that are not garbage. */
    if (state.gcKeepOutputs) {
        auto derivers = queryValidDerivers(path);
        for (auto & i : derivers)
            incoming.insert(i);
    }

    for (auto & i : incoming)
        if (i != path)
            if (canReachRoot(state, visited, i)) {
                state.alive.insert(path);
                return true;
            }

    return false;
}


void LocalStore::tryToDelete(GCState & state, const Path & path)
{
    checkInterrupt();

    auto realPath = realStoreDir + "/" + std::string(baseNameOf(path));
    if (realPath == linksDir || realPath == trashDir) return;

    //Activity act(*logger, lvlDebug, format("considering whether to delete '%1%'") % path);

    auto storePath = maybeParseStorePath(path);

    if (!storePath || !isValidPath(*storePath)) {
        /* A lock file belonging to a path that we're building right
           now isn't garbage. */
        if (isActiveTempFile(state, path, ".lock")) return;

        /* Don't delete .chroot directories for derivations that are
           currently being built. */
        if (isActiveTempFile(state, path, ".chroot")) return;

        /* Don't delete .check directories for derivations that are
           currently being built, because we may need to run
           diff-hook. */
        if (isActiveTempFile(state, path, ".check")) return;
    }

    StorePathSet visited;

    if (storePath && canReachRoot(state, visited, *storePath)) {
        debug("cannot delete '%s' because it's still reachable", path);
    } else {
        /* No path we visited was a root, so everything is garbage.
           But we only delete ‘path’ and its referrers here so that
           ‘nix-store --delete’ doesn't have the unexpected effect of
           recursing into derivations and outputs. */
        for (auto & i : visited)
            state.dead.insert(i);
        if (state.shouldDelete)
            deletePathRecursive(state, path);
    }
}


/* Unlink all files in /nix/store/.links that have a link count of 1,
   which indicates that there are no other links and so they can be
   safely deleted.  FIXME: race condition with optimisePath(): we
   might see a link count of 1 just before optimisePath() increases
   the link count. */
void LocalStore::removeUnusedLinks(const GCState & state)
{
#ifndef _WIN32
    AutoCloseDir dir(opendir(linksDir.c_str()));
    if (!dir) throw PosixError("opening directory '%1%'", linksDir);

    int64_t actualSize = 0, unsharedSize = 0;

    struct dirent * dirent;
    while (errno = 0, dirent = readdir(dir.get())) {
        checkInterrupt();
        string name = dirent->d_name;
        if (name == "." || name == "..") continue;
        Path path = linksDir + "/" + name;

        auto st = lstatPath(path);

        if (st.st_nlink != 1) {
            actualSize += st.st_size;
            unsharedSize += (st.st_nlink - 1) * st.st_size;
            continue;
        }

        printMsg(lvlTalkative, format("deleting unused link '%1%'") % path);

        if (unlink(path.c_str()) == -1)
            throw PosixError("deleting '%1%'", path);

        state.results.bytesFreed += st.st_size;
    }

    struct stat st;
    if (stat(linksDir.c_str(), &st) == -1)
        throw PosixError("statting '%1%'", linksDir);
    int64_t overhead = st.st_blocks * 512ULL;

    printInfo("note: currently hard linking saves %.2f MiB",
        ((unsharedSize - actualSize - overhead) / (1024.0 * 1024.0)));
#else
    long long actualSize = 0, unsharedSize = 0;

    WIN32_FIND_DATAW wfd;
    std::wstring wlinksDir = pathW(linksDir);
    HANDLE hFind = FindFirstFileExW((wlinksDir + L"\\*").c_str(), FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 0);
    if (hFind == INVALID_HANDLE_VALUE) {
        throw WinError("FindFirstFileExW when LocalStore::removeUnusedLinks()");
    } else {
        do {
            bool isDot    = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && wfd.cFileName[0]==L'.' && wfd.cFileName[1]==L'\0';
            bool isDotDot = (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && wfd.cFileName[0]==L'.' && wfd.cFileName[1]==L'.' && wfd.cFileName[2]==L'\0';
            if (isDot || isDotDot)
                continue;

            checkInterrupt();

            BY_HANDLE_FILE_INFORMATION bhfi;
            std::wstring wpath = wlinksDir + L'\\' + wfd.cFileName;
            HANDLE hFile = CreateFileW(wpath.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                                       FILE_FLAG_POSIX_SEMANTICS |
                                       ((wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ? FILE_FLAG_OPEN_REPARSE_POINT : 0) |
                                       ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY    ) != 0 ? FILE_FLAG_BACKUP_SEMANTICS   : 0),
                                       0);
            if (hFile == INVALID_HANDLE_VALUE)
                throw WinError("CreateFileW when LocalStore::removeUnusedLinks() '%1%'", to_bytes(wpath));
            if (!GetFileInformationByHandle(hFile, &bhfi))
                throw WinError("GetFileInformationByHandle when LocalStore::removeUnusedLinks() '%1%'", to_bytes(wpath));
            CloseHandle(hFile);

            uint64_t size = (uint64_t(bhfi.nFileSizeHigh) << 32) + bhfi.nFileSizeLow;
            if (bhfi.nNumberOfLinks != 1) {
                actualSize += size;
                unsharedSize += (bhfi.nNumberOfLinks - 1) * size;
                continue;
            }

            printMsg(lvlTalkative, format("deleting unused link '%1%'") % to_bytes(wpath));
    
            if (!DeleteFileW(wpath.c_str()))
                throw WinError("DeleteFileW when LocalStore::removeUnusedLinks() '%1%'", to_bytes(wpath));
            state.results.bytesFreed += size;

        } while(FindNextFileW(hFind, &wfd));
        WinError winError("FindNextFileW when LocalStore::removeUnusedLinks()");
        if (winError.lastError != ERROR_NO_MORE_FILES)
            throw winError;
        FindClose(hFind);
    }

    printInfo("note: currently hard linking saves %.2f MiB",
        ((unsharedSize - actualSize) / (1024.0 * 1024.0)));
#endif
}


void LocalStore::collectGarbage(const GCOptions & options, GCResults & results)
{
#ifdef _WIN32
    std::cerr << "LocalStore::collectGarbage" <<std::endl;
#endif
    GCState state(options, results);
    state.gcKeepOutputs = settings.gcKeepOutputs;
    state.gcKeepDerivations = settings.gcKeepDerivations;

    /* Using `--ignore-liveness' with `--delete' can have unintended
       consequences if `keep-outputs' or `keep-derivations' are true
       (the garbage collector will recurse into deleting the outputs
       or derivers, respectively).  So disable them. */
    if (options.action == GCOptions::gcDeleteSpecific && options.ignoreLiveness) {
        state.gcKeepOutputs = false;
        state.gcKeepDerivations = false;
    }

    state.shouldDelete = options.action == GCOptions::gcDeleteDead || options.action == GCOptions::gcDeleteSpecific;

    if (state.shouldDelete)
        deletePath(reservedPath);

    /* Acquire the global GC root.  This prevents
       a) New roots from being added.
       b) Processes from creating new temporary root files. */
#ifndef _WIN32
    AutoCloseFD fdGCLock = openGCLock(ltWrite);
#else
    AutoCloseWindowsHandle fdGCLock = openGCLock(ltWrite);
#endif

    /* Find the roots.  Since we've grabbed the GC lock, the set of
       permanent roots cannot increase now. */
    printInfo("finding garbage collector roots...");
    Roots rootMap;
    if (!options.ignoreLiveness)
        findRootsNoTemp(rootMap, true);

    for (auto & i : rootMap) state.roots.insert(i.first);

#ifndef _WIN32
    /* Read the temporary roots.  This acquires read locks on all
       per-process temporary root files.  So after this point no paths
       can be added to the set of temporary roots. */
    FDs fds;
    Roots tempRoots;
    findTempRoots(fds, tempRoots, true);
    for (auto & root : tempRoots) {
        state.tempRoots.insert(root.first);
        state.roots.insert(root.first);
    }
#endif

    /* After this point the set of roots or temporary roots cannot
       increase, since we hold locks on everything.  So everything
       that is not reachable from `roots' is garbage. */

    if (state.shouldDelete) {
        if (pathExists(trashDir)) deleteGarbage(state, trashDir);
        try {
            createDirs(trashDir);
        } catch (PosixError & e) {
            if (e.errNo == ENOSPC) {
                printInfo("note: can't create trash directory: %s", e.msg());
                state.moveToTrash = false;
            }
#ifdef _WIN32
        } catch (WinError & e) {
            throw e; // TODO
#endif
        }
    }

    /* Now either delete all garbage paths, or just the specified
       paths (for gcDeleteSpecific). */

    if (options.action == GCOptions::gcDeleteSpecific) {

        for (auto & i : options.pathsToDelete) {
            tryToDelete(state, printStorePath(i));
            if (state.dead.find(i) == state.dead.end())
                throw Error(
                    "cannot delete path '%1%' since it is still alive. "
                    "To find out why use: "
                    "nix-store --query --roots",
                    printStorePath(i));
        }

    } else if (options.maxFreed > 0) {

        if (state.shouldDelete)
            printInfo("deleting garbage...");
        else
            printInfo("determining live/dead paths...");

        try {
            Paths entries;
#ifndef _WIN32
            AutoCloseDir dir(opendir(realStoreDir.c_str()));
            if (!dir) throw PosixError("opening directory '%1%'", realStoreDir);

            /* Read the store and immediately delete all paths that
               aren't valid.  When using --max-freed etc., deleting
               invalid paths is preferred over deleting unreachable
               paths, since unreachable paths could become reachable
               again.  We don't use readDirectory() here so that GCing
               can start faster. */
            struct dirent * dirent;
            while (errno = 0, dirent = readdir(dir.get())) {
                checkInterrupt();
                string name = dirent->d_name;
                if (name == "." || name == "..") continue;
                Path path = storeDir + "/" + name;
                auto storePath = maybeParseStorePath(path);
                if (storePath && isValidPath(*storePath))
                    entries.push_back(path);
                else
                    tryToDelete(state, path);
            }

            dir.reset();
#else
            WIN32_FIND_DATAW wfd;
            HANDLE hFind = FindFirstFileExW((pathW(realStoreDir) + L"\\*").c_str(), FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 0);
            if (hFind == INVALID_HANDLE_VALUE) {
                throw WinError("FindFirstFileExW when collectGarbage '%1%'", realStoreDir);
            } else {
                do {
                    checkInterrupt();
                    if ((wfd.cFileName[0] == '.' && wfd.cFileName[1] == '\0')
                     || (wfd.cFileName[0] == '.' && wfd.cFileName[1] == '.' && wfd.cFileName[2] == '\0')) {
                    } else {
                        Path path = storeDir + "/" + to_bytes(wfd.cFileName);
                        if (isStorePath(path) && isValidPath(path))
                            entries.push_back(path);
                        else
                            tryToDelete(state, path);
                    }
                } while(FindNextFileW(hFind, &wfd));
                WinError winError("FindNextFileW when collectGarbage '%1%'", realStoreDir);
                if (winError.lastError != ERROR_NO_MORE_FILES)
                    throw winError;
                FindClose(hFind);
            }
#endif

            /* Now delete the unreachable valid paths.  Randomise the
               order in which we delete entries to make the collector
               less biased towards deleting paths that come
               alphabetically first (e.g. /nix/store/000...).  This
               matters when using --max-freed etc. */
            vector<Path> entries_(entries.begin(), entries.end());
            std::mt19937 gen(1);
            std::shuffle(entries_.begin(), entries_.end(), gen);

            for (auto & i : entries_)
                tryToDelete(state, i);

        } catch (GCLimitReached & e) {
        }
    }

#ifndef _NDEBUG
    /* paths shouldn't be  dead and alive at the same time */
    StorePathSet deadAndAlive;
    std::set_intersection(state.dead.begin(), state.dead.end(),
                          state.alive.begin(), state.alive.end(),
                          std::inserter(deadAndAlive, deadAndAlive.begin()));
    assert(deadAndAlive.size() == 0);
#endif

    if (state.options.action == GCOptions::gcReturnLive) {
        for (auto & i : state.alive)
            state.results.paths.insert(printStorePath(i));
        return;
    }

    if (state.options.action == GCOptions::gcReturnDead) {
        for (auto & i : state.dead)
            state.results.paths.insert(printStorePath(i));
        return;
    }

#ifndef _WIN32
    /* Allow other processes to add to the store from here on. */
    fdGCLock = -1;
    fds.clear();
#else
    fdGCLock = INVALID_HANDLE_VALUE;
#endif

    /* Delete the trash directory. */
    printInfo(format("deleting '%1%'") % trashDir);
    deleteGarbage(state, trashDir);

    /* Clean up the links directory. */
    if (options.action == GCOptions::gcDeleteDead || options.action == GCOptions::gcDeleteSpecific) {
        printInfo("deleting unused links...");
        removeUnusedLinks(state);
    }

    /* While we're at it, vacuum the database. */
    //if (options.action == GCOptions::gcDeleteDead) vacuumDB();
}


void LocalStore::autoGC(bool sync)
{
#ifdef HAVE_STATVFS
    static auto fakeFreeSpaceFile = getEnv("_NIX_TEST_FREE_SPACE_FILE");

    auto getAvail = [this]() -> uint64_t {
        if (fakeFreeSpaceFile)
            return std::stoll(readFile(*fakeFreeSpaceFile));

        struct statvfs st;
        if (statvfs(realStoreDir.c_str(), &st))
            throw PosixError("getting filesystem info about '%s'", realStoreDir);

        return (uint64_t) st.f_bavail * st.f_frsize;
    };

    std::shared_future<void> future;

    {
        auto state(_state.lock());

        if (state->gcRunning) {
            future = state->gcFuture;
            debug("waiting for auto-GC to finish");
            goto sync;
        }

        auto now = std::chrono::steady_clock::now();

        if (now < state->lastGCCheck + std::chrono::seconds(settings.minFreeCheckInterval)) return;

        auto avail = getAvail();

        state->lastGCCheck = now;

        if (avail >= settings.minFree || avail >= settings.maxFree) return;

        if (avail > state->availAfterGC * 0.97) return;

        state->gcRunning = true;

        std::promise<void> promise;
        future = state->gcFuture = promise.get_future().share();

        std::thread([promise{std::move(promise)}, this, avail, getAvail]() mutable {

            try {

                /* Wake up any threads waiting for the auto-GC to finish. */
                Finally wakeup([&]() {
                    auto state(_state.lock());
                    state->gcRunning = false;
                    state->lastGCCheck = std::chrono::steady_clock::now();
                    promise.set_value();
                });

                GCOptions options;
                options.maxFreed = settings.maxFree - avail;

                printInfo("running auto-GC to free %d bytes", options.maxFreed);

                GCResults results;

                collectGarbage(options, results);

                _state.lock()->availAfterGC = getAvail();

            } catch (...) {
                // FIXME: we could propagate the exception to the
                // future, but we don't really care.
                ignoreException();
            }

        }).detach();
    }

 sync:
    // Wait for the future outside of the state lock.
    if (sync) future.get();
#endif
}


}

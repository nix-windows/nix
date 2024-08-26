#include "profiles.hh"
#include "signals.hh"
#include "store-api.hh"
#include "local-fs-store.hh"
#include "users.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>


namespace nix {

namespace fs { using namespace std::filesystem; }

/**
 * Parse a generation name of the format
 * `<profilename>-<number>-link'.
 */
static std::optional<GenerationNumber> parseName(const std::string & profileName, const std::string & name)
{
    if (name.substr(0, profileName.size() + 1) != profileName + "-") return {};
    auto s = name.substr(profileName.size() + 1);
    auto p = s.find("-link");
    if (p == std::string::npos) return {};
    if (auto n = string2Int<unsigned int>(s.substr(0, p)))
        return *n;
    else
        return {};
}


std::pair<Generations, std::optional<GenerationNumber>> findGenerations(fs::path profile)
{
    Generations gens;

    fs::path profileDir = profile.parent_path();
    auto profileName = profile.filename().string();

    for (auto & i : fs::directory_iterator{profileDir}) {
        checkInterrupt();
        if (auto n = parseName(profileName, i.path().filename().string())) {
            auto path = i.path().string();
            gens.push_back({
                .number = *n,
                .path = path,
                .creationTime = lstat(path).st_mtime
            });
        }
    }

    gens.sort([](const Generation & a, const Generation & b)
    {
        return a.number < b.number;
    });

    return {
        gens,
        fs::symlink_exists(profile)
            ? parseName(profileName, fs::read_symlink(profile).string())
            : std::nullopt
    };
}


/**
 * Create a generation name that can be parsed by `parseName()`.
 */
static fs::path makeName(const fs::path & profile, GenerationNumber num)
{
    return fmt("%s-%s-link", profile, num);
}


fs::path createGeneration(LocalFSStore & store, fs::path profile, StorePath outPath)
{
    /* The new generation number should be higher than old the
       previous ones. */
    auto [gens, dummy] = findGenerations(profile);

    GenerationNumber num;
    if (gens.size() > 0) {
        Generation last = gens.back();

        if (fs::read_symlink(last.path) == store.printStorePath(outPath)) {
            /* We only create a new generation symlink if it differs
               from the last one.

               This helps keeping gratuitous installs/rebuilds from piling
               up uncontrolled numbers of generations, cluttering up the
               UI like grub. */
            return last.path;
        }

        num = last.number;
    } else {
        num = 0;
    }

    /* Create the new generation.  Note that addPermRoot() blocks if
       the garbage collector is running to prevent the stuff we've
       built from moving from the temporary roots (which the GC knows)
       to the permanent roots (of which the GC would have a stale
       view).  If we didn't do it this way, the GC might remove the
       user environment etc. we've just built. */
    fs::path generation = makeName(profile, num + 1);
    store.addPermRoot(outPath, generation.string());

    return generation;
}


void deleteGeneration(const fs::path & profile, GenerationNumber gen)
{
    fs::path generation = makeName(profile, gen);
    fs::remove(generation);
}

/**
 * Delete a generation with dry-run mode.
 *
 * Like `deleteGeneration()` but:
 *
 *  - We log what we are going to do.
 *
 *  - We only actually delete if `dryRun` is false.
 */
static void deleteGeneration2(const fs::path & profile, GenerationNumber gen, bool dryRun)
{
    if (dryRun)
        notice("would remove profile version %1%", gen);
    else {
        notice("removing profile version %1%", gen);
        deleteGeneration(profile, gen);
    }
}


void deleteGenerations(const fs::path & profile, const std::set<GenerationNumber> & gensToDelete, bool dryRun)
{
    PathLocks lock;
    lockProfile(lock, profile);

    auto [gens, curGen] = findGenerations(profile);

    if (gensToDelete.count(*curGen))
        throw Error("cannot delete current version of profile %1%'", profile);

    for (auto & i : gens) {
        if (!gensToDelete.count(i.number)) continue;
        deleteGeneration2(profile, i.number, dryRun);
    }
}

/**
 * Advanced the iterator until the given predicate `cond` returns `true`.
 */
static inline void iterDropUntil(Generations & gens, auto && i, auto && cond)
{
    for (; i != gens.rend() && !cond(*i); ++i);
}

void deleteGenerationsGreaterThan(const fs::path & profile, GenerationNumber max, bool dryRun)
{
    if (max == 0)
        throw Error("Must keep at least one generation, otherwise the current one would be deleted");

    PathLocks lock;
    lockProfile(lock, profile);

    auto [gens, _curGen] = findGenerations(profile);
    auto curGen = _curGen;

    auto i = gens.rbegin();

    // Find the current generation
    iterDropUntil(gens, i, [&](auto & g) { return g.number == curGen; });

    // Skip over `max` generations, preserving them
    for (GenerationNumber keep = 0; i != gens.rend() && keep < max; ++i, ++keep);

    // Delete the rest
    for (; i != gens.rend(); ++i)
        deleteGeneration2(profile, i->number, dryRun);
}

void deleteOldGenerations(const fs::path & profile, bool dryRun)
{
    PathLocks lock;
    lockProfile(lock, profile);

    auto [gens, curGen] = findGenerations(profile);

    for (auto & i : gens)
        if (i.number != curGen)
            deleteGeneration2(profile, i.number, dryRun);
}


void deleteGenerationsOlderThan(const fs::path & profile, time_t t, bool dryRun)
{
    PathLocks lock;
    lockProfile(lock, profile);

    auto [gens, curGen] = findGenerations(profile);

    auto i = gens.rbegin();

    // Predicate that the generation is older than the given time.
    auto older = [&](auto & g) { return g.creationTime < t; };

    // Find the first older generation, if one exists
    iterDropUntil(gens, i, older);

    /* Take the previous generation

       We don't want delete this one yet because it
       existed at the requested point in time, and
       we want to be able to roll back to it. */
    if (i != gens.rend()) ++i;

    // Delete all previous generations (unless current).
    for (; i != gens.rend(); ++i) {
        /* Creating date and generations should be monotonic, so lower
           numbered derivations should also be older. */
        assert(older(*i));
        if (i->number != curGen)
            deleteGeneration2(profile, i->number, dryRun);
    }
}


time_t parseOlderThanTimeSpec(std::string_view timeSpec)
{
    if (timeSpec.empty() || timeSpec[timeSpec.size() - 1] != 'd')
        throw UsageError("invalid number of days specifier '%1%', expected something like '14d'", timeSpec);

    time_t curTime = time(0);
    auto strDays = timeSpec.substr(0, timeSpec.size() - 1);
    auto days = string2Int<int>(strDays);

    if (!days || *days < 1)
        throw UsageError("invalid number of days specifier '%1%'", timeSpec);

    return curTime - *days * 24 * 3600;
}


void switchLink(fs::path link, fs::path target)
{
    /* Hacky. */
    if (target.parent_path() == link.parent_path()) target = target.filename();

    replaceSymlink(target, link);
}


void switchGeneration(
    const fs::path & profile,
    std::optional<GenerationNumber> dstGen,
    bool dryRun)
{
    PathLocks lock;
    lockProfile(lock, profile);

    auto [gens, curGen] = findGenerations(profile);

    std::optional<Generation> dst;
    for (auto & i : gens)
        if ((!dstGen && i.number < curGen) ||
            (dstGen && i.number == *dstGen))
            dst = i;

    if (!dst) {
        if (dstGen)
            throw Error("profile version %1% does not exist", *dstGen);
        else
            throw Error("no profile version older than the current (%1%) exists", curGen.value_or(0));
    }

    notice("switching profile from version %d to %d", curGen.value_or(0), dst->number);

    if (dryRun) return;

    switchLink(profile, dst->path);
}


void lockProfile(PathLocks & lock, const fs::path & profile)
{
    lock.lockPaths({profile}, fmt("waiting for lock on profile '%1%'", profile));
    lock.setDeletion(true);
}


std::string optimisticLockProfile(const fs::path & profile)
{
    return fs::symlink_exists(profile) ? fs::read_symlink(profile).string() : "";
}


fs::path profilesDir()
{
    auto profileRoot =
        isRootUser()
        ? rootProfilesDir()
        : createNixStateDir() / "profiles";
    fs::create_directories(profileRoot);
    return profileRoot;
}

fs::path rootProfilesDir()
{
    return fs::path{settings.nixStateDir} / "profiles/per-user/root";
}


fs::path getDefaultProfile()
{
    fs::path profileLink = settings.useXDGBaseDirectories
        ? createNixStateDir() / "profile"
        : getHome() / ".nix-profile";
    try {
        auto profile = profilesDir() / "profile";
        if (!fs::symlink_exists(profileLink)) {
            replaceSymlink(profile, profileLink);
        }
        // Backwards compatibiliy measure: Make root's profile available as
        // `.../default` as it's what NixOS and most of the init scripts expect
        auto globalProfileLink = fs::path{settings.nixStateDir} / "profiles/default";
        if (isRootUser() && !fs::symlink_exists(globalProfileLink)) {
            replaceSymlink(profile, globalProfileLink);
        }
        return fs::weakly_canonical(profileLink.parent_path() / fs::read_symlink(profileLink));
    } catch (Error &) {
        return profileLink;
    } catch (fs::filesystem_error &) {
        return profileLink;
    }
}

fs::path defaultChannelsDir()
{
    return profilesDir() / "channels";
}

fs::path rootChannelsDir()
{
    return rootProfilesDir() / "channels";
}

}

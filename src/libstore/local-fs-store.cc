#include "archive.hh"
#include "fs-accessor.hh"
#include "store-api.hh"
#include "local-fs-store.hh"
#include "globals.hh"
#include "compression.hh"
#include "derivations.hh"

namespace nix {

LocalFSStore::LocalFSStore(const Params & params)
    : Store(params)
{
}

struct LocalStoreAccessor : public FSAccessor
{
    ref<LocalFSStore> store;

    LocalStoreAccessor(ref<LocalFSStore> store) : store(store) { }

    Path toRealPath(const Path & path)
    {
        auto storePath = store->toStorePath(path).first;
        if (!store->isValidPath(storePath))
            throw InvalidPath("path '%1%' is not a valid store path", store->printStorePath(storePath));
        return store->getRealStoreDir() + std::string(path, store->storeDir.size());
    }

    FSAccessor::Stat stat1(const Path & path) override
    {
        auto realPath = toRealPath(path);
#ifndef _WIN32
        struct stat st;
        if (lstat(realPath.c_str(), &st)) {
            if (errno == ENOENT || errno == ENOTDIR) return {Type::tMissing, 0, false};
<<<<<<< HEAD
            throw PosixError(format("getting status-7 of '%1%'") % path);
||||||| merged common ancestors
            throw SysError(format("getting status of '%1%'") % path);
=======
            throw SysError("getting status of '%1%'", path);
>>>>>>> meson
        }

        if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode))
            throw Error("file '%1%' has unsupported type", path);

        return {
            S_ISREG(st.st_mode) ? Type::tRegular :
            S_ISLNK(st.st_mode) ? Type::tSymlink :
            Type::tDirectory,
            S_ISREG(st.st_mode) ? (uint64_t) st.st_size : 0,
            S_ISREG(st.st_mode) && st.st_mode & S_IXUSR};
#else
// TODO: make Windows native
        struct stat st;

        if (::stat(realPath.c_str(), &st)) {
            if (errno == ENOENT || errno == ENOTDIR) return {Type::tMissing, 0, false};
            throw PosixError(format("getting status-8 of '%1%'") % path);
        }

        unsigned char dt = getFileType(realPath);

        if (dt == DT_UNKNOWN)
            throw Error(format("file '%1%' has unsupported type") % path);

        return {
            dt == DT_REG ? Type::tRegular :
            dt == DT_LNK ? Type::tSymlink :
            Type::tDirectory,
            dt == DT_REG ? (uint64_t) st.st_size : 0,
            0};
#endif
    }

    StringSet readDirectory(const Path & path) override
    {
        auto realPath = toRealPath(path);

        auto entries = nix::readDirectory(realPath);

        StringSet res;
        for (auto & entry : entries)
            res.insert(entry.name());

        return res;
    }

    std::string readFile(const Path & path) override
    {
        return nix::readFile(toRealPath(path));
    }

    std::string readLink(const Path & path) override
    {
        return nix::readLink(toRealPath(path));
    }
};

ref<FSAccessor> LocalFSStore::getFSAccessor()
{
    return make_ref<LocalStoreAccessor>(ref<LocalFSStore>(
            std::dynamic_pointer_cast<LocalFSStore>(shared_from_this())));
}

void LocalFSStore::narFromPath(const StorePath & path, Sink & sink)
{
    if (!isValidPath(path))
        throw Error("path '%s' is not valid", printStorePath(path));
    dumpPath(getRealStoreDir() + std::string(printStorePath(path), storeDir.size()), sink);
}

const string LocalFSStore::drvsLogDir = "drvs";



std::shared_ptr<std::string> LocalFSStore::getBuildLog(const StorePath & path_)
{
    auto path = path_;

    if (!path.isDerivation()) {
        try {
            auto info = queryPathInfo(path);
            if (!info->deriver) return nullptr;
            path = *info->deriver;
        } catch (InvalidPath &) {
            return nullptr;
        }
    }

    auto baseName = std::string(baseNameOf(printStorePath(path)));

    for (int j = 0; j < 2; j++) {

        Path logPath =
            j == 0
            ? fmt("%s/%s/%s/%s", logDir, drvsLogDir, string(baseName, 0, 2), string(baseName, 2))
            : fmt("%s/%s/%s", logDir, drvsLogDir, baseName);
        Path logBz2Path = logPath + ".bz2";

        if (pathExists(logPath))
            return std::make_shared<std::string>(readFile(logPath));

        else if (pathExists(logBz2Path)) {
            try {
                return decompress("bzip2", readFile(logBz2Path));
            } catch (Error &) { }
        }

    }

    return nullptr;
}

}

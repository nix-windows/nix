#include "remote-fs-accessor.hh"
#include "nar-accessor.hh"
#include "json.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace nix {

RemoteFSAccessor::RemoteFSAccessor(ref<Store> store, const Path & cacheDir)
    : store(store)
    , cacheDir(cacheDir)
{
    if (cacheDir != "")
        createDirs(cacheDir);
}

Path RemoteFSAccessor::makeCacheFile(std::string_view hashPart, const std::string & ext)
{
    assert(cacheDir != "");
    return fmt("%s/%s.%s", cacheDir, hashPart, ext);
}

void RemoteFSAccessor::addToCache(std::string_view hashPart, const std::string & nar,
    ref<FSAccessor> narAccessor)
{
    nars.emplace(hashPart, narAccessor);

    if (cacheDir != "") {
        try {
            std::ostringstream str;
            JSONPlaceholder jsonRoot(str);
            listNar(jsonRoot, narAccessor, "", true);
            writeFile(makeCacheFile(hashPart, "ls"), str.str());

            /* FIXME: do this asynchronously. */
            writeFile(makeCacheFile(hashPart, "nar"), nar);

        } catch (...) {
            ignoreException();
        }
    }
}

std::pair<ref<FSAccessor>, Path> RemoteFSAccessor::fetch(const Path & path_)
{
    auto path = canonPath(path_);

    auto [storePath, restPath] = store->toStorePath(path);

    if (!store->isValidPath(storePath))
        throw InvalidPath("path '%1%' is not a valid store path", store->printStorePath(storePath));

    auto i = nars.find(std::string(storePath.hashPart()));
    if (i != nars.end()) return {i->second, restPath};

    StringSink sink;
    std::string listing;
    Path cacheFile;

    if (cacheDir != "" && pathExists(cacheFile = makeCacheFile(storePath.hashPart(), "nar"))) {

        try {
            listing = nix::readFile(makeCacheFile(storePath.hashPart(), "ls"));

            auto narAccessor = makeLazyNarAccessor(listing,
                [cacheFile](uint64_t offset, uint64_t length) {

#ifndef _WIN32
                    AutoCloseFD fd = open(cacheFile.c_str(), O_RDONLY | O_CLOEXEC);
                    if (!fd)
                        throw PosixError("opening NAR cache file '%s'", cacheFile);

                    if (lseek(fd.get(), offset, SEEK_SET) != (off_t) offset)
                        throw PosixError("seeking in '%s'", cacheFile);
#else
                    AutoCloseWindowsHandle fd = CreateFileW(pathW(cacheFile).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (fd.get() == INVALID_HANDLE_VALUE)
                        throw WinError("CreateFileW when RemoteFSAccessor::fetch '%1%'", cacheFile);

                    LARGE_INTEGER setOffset;
                    setOffset.QuadPart = offset;
                    LARGE_INTEGER newOffset;
                    if (!SetFilePointerEx(fd.get(), setOffset, &newOffset, FILE_BEGIN) || offset != newOffset.QuadPart)
                        throw WinError("CreateFileW when RemoteFSAccessor::fetch '%1%'", cacheFile);
#endif

                    std::string buf(length, 0);
                    readFull(fd.get(), (unsigned char *) buf.data(), length);

                    return buf;
                });

            nars.emplace(storePath.hashPart(), narAccessor);
            return {narAccessor, restPath};

        } catch (SysError &) { }

        try {
            *sink.s = nix::readFile(cacheFile);

            auto narAccessor = makeNarAccessor(sink.s);
            nars.emplace(storePath.hashPart(), narAccessor);
            return {narAccessor, restPath};

        } catch (SysError &) { }
    }

    store->narFromPath(storePath, sink);
    auto narAccessor = makeNarAccessor(sink.s);
    addToCache(storePath.hashPart(), *sink.s, narAccessor);
    return {narAccessor, restPath};
}

FSAccessor::Stat RemoteFSAccessor::stat1(const Path & path)
{
    auto res = fetch(path);
    return res.first->stat1(res.second);
}

StringSet RemoteFSAccessor::readDirectory(const Path & path)
{
    auto res = fetch(path);
    return res.first->readDirectory(res.second);
}

std::string RemoteFSAccessor::readFile(const Path & path)
{
    auto res = fetch(path);
    return res.first->readFile(res.second);
}

std::string RemoteFSAccessor::readLink(const Path & path)
{
    auto res = fetch(path);
    return res.first->readLink(res.second);
}

}

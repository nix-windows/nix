#pragma once

#include "util.hh"

namespace nix {

/* Open (possibly create) a lock file and return the file descriptor.
   -1 is returned if create is false and the lock could not be opened
   because it doesn't exist.  Any other error throws an exception. */
#ifndef __MINGW32__
AutoCloseFD openLockFile(const Path & path, bool create);
#else
AutoCloseWindowsHandle openLockFile(const Path & path, bool create);
#endif

/* Delete an open lock file. */
#ifndef __MINGW32__
void deleteLockFile(const Path & path, int fd);
#else
void deleteLockFile(const Path & path);
#endif

enum LockType { ltRead, ltWrite, ltNone };

#ifndef __MINGW32__
bool lockFile(int fd, LockType lockType, bool wait);
#else
bool lockFile(HANDLE handle, LockType lockType, bool wait);
#endif

MakeError(AlreadyLocked, Error);

class PathLocks
{
private:
#ifndef __MINGW32__
    typedef std::pair<int, Path> FDPair;
#else
    typedef std::pair<HANDLE, Path> FDPair;
#endif
    list<FDPair> fds;
    bool deletePaths;

public:
    PathLocks();
    PathLocks(const PathSet & paths,
        const string & waitMsg = "");
    bool lockPaths(const PathSet & _paths,
        const string & waitMsg = "",
        bool wait = true);
    ~PathLocks();
    void unlock();
    void setDeletion(bool deletePaths);
};

bool pathIsLockedByMe(const Path & path);

}

#pragma once

#include "sync.hh"
#include "types.hh"
#include "util.hh"

namespace nix {

class UserLock
{
private:
    Path fnUserLock;
    AutoCloseFile fdUserLock;

    bool isEnabled = false;
    string user;
#ifndef _WIN32
    uid_t uid = 0;
    gid_t gid = 0;
    std::vector<gid_t> supplementaryGIDs;
#endif

public:
    UserLock();

    void kill();

    string getUser() { return user; }
#ifndef _WIN32
    uid_t getUID() { assert(uid); return uid; }
    uid_t getGID() { assert(gid); return gid; }
    std::vector<gid_t> getSupplementaryGIDs() { return supplementaryGIDs; }
#endif
    bool findFreeUser();

    bool enabled() { return isEnabled; }

};

}

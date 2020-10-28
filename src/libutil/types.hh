#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#ifdef _MSC_VER
#undef min
#undef max
#endif
#endif

#include "ref.hh"

#include <list>
#include <set>
#include <string>
#include <map>
#include <vector>

namespace nix {

using std::list;
using std::set;
using std::vector;
using std::string;

typedef list<string> Strings;
typedef set<string> StringSet;
typedef std::map<string, string> StringMap;

/* Paths are just strings. */

typedef string Path;
typedef list<Path> Paths;
typedef set<Path> PathSet;

typedef vector<std::pair<string, string>> Headers;

/* Helper class to run code at startup. */
template<typename T>
struct OnStartup
{
    OnStartup(T && t) { t(); }
};

/* Wrap bools to prevent string literals (i.e. 'char *') from being
   cast to a bool in Attr. */
template<typename T>
struct Explicit {
    T t;

    bool operator ==(const Explicit<T> & other) const
    {
        return t == other.t;
    }
};

#ifdef _WIN32
std::ostream & operator << (std::ostream & os, const std::wstring & path) = delete;
std::string to_bytes(const std::wstring & path);
std::wstring from_bytes(const std::string & s);
#endif

}

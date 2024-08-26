#include "util.hh"
#include "users.hh"
#include "environment-variables.hh"
#include "executable-path.hh"
#include "file-system.hh"

namespace nix {

namespace fs { using namespace std::filesystem; }

fs::path getCacheDir()
{
    auto dir = getEnvOs(OS_STR("NIX_CACHE_HOME"));
    if (dir) {
        return *dir;
    } else {
        auto xdgDir = getEnvOs(OS_STR("XDG_CACHE_HOME"));
        if (xdgDir) {
            return fs::path{*xdgDir} / "nix";
        } else {
            return getHome() / ".cache" / "nix";
        }
    }
}


fs::path getConfigDir()
{
    auto dir = getEnvOs(OS_STR("NIX_CONFIG_HOME"));
    if (dir) {
        return *dir;
    } else {
        auto xdgDir = getEnvOs(OS_STR("XDG_CONFIG_HOME"));
        if (xdgDir) {
            return fs::path{*xdgDir} / "nix";
        } else {
            return getHome() / ".config" / "nix";
        }
    }
}

std::vector<fs::path> getConfigDirs()
{
    auto configDirs = getEnvOs(OS_STR("XDG_CONFIG_DIRS")).value_or(OS_STR("/etc/xdg"));
    auto result = ExecutablePath::parse(configDirs).directories;
    for (auto & p : result) {
        p /= "nix";
    }
    result.insert(result.begin(), getConfigDir());
    return result;
}


fs::path getDataDir()
{
    auto dir = getEnvOs(OS_STR("NIX_DATA_HOME"));
    if (dir) {
        return *dir;
    } else {
        auto xdgDir = getEnvOs(OS_STR("XDG_DATA_HOME"));
        if (xdgDir) {
            return fs::path{*xdgDir} / "nix";
        } else {
            return getHome() / ".local" / "share" / "nix";
        }
    }
}

fs::path getStateDir()
{
    auto dir = getEnvOs(OS_STR("NIX_STATE_HOME"));
    if (dir) {
        return *dir;
    } else {
        auto xdgDir = getEnvOs(OS_STR("XDG_STATE_HOME"));
        if (xdgDir) {
            return fs::path{*xdgDir} / "nix";
        } else {
            return getHome() / ".local" / "state"/ "nix";
        }
    }
}

fs::path createNixStateDir()
{
    fs::path dir = getStateDir();
    fs::create_directories(dir);
    return dir;
}


std::string expandTilde(std::string_view path)
{
    // TODO: expand ~user ?
    auto tilde = path.substr(0, 2);
    if (tilde == "~/" || tilde == "~")
        return (getHome() / std::string(path.substr(1))).string();
    else
        return std::string(path);
}

}

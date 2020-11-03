#include "globals.hh"
#include "util.hh"
#include "archive.hh"
#include "args.hh"
#include "abstract-setting-to-json.hh"

#include <algorithm>
#include <map>
#include <thread>
#ifndef _WIN32
#include <dlfcn.h>
#include <sys/utsname.h>
#endif

#include <nlohmann/json.hpp>


namespace nix {

#ifndef _WIN32
/* The default location of the daemon socket, relative to nixStateDir.
   The socket is in a directory to allow you to control access to the
   Nix daemon by setting the mode/ownership of the directory
   appropriately.  (This wouldn't work on the socket itself since it
   must be deleted and recreated on startup.) */
#define DEFAULT_SOCKET_PATH "/daemon-socket/socket"
#endif

Settings settings;

static GlobalConfig::Register rSettings(&settings);

Settings::Settings()
    : nixStore(canonPath(getEnv("NIX_STORE_DIR").value_or(getEnv("NIX_STORE").value_or(NIX_STORE_DIR))))
    , nixDataDir(canonPath(getEnv("NIX_DATA_DIR").value_or(NIX_DATA_DIR)))
    , nixLogDir(canonPath(getEnv("NIX_LOG_DIR").value_or(NIX_LOG_DIR)))
    , nixStateDir(canonPath(getEnv("NIX_STATE_DIR").value_or(NIX_STATE_DIR)))
    , nixConfDir(canonPath(getEnv("NIX_CONF_DIR").value_or(NIX_CONF_DIR)))
    , nixUserConfFiles(getUserConfigFiles())
    , nixBinDir(canonPath(getEnv("NIX_BIN_DIR").value_or(NIX_BIN_DIR)))
#ifndef _WIN32
    , nixPrefix(NIX_PREFIX)
    , nixLibexecDir(canonPath(getEnv("NIX_LIBEXEC_DIR").value_or(NIX_LIBEXEC_DIR)))
    , nixManDir(canonPath(NIX_MAN_DIR))
    , nixDaemonSocketFile(canonPath(getEnv("NIX_DAEMON_SOCKET_PATH").value_or(nixStateDir + DEFAULT_SOCKET_PATH)))
#endif
{
    fprintf(stderr, "NixStore=%s\n",        nixStore.c_str());
    fprintf(stderr, "NixDataDir=%s\n",      nixDataDir.c_str());
    fprintf(stderr, "NixLogDir=%s\n",       nixLogDir.c_str());
    fprintf(stderr, "NixStateDir=%s\n",     nixStateDir.c_str());
    fprintf(stderr, "NixConfDir=%s\n",      nixConfDir.c_str());
    fprintf(stderr, "NixBinDir=%s\n",       nixBinDir.c_str());
    assert(!nixStore.empty());
    assert(!nixDataDir.empty());
    assert(!nixLogDir.empty());
    assert(!nixStateDir.empty());
    assert(!nixConfDir.empty());
    assert(!nixBinDir.empty());

#ifndef _WIN32
    buildUsersGroup = getuid() == 0 ? "nixbld" : "";
#endif

    lockCPU = getEnv("NIX_AFFINITY_HACK") == "1";
    allowSymlinkedStore = getEnv("NIX_IGNORE_SYMLINK_STORE") == "1";

    caFile = getEnv("NIX_SSL_CERT_FILE").value_or(getEnv("SSL_CERT_FILE").value_or(""));
    if (caFile == "") {
        for (auto & fn : {
#ifndef _WIN32
                          "/etc/ssl/certs/ca-certificates.crt",
                          "/nix/var/nix/profiles/default/etc/ssl/certs/ca-bundle.crt"
#else
                          nixStateDir + "/profiles/default/etc/ssl/certs/ca-bundle.crt",
                          nixConfDir + "/certs/ca-bundle.crt"
#endif
                         })
            if (pathExists(fn)) {
                caFile = fn;
                break;
            }
    }

    /* Backwards compatibility. */
    auto s = getEnv("NIX_REMOTE_SYSTEMS");
    if (s) {
        Strings ss;
        for (auto & p : tokenizeString<Strings>(*s, ":"))
            ss.push_back("@" + p);
        builders = concatStringsSep(" ", ss);
    }

#if defined(__linux__) && defined(SANDBOX_SHELL)
    sandboxPaths = tokenizeString<StringSet>("/bin/sh=" SANDBOX_SHELL);
#endif


/* chroot-like behavior from Apple's sandbox */
#if __APPLE__
    sandboxPaths = tokenizeString<StringSet>("/System/Library/Frameworks /System/Library/PrivateFrameworks /bin/sh /bin/bash /private/tmp /private/var/tmp /usr/lib");
    allowedImpureHostPrefixes = tokenizeString<StringSet>("/System/Library /usr/lib /dev /bin/sh");
#endif
}

void loadConfFile()
{
    globalConfig.applyConfigFile(settings.nixConfDir + "/nix.conf");

    /* We only want to send overrides to the daemon, i.e. stuff from
       ~/.nix/nix.conf or the command line. */
    globalConfig.resetOverriden();

    auto files = settings.nixUserConfFiles;
    for (auto file = files.rbegin(); file != files.rend(); file++) {
        globalConfig.applyConfigFile(*file);
    }

    auto nixConfEnv = getEnv("NIX_CONFIG");
    if (nixConfEnv.has_value()) {
        globalConfig.applyConfig(nixConfEnv.value(), "NIX_CONFIG");
    }

}

std::vector<Path> getUserConfigFiles()
{
    // Use the paths specified in NIX_USER_CONF_FILES if it has been defined
    auto nixConfFiles = getEnv("NIX_USER_CONF_FILES");
    if (nixConfFiles.has_value()) {
        return tokenizeString<std::vector<string>>(nixConfFiles.value(), ":");
    }

    // Use the paths specified by the XDG spec
    std::vector<Path> files;
    auto dirs = getConfigDirs();
    for (auto & dir : dirs) {
        files.insert(files.end(), dir + "/nix/nix.conf");
    }
    return files;
}

unsigned int Settings::getDefaultCores()
{
    return std::max(1U, std::thread::hardware_concurrency());
}

StringSet Settings::getDefaultSystemFeatures()
{
    /* For backwards compatibility, accept some "features" that are
       used in Nixpkgs to route builds to certain machines but don't
       actually require anything special on the machines. */
    StringSet features{"nixos-test", "benchmark", "big-parallel", "recursive-nix"};

    #if __linux__
    if (access("/dev/kvm", R_OK | W_OK) == 0)
        features.insert("kvm");
    #endif

    return features;
}

bool Settings::isExperimentalFeatureEnabled(const std::string & name)
{
    auto & f = experimentalFeatures.get();
    return std::find(f.begin(), f.end(), name) != f.end();
}

void Settings::requireExperimentalFeature(const std::string & name)
{
    if (!isExperimentalFeatureEnabled(name))
        throw Error("experimental Nix feature '%1%' is disabled; use '--experimental-features %1%' to override", name);
}

bool Settings::isWSL1()
{
#ifdef __linux__
    struct utsname utsbuf;
    uname(&utsbuf);
    // WSL1 uses -Microsoft suffix
    // WSL2 uses -microsoft-standard suffix
    return hasSuffix(utsbuf.release, "-Microsoft");
#else
    return false;
#endif
}

const string nixVersion = PACKAGE_VERSION;

NLOHMANN_JSON_SERIALIZE_ENUM(SandboxMode, {
    {SandboxMode::smEnabled, true},
    {SandboxMode::smRelaxed, "relaxed"},
    {SandboxMode::smDisabled, false},
});

template<> void BaseSetting<SandboxMode>::set(const std::string & str, bool append)
{
    if (str == "true") value = smEnabled;
    else if (str == "relaxed") value = smRelaxed;
    else if (str == "false") value = smDisabled;
    else throw UsageError("option '%s' has invalid value '%s'", name, str);
}

template<> bool BaseSetting<SandboxMode>::isAppendable()
{
    return false;
}

template<> std::string BaseSetting<SandboxMode>::to_string() const
{
    if (value == smEnabled) return "true";
    else if (value == smRelaxed) return "relaxed";
    else if (value == smDisabled) return "false";
    else abort();
}

template<> void BaseSetting<SandboxMode>::convertToArg(Args & args, const std::string & category)
{
    args.addFlag({
        .longName = name,
        .description = "Enable sandboxing.",
        .category = category,
        .handler = {[=]() { override(smEnabled); }}
    });
    args.addFlag({
        .longName = "no-" + name,
        .description = "Disable sandboxing.",
        .category = category,
        .handler = {[=]() { override(smDisabled); }}
    });
    args.addFlag({
        .longName = "relaxed-" + name,
        .description = "Enable sandboxing, but allow builds to disable it.",
        .category = category,
        .handler = {[=]() { override(smRelaxed); }}
    });
}

void MaxBuildJobsSetting::set(const std::string & str, bool append)
{
    if (str == "auto") value = std::max(1U, std::thread::hardware_concurrency());
    else if (!string2Int(str, value))
        throw UsageError("configuration setting '%s' should be 'auto' or an integer", name);
}


void initPlugins()
{
#ifndef _WIN32
    for (const auto & pluginFile : settings.pluginFiles.get()) {
        Paths pluginFiles;
        try {
            auto ents = readDirectory(pluginFile);
            for (const auto & ent : ents)
                pluginFiles.emplace_back(pluginFile + "/" + ent.name());
        } catch (PosixError & e) {
            if (e.errNo != ENOTDIR)
                throw;
            pluginFiles.emplace_back(pluginFile);
        }
        for (const auto & file : pluginFiles) {
            /* handle is purposefully leaked as there may be state in the
               DSO needed by the action of the plugin. */
            void *handle =
                dlopen(file.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (!handle)
                throw Error("could not dynamically open plugin file '%s': %s", file, dlerror());
        }
    }

    /* Since plugins can add settings, try to re-apply previously
       unknown settings. */
    globalConfig.reapplyUnknownSettings();
    globalConfig.warnUnknownSettings();
#endif
}

}

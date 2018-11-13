#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <set>
#include <memory>
#include <tuple>
#include <iomanip>
#if __APPLE__
#include <sys/time.h>
#endif

#include "machines.hh"
#include "shared.hh"
#include "pathlocks.hh"
#include "globals.hh"
#include "serialise.hh"
#include "store-api.hh"
#include "derivations.hh"
#include "local-store.hh"

using namespace nix;
using std::cin;

static void handleAlarm(int sig) {
}

std::string escapeUri(std::string uri)
{
    std::replace(uri.begin(), uri.end(), '/', '_');
    return uri;
}

static string currentLoad;

#ifndef __MINGW32__
static AutoCloseFD openSlotLock(const Machine & m, unsigned long long slot)
#else
static AutoCloseWindowsHandle openSlotLock(const Machine & m, unsigned long long slot)
#endif
{
    return openLockFile(fmt("%s/%s-%d", currentLoad, escapeUri(m.storeUri), slot), true);
}

int main (int argc, char * * argv)
{
    return handleExceptions(argv[0], [&]() {
        initNix();

        logger = makeJSONLogger(*logger);

#ifndef __MINGW32__
        /* Ensure we don't get any SSH passphrase or host key popups. */
        unsetenv("DISPLAY");
        unsetenv("SSH_ASKPASS");
#endif
        if (argc != 2)
            throw UsageError("called without required arguments");

        verbosity = (Verbosity) std::stoll(argv[1]);
#ifndef __MINGW32__
        FdSource source(STDIN_FILENO);
#else
        FdSource source(GetStdHandle(STD_INPUT_HANDLE));
#endif
        /* Read the parent's settings. */
        while (readInt(source)) {
            auto name = readString(source);
            auto value = readString(source);
            settings.set(name, value);
        }

        settings.maxBuildJobs.set("1"); // hack to make tests with local?root= work

        initPlugins();

        auto store = openStore().cast<LocalStore>();

        /* It would be more appropriate to use $XDG_RUNTIME_DIR, since
           that gets cleared on reboot, but it wouldn't work on macOS. */
        currentLoad = store->stateDir + "/current-load";

        std::shared_ptr<Store> sshStore;
#ifndef __MINGW32__
        AutoCloseFD bestSlotLock;
#else
        AutoCloseWindowsHandle bestSlotLock;
#endif

        auto machines = getMachines();
        debug("got %d remote builders", machines.size());

        if (machines.empty()) {
            std::cerr << "# decline-permanently\n";
            return;
        }

        string drvPath;
        string storeUri;

        while (true) {

            try {
                auto s = readString(source);
                if (s != "try") return;
            } catch (EndOfFile &) { return; }

            auto amWilling = readInt(source);
            auto neededSystem = readString(source);
            source >> drvPath;
            auto requiredFeatures = readStrings<std::set<std::string>>(source);

            auto canBuildLocally = amWilling
                &&  (  neededSystem == settings.thisSystem
                    || settings.extraPlatforms.get().count(neededSystem) > 0);

            /* Error ignored here, will be caught later */
            mkdir(currentLoad.c_str()
#ifndef __MINGW32__
            , 0777
#endif
            );

            while (true) {
#ifndef __MINGW32__
                bestSlotLock = -1;
                AutoCloseFD lock = openLockFile(currentLoad + "/main-lock", true);
#else
                bestSlotLock = INVALID_HANDLE_VALUE;
                AutoCloseWindowsHandle lock = openLockFile(currentLoad + "/main-lock", true);
#endif
                lockFile(lock.get(), ltWrite, true);

                bool rightType = false;

                Machine * bestMachine = nullptr;
                unsigned long long bestLoad = 0;
                for (auto & m : machines) {
                    debug("considering building on remote machine '%s'", m.storeUri);

                    if (m.enabled && std::find(m.systemTypes.begin(),
                            m.systemTypes.end(),
                            neededSystem) != m.systemTypes.end() &&
                        m.allSupported(requiredFeatures) &&
                        m.mandatoryMet(requiredFeatures)) {
                        rightType = true;
#ifndef __MINGW32__
                        AutoCloseFD free;
#else
                        AutoCloseWindowsHandle free;
#endif
                        unsigned long long load = 0;
                        for (unsigned long long slot = 0; slot < m.maxJobs; ++slot) {
                            auto slotLock = openSlotLock(m, slot);
                            if (lockFile(slotLock.get(), ltWrite, false)) {
                                if (!free) {
                                    free = std::move(slotLock);
                                }
                            } else {
                                ++load;
                            }
                        }
                        if (!free) {
                            continue;
                        }
                        bool best = false;
                        if (!bestSlotLock) {
                            best = true;
                        } else if (load / m.speedFactor < bestLoad / bestMachine->speedFactor) {
                            best = true;
                        } else if (load / m.speedFactor == bestLoad / bestMachine->speedFactor) {
                            if (m.speedFactor > bestMachine->speedFactor) {
                                best = true;
                            } else if (m.speedFactor == bestMachine->speedFactor) {
                                if (load < bestLoad) {
                                    best = true;
                                }
                            }
                        }
                        if (best) {
                            bestLoad = load;
                            bestSlotLock = std::move(free);
                            bestMachine = &m;
                        }
                    }
                }

                if (!bestSlotLock) {
                    if (rightType && !canBuildLocally)
                        std::cerr << "# postpone\n";
                    else
                        std::cerr << "# decline\n";
                    break;
                }

#ifdef __MINGW32__
                FILETIME ft;
                GetSystemTimeAsFileTime(&ft);
                if (!SetFileTime(bestSlotLock.get(), &ft, &ft, &ft))
                    throw WinError("SetFileTime");
                lock = INVALID_HANDLE_VALUE;
#elif __APPLE__
                futimes(bestSlotLock.get(), NULL);
                lock = -1;
#else
                futimens(bestSlotLock.get(), NULL);
                lock = -1;
#endif

                try {

                    Activity act(*logger, lvlTalkative, actUnknown, fmt("connecting to '%s'", bestMachine->storeUri));

                    Store::Params storeParams;
                    if (hasPrefix(bestMachine->storeUri, "ssh://")) {
                        storeParams["max-connections"] ="1";
                        storeParams["log-fd"] = "4";
                        if (bestMachine->sshKey != "")
                            storeParams["ssh-key"] = bestMachine->sshKey;
                    }

                    sshStore = openStore(bestMachine->storeUri, storeParams);
                    sshStore->connect();
                    storeUri = bestMachine->storeUri;

                } catch (std::exception & e) {
#ifndef __MINGW32__
                    auto msg = chomp(drainFD(5, false));
                    printError("cannot build on '%s': %s%s",
                        bestMachine->storeUri, e.what(),
                        (msg.empty() ? "" : ": " + msg));
#else
fprintf(stderr, "todo abort %s:%d\n", __FILE__, __LINE__);fflush(stderr);
_exit(101);
#endif
                    bestMachine->enabled = false;
                    continue;
                }

                goto connected;
            }
        }

connected:
        close(5);

        std::cerr << "# accept\n" << storeUri << "\n";

        auto inputs = readStrings<PathSet>(source);
        auto outputs = readStrings<PathSet>(source);
#ifndef __MINGW32__
        AutoCloseFD uploadLock = openLockFile(currentLoad + "/" + escapeUri(storeUri) + ".upload-lock", true);
#else
        AutoCloseWindowsHandle uploadLock = openLockFile(currentLoad + "/" + escapeUri(storeUri) + ".upload-lock", true);
#endif
        {
            Activity act(*logger, lvlTalkative, actUnknown, fmt("waiting for the upload lock to '%s'", storeUri));
#ifndef __MINGW32__
            auto old = signal(SIGALRM, handleAlarm);
            alarm(15 * 60);
#endif
            if (!lockFile(uploadLock.get(), ltWrite, true))
                printError("somebody is hogging the upload lock for '%s', continuing...");
#ifndef __MINGW32__
            alarm(0);
            signal(SIGALRM, old);
#endif
        }

        auto substitute = settings.buildersUseSubstitutes ? Substitute : NoSubstitute;

        {
            Activity act(*logger, lvlTalkative, actUnknown, fmt("copying dependencies to '%s'", storeUri));
            copyPaths(store, ref<Store>(sshStore), inputs, NoRepair, NoCheckSigs, substitute);
        }

#ifndef __MINGW32__
        uploadLock = -1;
#else
        uploadLock = INVALID_HANDLE_VALUE;
#endif

        BasicDerivation drv(readDerivation(store->realStoreDir + "/" + baseNameOf(drvPath)));
        drv.inputSrcs = inputs;

        auto result = sshStore->buildDerivation(drvPath, drv);

        if (!result.success())
            throw Error("build of '%s' on '%s' failed: %s", drvPath, storeUri, result.errorMsg);

        PathSet missing;
        for (auto & path : outputs)
            if (!store->isValidPath(path)) missing.insert(path);

        if (!missing.empty()) {
            Activity act(*logger, lvlTalkative, actUnknown, fmt("copying outputs from '%s'", storeUri));
            store->locksHeld.insert(missing.begin(), missing.end()); /* FIXME: ugly */
            copyPaths(ref<Store>(sshStore), store, missing, NoRepair, NoCheckSigs, NoSubstitute);
        }

        return;
    });
}

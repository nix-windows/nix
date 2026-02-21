#include "nix/store/build/derivation-builder.hh"
#include "nix/store/local-store.hh"
#include "nix/store/path-info.hh"
#include "nix/store/posix-fs-canonicalise.hh"
#include "nix/store/globals.hh"
#include "nix/util/configuration.hh"
#include "nix/util/experimental-features.hh"
#include "nix/util/file-content-address.hh"
#include "nix/util/file-system.hh"
#include "nix/util/logging.hh"
#include "nix/util/muxable-pipe.hh"
#include "nix/util/os-string.hh"
#include "nix/util/processes.hh"
#include "nix/util/posix-source-accessor.hh"
#include "nix/util/source-path.hh"
#include "nix/util/strings.hh"
#include "nix/util/windows-async-pipe.hh"

#include <nlohmann/json.hpp>
#include <windows.h>

namespace nix {

/**
 * Windows derivation builder, based on volth's work in
 * https://github.com/nix-windows/nix/tree/windows
 */
class WindowsDerivationBuilder : public DerivationBuilder, public DerivationBuilderParams
{
    LocalStore & store;
    std::unique_ptr<DerivationBuilderCallbacks> miscMethods;

    const DerivationType derivationType;

    typedef StringMap Environment;
    Environment env;

    StringMap inputRewrites, outputRewrites;
    typedef std::map<StorePath, StorePath> RedirectedOutputs;
    RedirectedOutputs redirectedOutputs;
    OutputPathMap scratchOutputs;

    /**
     * The temporary directory used for the build.
     */
    std::filesystem::path tmpDir;

    /**
     * The original temp dir path before drive substitution.
     */
    std::filesystem::path tmpDirOrig;

    /**
     * The drive letter used for substitution (empty if none).
     */
    std::string substitutedDrive;

    /**
     * The process.
     */
    Pid pid;

    /**
     * Whether we have a child process running.
     */
    bool childStarted = false;

    /**
     * Async pipe for builder stdout/stderr, used with IOCP.
     */
    windows::AsyncPipe asyncBuilderOut;

    /**
     * Handle to NUL device for stdin.
     */
    AutoCloseFD nulHandle;

    /**
     * Job object to kill child on parent death.
     */
    AutoCloseFD jobObject;

    /**
     * The I/O Completion Port handle from the Worker, needed for
     * creating async pipes on Windows.
     */
    Descriptor ioCompletionPort;

    StorePath makeFallbackPath(const std::string & outputName)
    {
        return store.makeStorePath(
            "rewrite:" + std::string(drvPath.hashPart()) + ":name:" + outputName, Hash::dummy, outputName);
    }

    StorePath makeFallbackPath(const StorePath & path)
    {
        return store.makeStorePath(
            "rewrite:" + std::string(drvPath.hashPart()) + ":" + std::string(path.hashPart()),
            Hash::dummy,
            path.name());
    }

    const StorePathSet & originalPaths() override
    {
        return inputPaths;
    }

    bool isAllowed(const StorePath & path) override
    {
        return inputPaths.count(path) || addedPaths.count(path);
    }

    bool isAllowed(const DrvOutput & id) override
    {
        return addedDrvOutputs.count(id);
    }

public:

    WindowsDerivationBuilder(
        LocalStore & store,
        std::unique_ptr<DerivationBuilderCallbacks> miscMethods,
        DerivationBuilderParams params,
        Descriptor ioCompletionPort)
        : DerivationBuilderParams{std::move(params)}
        , store{store}
        , miscMethods{std::move(miscMethods)}
        , derivationType{drv.type()}
        , ioCompletionPort{ioCompletionPort}
    {
    }

    void cleanupOnDestruction() noexcept override
    {
        builderOut.release();

        try {
            killChild();
        } catch (...) {
            ignoreExceptionInDestructor();
        }
        try {
            cleanupBuild(false);
        } catch (...) {
            ignoreExceptionInDestructor();
        }
    }

    std::optional<Descriptor> startBuild() override;
    SingleDrvOutputs unprepareBuild() override;
    bool killChild() override;

    MuxablePipePollState::CommChannel getBuilderOutputChannel() override
    {
        return &asyncBuilderOut;
    }

private:

    void initEnv();
    void cleanupBuild(bool force);
    SingleDrvOutputs registerOutputs();

    void addDependencyImpl(const StorePath & path) override
    {
        addedPaths.insert(path);
    }
};

std::optional<Descriptor> WindowsDerivationBuilder::startBuild()
{
    auto buildDir = store.config->getBuildDir();
    createDirs(buildDir);

    tmpDirOrig = tmpDir = createTempDir(buildDir, "nix-build-" + std::string(drv.name));

    /* Try drive letter substitution to shorten paths and avoid MAX_PATH issues. */
    {
        DWORD bitmaskDrives = GetLogicalDrives();
        for (char letter = 'Z'; letter >= 'D'; letter--) {
            bool isBusy = (bitmaskDrives & (1 << (letter - 'A'))) != 0;
            if (isBusy)
                continue;

            std::string drivePath = std::string(1, letter) + ":";
            std::string shortPath = drivePath + "\\x";
            std::wstring wDrive = string_to_os_string(drivePath);
            std::wstring wOrig = string_to_os_string(tmpDirOrig.string());

            if (DefineDosDeviceW(DDD_NO_BROADCAST_SYSTEM, wDrive.c_str(), wOrig.c_str())) {
                tmpDir = shortPath;
                substitutedDrive = drivePath;
                if (!CreateDirectoryW(string_to_os_string(shortPath).c_str(), NULL)) {
                    auto err = GetLastError();
                    /* Undo the drive mapping if we can't create the dir. */
                    DefineDosDeviceW(
                        DDD_NO_BROADCAST_SYSTEM | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
                        wDrive.c_str(),
                        wOrig.c_str());
                    substitutedDrive.clear();
                    tmpDir = tmpDirOrig;
                    warn("CreateDirectoryW for substituted drive failed (error %d), using long path", err);
                }
                break;
            }
        }
        if (tmpDir == tmpDirOrig) {
            warn("no free drive letter available, build root will be '%s'", tmpDir.string());
        }
    }

    /* TODO: duplicates computeScratchOutputs() */
    for (auto & [outputName, status] : initialOutputs) {
        auto scratchPath = !status.known                                       ? makeFallbackPath(outputName)
                           : !status.known->isPresent()                        ? status.known->path
                           : buildMode != bmRepair && !status.known->isValid() ? status.known->path
                                                                               : makeFallbackPath(status.known->path);
        scratchOutputs.insert_or_assign(outputName, scratchPath);

        inputRewrites[hashPlaceholder(outputName)] = store.printStorePath(scratchPath);

        if (!status.known)
            continue;
        auto fixedFinalPath = status.known->path;

        if (fixedFinalPath == scratchPath)
            continue;

        deletePath(store.printStorePath(scratchPath));

        {
            std::string h1{fixedFinalPath.hashPart()};
            std::string h2{scratchPath.hashPart()};
            inputRewrites[h1] = h2;
        }

        redirectedOutputs.insert_or_assign(std::move(fixedFinalPath), std::move(scratchPath));
    }

    initEnv();

    /* TODO: duplicates logBuilderInfo() */
    printMsg(lvlChatty, "executing builder '%1%'", drv.builder);
    printMsg(lvlChatty, "using builder args '%1%'", concatStringsSep(" ", drv.args));
    for (auto & i : drv.env)
        printMsg(lvlVomit, "setting builder env variable '%1%'='%2%'", i.first, i.second);

    miscMethods->openLogFile();

    asyncBuilderOut.createAsyncPipe(ioCompletionPort);

    {
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;

        nulHandle = AutoCloseFD{CreateFileA(
            "NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &sa, OPEN_EXISTING, 0, NULL)};
        if (!nulHandle)
            throw windows::WinError("CreateFileA(NUL)");
    }

    buildResult.startTime = time(0);

    Strings builderArgs;
    std::map<std::wstring, std::wstring> uenv;

    {
        auto hostEnv = getEnv();
        static const std::set<std::string> passthrough = {
            "ComSpec",
            "windir",
            "SystemRoot",
            "SystemDrive",
            "PATHEXT",
            "NUMBER_OF_PROCESSORS",
            "OS",
            "PROCESSOR_ARCHITECTURE",
            "PROCESSOR_ARCHITEW6432",
        };
        for (auto & [key, val] : hostEnv) {
            for (auto & p : passthrough) {
                if (key.size() == p.size()) {
                    bool match = true;
                    for (size_t i = 0; i < key.size(); i++) {
                        if (tolower(key[i]) != tolower(p[i])) {
                            match = false;
                            break;
                        }
                    }
                    if (match)
                        uenv[string_to_os_string(key)] = string_to_os_string(val);
                }
            }
        }
    }

    for (auto & [key, val] : env) {
        std::string value = rewriteStrings(val, inputRewrites);
        uenv[string_to_os_string(key)] = string_to_os_string(value);
    }

    uenv[L"USERPROFILE"] = string_to_os_string(tmpDir.string());

    if (drv.isBuiltin()) {
        if (drv.builder == "builtin:fetchurl") {
            auto hostEnv = getEnv();
            for (auto & key : std::initializer_list<std::string>{"USERPROFILE", "TEMP", "PATH"}) {
                auto it = hostEnv.find(key);
                if (it != hostEnv.end())
                    uenv[string_to_os_string(key)] = string_to_os_string(it->second);
            }

            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            auto nixExe = os_string_to_string((std::filesystem::path(exePath).parent_path() / L"nix.exe").native());
            builderArgs.push_back(nixExe);

            builderArgs.push_back("--hashed-mirrors");
            builderArgs.push_back(settings.getLocalSettings().hashedMirrors.to_string());
            builderArgs.push_back("builtin-fetchurl");

            nlohmann::json json;
            for (auto & e : drv.env) {
                json[e.first] = rewriteStrings(e.second, inputRewrites);
            }
            builderArgs.push_back(json.dump());
        } else {
            throw Error("unsupported builtin function '%s'", std::string(drv.builder, 8));
        }
    } else {
        builderArgs.push_back(drv.builder);
        for (auto & i : drv.args)
            builderArgs.push_back(rewriteStrings(i, inputRewrites));
    }

    assert(!builderArgs.empty());
    std::string cmdline;
    for (const auto & arg : builderArgs) {
        if (!cmdline.empty())
            cmdline += ' ';
        cmdline += windowsEscape(arg);
    }
    std::wstring ucmdline = string_to_os_string(cmdline);

    std::wstring uenvline;
    for (auto & [key, val] : uenv)
        uenvline += key + L'=' + val + L'\0';
    uenvline += L'\0';

    STARTUPINFOW si = {};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nulHandle.get();
    si.hStdOutput = asyncBuilderOut.writeSide.get();
    si.hStdError = asyncBuilderOut.writeSide.get();

    PROCESS_INFORMATION pi = {};

    std::wstring wWorkDir =
        string_to_os_string(rewriteStrings(env.count("PWD") ? env["PWD"] : tmpDir.string(), inputRewrites));

    if (!CreateProcessW(
            NULL,
            const_cast<wchar_t *>(ucmdline.c_str()),
            NULL,
            NULL,
            TRUE, /* inherit handles */
            CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW | CREATE_SUSPENDED,
            const_cast<wchar_t *>(uenvline.c_str()),
            wWorkDir.c_str(),
            &si,
            &pi)) {
        throw windows::WinError("CreateProcessW(%s)", cmdline);
    }

    jobObject = AutoCloseFD{CreateJobObjectA(NULL, NULL)};
    if (!jobObject) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw windows::WinError("CreateJobObjectA");
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(jobObject.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

    if (!AssignProcessToJobObject(jobObject.get(), pi.hProcess)) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw windows::WinError("AssignProcessToJobObject");
    }

    if (!ResumeThread(pi.hThread)) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw windows::WinError("ResumeThread");
    }

    /* Close the write side of the pipe so we get EOF when the builder exits. */
    asyncBuilderOut.writeSide.close();
    CloseHandle(pi.hThread);

    pid = Pid{AutoCloseFD{pi.hProcess}};
    childStarted = true;

    /* Set builderOut to reference the async pipe read handle so the
       goal code can match ChildOutput events. This is a non-owning
       alias — asyncBuilderOut owns the actual handle. */
    builderOut = AutoCloseFD{asyncBuilderOut.readSide.get()};

    return builderOut.get();
}

/* TODO: duplicates initEnv() */
void WindowsDerivationBuilder::initEnv()
{
    env.clear();

    env["PATH"] = "/path-not-set";
    env["NIX_STORE"] = store.storeDir;

    env["NIX_BUILD_CORES"] = fmt(
        "%d",
        settings.getLocalSettings().buildCores ? settings.getLocalSettings().buildCores : settings.getDefaultCores());

    for (const auto & [name, info] : desugaredEnv.variables) {
        env[name] = info.prependBuildDirectory ? (tmpDir / info.value).string() : info.value;
    }

    for (const auto & [fileName, value] : desugaredEnv.extraFiles) {
        Path p = (tmpDir / fileName).string();
        writeFile(p, rewriteStrings(value, inputRewrites));
    }

    env["NIX_BUILD_TOP"] = tmpDir.string();
    env["TMPDIR"] = env["TEMPDIR"] = env["TMP"] = env["TEMP"] = tmpDir.string();
    env["PWD"] = tmpDir.string();

    if (derivationType.isFixed())
        env["NIX_OUTPUT_CHECKED"] = "1";

    if (!derivationType.isSandboxed()) {
        for (auto & i : drvOptions.impureEnvVars) {
            env[i] = getEnv(i).value_or("");
        }
    }

    env["NIX_LOG_FD"] = "2";
    env["TERM"] = "xterm-256color";
}

bool WindowsDerivationBuilder::killChild()
{
    if (!childStarted)
        return false;
    pid.kill();
    childStarted = false;
    miscMethods->childTerminated();
    return true;
}

SingleDrvOutputs WindowsDerivationBuilder::unprepareBuild()
{
    /* TODO: duplicates commonUnprepare() */
    int status = pid.wait();
    childStarted = false;

    debug("builder process for '%s' finished", store.printStorePath(drvPath));

    buildResult.timesBuilt++;
    buildResult.stopTime = time(0);

    miscMethods->childTerminated();

    builderOut.release(); /* non-owning alias; asyncBuilderOut owns the handle */
    asyncBuilderOut.close();
    nulHandle.close();

    miscMethods->closeLogFile();

    debug("builder for '%s' terminated with status %d", store.printStorePath(drvPath), status);

    if (status != 0) {
        cleanupBuild(false);

        throw BuilderFailureError{
            BuildResult::Failure::TransientFailure,
            status,
            "",
        };
    }

    auto builtOutputs = registerOutputs();

    cleanupBuild(true);

    return builtOutputs;
}

/* TODO: this is a simplified version of registerOutputs() from
   unix/build/derivation-builder-common.cc. It is missing reference
   scanning, output hash rewriting, fixed-output hash verification,
   CA derivation support, and multi-round determinism checks. These
   will need to be ported as Windows support matures, ideally by
   moving the common registerOutputs() to a platform-independent
   location. */
SingleDrvOutputs WindowsDerivationBuilder::registerOutputs()
{
    InodesSeen inodesSeen;
    std::map<std::string, ValidPathInfo> infos;

    for (auto & [outputName, scratchPath] : scratchOutputs) {
        auto actualPath = store.toRealPath(store.printStorePath(scratchPath));

        if (!pathExists(actualPath))
            throw BuildError(
                BuildResult::Failure::TransientFailure,
                "builder for '%s' failed to produce output path for output '%s' at '%s'",
                store.printStorePath(drvPath),
                outputName,
                actualPath);

        /* Canonicalise permissions. On Windows we don't have build users. */
        canonicalisePathMetaData(
            actualPath, {NIX_WHEN_SUPPORT_ACLS(settings.getLocalSettings().ignoredAcls)}, inodesSeen);

        auto accessor = makeFSSourceAccessor(std::filesystem::path{actualPath});
        auto narHashAndSize =
            hashPath({accessor, CanonPath::root}, FileSerialisationMethod::NixArchive, HashAlgorithm::SHA256);

        auto finalPath = scratchPath;
        ValidPathInfo newInfo{finalPath, {store, narHashAndSize.hash}};
        newInfo.narSize = narHashAndSize.numBytesDigested;
        newInfo.deriver = drvPath;
        newInfo.ultimate = true;

        auto redirected = get(redirectedOutputs, finalPath);
        if (redirected) {
            auto source = store.toRealPath(store.printStorePath(scratchPath));
            auto dest = store.toRealPath(store.printStorePath(*redirected));
            deletePath(dest);
            moveFile(source, dest);
            finalPath = *redirected;
            newInfo.path = finalPath;
        }

        store.signPathInfo(newInfo);
        infos.emplace(outputName, std::move(newInfo));
    }

    {
        ValidPathInfos infos2;
        for (auto & [outputName, newInfo] : infos)
            infos2.insert_or_assign(newInfo.path, newInfo);
        store.registerValidPaths(infos2);
    }

    SingleDrvOutputs builtOutputs;
    for (auto & [outputName, newInfo] : infos) {
        auto initialOutput = get(initialOutputs, outputName);
        assert(initialOutput);

        auto thisRealisation = Realisation{
            {
                .outPath = newInfo.path,
            },
            DrvOutput{initialOutput->outputHash, outputName},
        };
        if (experimentalFeatureSettings.isEnabled(Xp::CaDerivations) && !drv.type().isImpure()) {
            store.signRealisation(thisRealisation);
            store.registerDrvOutput(thisRealisation);
        }
        builtOutputs.emplace(outputName, thisRealisation);
    }

    return builtOutputs;
}

/* TODO: duplicates cleanupBuildCore() */
void WindowsDerivationBuilder::cleanupBuild(bool force)
{
    if (!substitutedDrive.empty()) {
        std::wstring wDrive = string_to_os_string(substitutedDrive);
        std::wstring wOrig = string_to_os_string(tmpDirOrig.string());
        if (!DefineDosDeviceW(
                DDD_NO_BROADCAST_SYSTEM | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
                wDrive.c_str(),
                wOrig.c_str())) {
            printError("warning: DefineDosDeviceW remove failed for %s", substitutedDrive);
        }
        substitutedDrive.clear();
    }

    if (!tmpDirOrig.empty()) {
        if (settings.keepFailed && !force && !drv.isBuiltin()) {
            printError("note: keeping build directory '%s'", tmpDirOrig.string());
        } else {
            deletePath(tmpDirOrig);
        }
        tmpDirOrig.clear();
        tmpDir.clear();
    }
}

void DerivationBuilderDeleter::operator()(DerivationBuilder * builder) noexcept
{
    if (!builder)
        return;

    builder->cleanupOnDestruction();

    delete builder;
}

DerivationBuilderUnique makeDerivationBuilder(
    LocalStore & store,
    std::unique_ptr<DerivationBuilderCallbacks> miscMethods,
    DerivationBuilderParams params,
    Descriptor ioCompletionPort)
{
    return DerivationBuilderUnique(
        new WindowsDerivationBuilder(store, std::move(miscMethods), std::move(params), ioCompletionPort));
}

DerivationBuilderUnique makeExternalDerivationBuilder(
    LocalStore & store,
    std::unique_ptr<DerivationBuilderCallbacks> miscMethods,
    DerivationBuilderParams params,
    const ExternalBuilder & handler,
    Descriptor)
{
    throw Error("external builders are not supported on Windows");
}

} // namespace nix

#include "command.hh"
#include "store-api.hh"
#include "archive.hh"

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <processthreadsapi.h>
#endif

using namespace nix;

struct CmdDumpPath : StorePathCommand
{
    std::string description() override
    {
        return "serialise a store path to stdout in NAR format";
    }

    std::string doc() override
    {
        return
          #include "store-dump-path.md"
          ;
    }

    void run(ref<Store> store, const StorePath & storePath) override
    {
        Descriptor standard_out =
#ifdef _WIN32
            GetStdHandle(STD_OUTPUT_HANDLE)
#else
            STDOUT_FILENO
#endif
            ;
        FdSink sink(standard_out);
        store->narFromPath(storePath, sink);
        sink.flush();
    }
};

static auto rDumpPath = registerCommand2<CmdDumpPath>({"store", "dump-path"});

struct CmdDumpPath2 : Command
{
    Path path;

    CmdDumpPath2()
    {
        expectArgs({
            .label = "path",
            .handler = {&path},
            .completer = completePath
        });
    }

    std::string description() override
    {
        return "serialise a path to stdout in NAR format";
    }

    std::string doc() override
    {
        return
          #include "nar-dump-path.md"
          ;
    }

    void run() override
    {
        Descriptor standard_out =
#ifdef _WIN32
            GetStdHandle(STD_OUTPUT_HANDLE)
#else
            STDOUT_FILENO
#endif
            ;
        FdSink sink(standard_out);
        dumpPath(path, sink);
        sink.flush();
    }
};

struct CmdNarDumpPath : CmdDumpPath2 {
    void run() override {
        warn("'nix nar dump-path' is a deprecated alias for 'nix nar pack'");
        CmdDumpPath2::run();
    }
};

static auto rCmdNarPack = registerCommand2<CmdDumpPath2>({"nar", "pack"});
static auto rCmdNarDumpPath = registerCommand2<CmdNarDumpPath>({"nar", "dump-path"});

#include "command.hh"
#include "store-api.hh"
#include "fs-accessor.hh"
#include "nar-accessor.hh"

#ifdef _WIN32
#include "builtins.hh"
#include "derivations.hh"
#include <nlohmann/json.hpp>
#endif

using namespace nix;

struct MixCat : virtual Args
{
    std::string path;

    void cat(ref<FSAccessor> accessor)
    {
        auto st = accessor->stat1(path);
        if (st.type == FSAccessor::Type::tMissing)
            throw Error("path '%1%' does not exist", path);
        if (st.type != FSAccessor::Type::tRegular)
            throw Error("path '%1%' is not a regular file", path);

        std::cout << accessor->readFile(path);
    }
};

struct CmdCatStore : StoreCommand, MixCat
{
    CmdCatStore()
    {
        expectArgs({
            .label = "path",
            .handler = {&path},
            .completer = completePath
        });
    }

    std::string description() override
    {
        return "print the contents of a file in the Nix store on stdout";
    }

    Category category() override { return catUtility; }

    void run(ref<Store> store) override
    {
        cat(store->getFSAccessor());
    }
};

struct CmdCatNar : StoreCommand, MixCat
{
    Path narPath;

    CmdCatNar()
    {
        expectArgs({
            .label = "nar",
            .handler = {&narPath},
            .completer = completePath
        });
        expectArg("path", &path);
    }

    std::string description() override
    {
        return "print the contents of a file inside a NAR file on stdout";
    }

    Category category() override { return catUtility; }

    void run(ref<Store> store) override
    {
        cat(makeNarAccessor(make_ref<std::string>(readFile(narPath))));
    }
};

static auto rCmdCatStore = registerCommand<CmdCatStore>("cat-store");
static auto rCmdCatNar = registerCommand<CmdCatNar>("cat-nar");


struct CmdLn : Command
{
    Path target, link;

    CmdLn()
    {
        expectArg("target", &target);
        expectArg("link", &link);
    }

    std::string description() override
    {
        return "make a symbolic link (because MSYS's ln does not do it)";
    }

    void run() override
    {
        createSymlink(target, absPath(link));
    }
};

static auto rCmdLn = registerCommand<CmdLn>("ln");


#ifdef _WIN32
// builtin:fetchurl builder as there is no fork()
struct CmdBuiltinFetchurl : Command
{
    std::string drvenv;

    CmdBuiltinFetchurl()
    {
        expectArg("drvenv", &drvenv);
    }

    std::string description() override
    {
        return "an internal command supporting `builtin:fetchurl` builder";
    }

    void run() override
    {
        logger = makeJSONLogger(*logger);

        std::cerr << "drvenv=" << drvenv << std::endl;
        auto j = nlohmann::json::parse(drvenv);
        std::cerr << "j=" << j << std::endl;

        BasicDerivation drv2;
        for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
          drv2.env[it.key()] = it.value();
        }
        builtinFetchurl(drv2, /*netrcData*/"");
    }
};

static auto rCmdBuiltinFetchurl = registerCommand<CmdBuiltinFetchurl>("builtin-fetchurl");
static RegisterCommand r4(make_ref<CmdBuiltinFetchurl>());
#endif

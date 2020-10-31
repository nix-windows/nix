#include "builtins.hh"
#include "download.hh"
#include "store-api.hh"
#include "archive.hh"
#include "compression.hh"

namespace nix {

void builtinFetchurl(const BasicDerivation & drv, const std::string & netrcData)
{
#ifndef _WIN32
    /* Make the host's netrc data available. Too bad curl requires
       this to be stored in a file. It would be nice if we could just
       pass a pointer to the data. */
    if (netrcData != "") {
        settings.netrcFile = "netrc";
        writeFile(settings.netrcFile, netrcData, 0600);
    }
#endif
    auto getAttr = [&](const string & name) {
        auto i = drv.env.find(name);
        if (i == drv.env.end()) throw Error(format("attribute '%s' missing") % name);
        return i->second;
    };

    Path storePath = getAttr("out");
    std::vector<std::string> mainUrls = tokenizeString<std::vector<string>>(getAttr("urls"), " ");
    assert(!mainUrls.empty());
    bool unpack = get(drv.env, "unpack", "") == "1";

    /* Note: have to use a fresh downloader here because we're in
       a forked process. */
    auto downloader = makeDownloader();

    auto fetch = [&](const std::vector<std::string> & urls) {
        std::string url = urls[0]; // todo: support multiurl
        auto source = sinkToSource([&](Sink & sink) {

            /* No need to do TLS verification, because we check the hash of
               the result anyway. */
            DownloadRequest request(url);
            request.verifyTLS = false;
            request.decompress = false;

            auto decompressor = makeDecompressionSink(
                unpack && hasSuffix(url, ".xz") ? "xz" : "none", sink);
            downloader->download(std::move(request), *decompressor);
            decompressor->finish();
        });

        if (unpack)
            restorePath(storePath, *source);
        else
            writeFile(storePath, *source);
#ifndef _WIN32
        auto executable = drv.env.find("executable");
        if (executable != drv.env.end() && executable->second == "1") {
            if (chmod(storePath.c_str(), 0755) == -1)
                throw PosixError(format("making '%1%' executable") % storePath);
        }
#endif
    };

    /* Try the hashed mirrors first. */
    if (getAttr("outputHashMode") == "flat") {
        std::vector<std::string> mirrorsUrls;
        for (auto hashedMirror : settings.hashedMirrors.get()) {
            if (!hasSuffix(hashedMirror, "/")) hashedMirror += '/';
            auto ht = parseHashType(getAttr("outputHashAlgo"));
            mirrorsUrls.push_back(hashedMirror + printHashType(ht) + "/" + Hash(getAttr("outputHash"), ht).to_string(Base16, false));
        }
        try {
            fetch(mirrorsUrls);
            return;
        } catch (Error & e) {
            debug(e.what());
        }
    }

    /* Otherwise try the specified URL. */
    fetch(mainUrls);
}

}

# Nix lib store build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.')

libutil_src = files(
    'affinity.cc',
    'archive.cc',
    'args.cc',
    'compression.cc',
    'config.cc',
    'error.cc',
    'hash.cc',
    'json.cc',
    'logging.cc',
    'rust-ffi.cc',
    'serialise.cc',
    'tarfile.cc',
    'thread-pool.cc',
    'url.cc',
    'util.cc',
    'xml-writer.cc',
)
libutil_headers = files(
    'abstract-setting-to-json.hh',
    'affinity.hh',
    'ansicolor.hh',
    'archive.hh',
    'args.hh',
    'callback.hh',
    'compression.hh',
    'config.hh',
    'error.hh',
    'finally.hh',
    'fmt.hh',
    'hash.hh',
    'json.hh',
    'logging.hh',
    'lru-cache.hh',
    'monitor-fd.hh',
    'pool.hh',
    'ref.hh',
    'rust-ffi.hh',
    'serialise.hh',
    'split.hh',
    'sync.hh',
    'tarfile.hh',
    'thread-pool.hh',
    'topo-sort.hh',
    'types.hh',
    'url.hh',
    'url-parts.hh',
    'util.hh',
    'xml-writer.hh',
)



# dependancies
#============================================================================

libutil_dep_list = [
    libarchive_dep,
    boost_dep,
    libbz2_dep,
    liblzma_dep,
    libbrotli_dep,
    openssl_dep,
    pthread_dep,
#    libsodium_dep,
    zlib_dep,
]




# Link args
#============================================================================

libutil_link_list = []

libutil_link_args = []




# compiler args
#============================================================================

libutil_cxx_args = []




# targets
#============================================================================




# build
#============================================================================

libutil_lib = library(
    'nixutil',
    install : true,
    install_mode : 'rwxr-xr-x',
    install_dir : libdir,
    include_directories : src_inc,
    sources : libutil_src,
    link_args : libutil_link_args,
    dependencies : libutil_dep_list)

install_headers(
    libutil_headers,
    install_dir : join_paths(includedir, 'nix'))

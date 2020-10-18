# Nix lib expr build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.')

libfetchers_src = files(
    'attrs.cc',
    'cache.cc',
    'fetchers.cc',
    'git.cc',
    'github.cc',
    'indirect.cc',
    'mercurial.cc',
    'path.cc',
    'registry.cc',
    'tarball.cc',
)

libfetchers_headers = files(
    'attrs.hh',
    'cache.hh',
    'fetchers.hh',
    'registry.hh',
)



# Link args
#============================================================================

libfetchers_link_list = [
    libutil_lib,
    libstore_lib,
]

libfetchers_link_args = []




# compiler args
#============================================================================

libfetchers_cxx_args = []




# build
#============================================================================

libfetchers_lib = library(
    'nixfetchers',
    install : true,
    install_mode : 'rwxr-xr-x',
    install_dir : libdir,
    include_directories : src_inc,
    link_with : libfetchers_link_list,
    sources : libfetchers_src,
    link_args : libfetchers_link_args,
)

install_headers(
    libfetchers_headers,
    install_dir : join_paths(includedir, 'nix'),
)

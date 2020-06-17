# Nix lib expr build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.')

libfetchers_src = files(
    join_paths(meson.source_root(), 'src/libfetchers/attrs.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/cache.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/fetchers.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/git.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/github.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/mercurial.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/path.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/tarball.cc'),
    join_paths(meson.source_root(), 'src/libfetchers/tree-info.cc'),
)

libfetchers_headers = files(
    join_paths(meson.source_root(), 'src/libfetchers/attrs.hh'),
    join_paths(meson.source_root(), 'src/libfetchers/cache.hh'),
    join_paths(meson.source_root(), 'src/libfetchers/fetchers.hh'),
    join_paths(meson.source_root(), 'src/libfetchers/tree-info.hh'),
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

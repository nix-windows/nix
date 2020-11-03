# Nix exec build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.')

nix_src_files = [
    join_paths(meson.source_root(), 'src/nix-store/dotgraph.cc'),
    join_paths(meson.source_root(), 'src/nix-store/nix-store.cc'),
]

#if editline_dep.found()
#  nix_src_files += [join_paths(meson.source_root(), 'src/nix/repl.cc')]
#endif

nix_src = files(nix_src_files)


nix_headers = files (
    join_paths(meson.source_root(), 'src/nix-store/dotgraph.hh'),
)

# dependancies
#============================================================================

nix_dep_list = [
    pthread_dep,
    libdl_dep,
    boost_dep,
    editline_dep,
    libsodium_dep,
    # There is a boemgc use in a static/inline method in libexpr. Sadly, meson
    # don't have any propogated public dep mechanism.
    gc_dep,
]





# Link args
#============================================================================

nix_link_list = [
    libutil_lib,
    libstore_lib,
    libmain_lib,
    libexpr_lib
]

nix_link_args = []




# compiler args
#============================================================================

nix_cxx_args = []




# targets
#============================================================================




# build
#============================================================================

nix_bin = executable(
    'nix-store',
    install : true,
    install_mode : 'rwxr-xr-x',
    install_dir : bindir,
    include_directories : src_inc,
    sources : nix_src,
    link_with : nix_link_list,
    dependencies : nix_dep_list)



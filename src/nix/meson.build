# Nix exec build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.')

nix_src_files = [
    'add-to-store.cc',
    'app.cc',
    'build.cc',
    'bundle.cc',
    'cat.cc',
    'command.cc',
    'copy.cc',
    'describe-stores.cc',
    'develop.cc',
    'diff-closures.cc',
    'dump-path.cc',
    'edit.cc',
    'eval.cc',
    'flake.cc',
    'hash.cc',
    'installables.cc',
    'legacy.cc',
    'log.cc',
    'ls.cc',
    'main.cc',
    'make-content-addressable.cc',
    'markdown.cc',
    'optimise-store.cc',
    'path-info.cc',
    'ping-store.cc',
    'profile.cc',
    'registry.cc',
    'run.cc',
    'search.cc',
    'show-config.cc',
    'show-derivation.cc',
    'sigs.cc',
    'upgrade-nix.cc',
    'verify.cc',
    'why-depends.cc',
]

if not build_on_windows
  nix_src_files += [
    'doctor.cc',

    '../build-remote/build-remote.cc',
    '../nix-build/nix-build.cc',
    '../nix-channel/nix-channel.cc',
    '../nix-collect-garbage/nix-collect-garbage.cc',
    '../nix-copy-closure/nix-copy-closure.cc',
    '../nix-daemon/nix-daemon.cc',
    '../nix-env/nix-env.cc',
    '../nix-env/user-env.cc',
    '../nix-instantiate/nix-instantiate.cc',
    '../nix-prefetch-url/nix-prefetch-url.cc',
    '../nix-store/dotgraph.cc',
    '../nix-store/graphml.cc',
    '../nix-store/nix-store.cc',

  ]
endif

if editline_dep.found()
  nix_src_files += ['repl.cc']
endif

nix_src = files(nix_src_files)

if not build_on_windows
  nix_ln_src = [
    [ bindir, 'nix-build' ],
    [ bindir, 'nix-channel' ],
    [ bindir, 'nix-collect-garbage' ],
    [ bindir, 'nix-copy-closure' ],
    [ bindir, 'nix-daemon' ],
    [ bindir, 'nix-env' ],
    [ bindir, 'nix-hash' ],
    [ bindir, 'nix-instantiate' ],
    [ bindir, 'nix-prefetch-url' ],
    [ bindir, 'nix-shell' ],
    [ bindir, 'nix-store' ],
    [ join_paths(libexecdir, 'nix'), 'build-remote' ],
  ]
endif

nix_header_files = [
    'command.hh',
    'installables.hh',
    'legacy.hh',
    'markdown.hh',

    '../nix/command.hh',
    '../nix/legacy.hh',
]
if not build_on_windows
  nix_header_files += [
    '../nix-env/user-env.hh',
    '../nix-store/dotgraph.hh',
    '../nix-store/graphml.hh',
  ]
endif
nix_headers = files(nix_header_files)

# dependancies
#============================================================================

nix_dep_list = [
    boost_dep,
    pthread_dep,
    libdl_dep,
    boost_dep,
    editline_dep,
    libsodium_dep,
	# In addition to the one in repl.cc, there is a boemgc use in a
	# static/inline method in libexpr. Sadly, meson don't have any propogated
	# public dep mechanism.
    gc_dep,
    lowdown_dep,
]





# Link args
#============================================================================

nix_link_list = [
    libutil_lib,
    libstore_lib,
    libmain_lib,
    libexpr_lib,
    libfetchers_lib,
]

nix_link_args = []




# compiler args
#============================================================================

nix_cxx_args = []




# targets
#============================================================================

embedded_file_dirs = [
  [ 'get-env.sh', '.' ],
  [ 'buildenv.nix', '../nix-env' ],
  [ 'unpack-channel.nix', '../nix-channel' ],
]

foreach entry : embedded_file_dirs
  file = entry[0]
  dir = entry[1]
  nix_src += custom_target(
    (file + '.gen.hh'),
    output : (file + '.gen.hh'),
    input : dir + '/' + file,
    command : [bash, '-c', gen_header, 'sh', '@OUTPUT@'],
  )
endforeach


# build
#============================================================================

nix_bin = executable(
    'nix',
    install : true,
    install_mode : 'rwxr-xr-x',
    install_dir : bindir,
    include_directories : src_inc,
    sources : nix_src,
    link_with : nix_link_list,
    dependencies : nix_dep_list)

if not build_on_windows
  # meson does not yet support installing symlink files.
  # this is a simple workaround
  foreach name : nix_ln_src
    meson.add_install_script(
        'ln',
        '-sf',
        join_paths(bindir, 'nix'),
        name[1])
    meson.add_install_script(
        'cp',
        '-r',
        name[1],
        join_paths(name[0], name[1]))
  endforeach

  install_headers(
    nix_headers,
    install_dir : join_paths(includedir, 'nix'),
  )
endif

if build_on_windows
  foreach dep : [ boost_dep, libcurl_dep, libbz2_dep, liblzma_dep, openssl_dep, sqlite3_dep ]
    foreach dll : dep.get_variable(internal: 'dlls', default_value: '').split(';')
      if dll != ''
        install_data(dll, install_dir : bindir)
      endif
    endforeach
  endforeach
endif

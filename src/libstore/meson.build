# Nix lib store build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.')

libstore_src_files = [
    'binary-cache-store.cc',
    'content-address.cc',
    'crypto.cc',
    'derivations.cc',
    'dummy-store.cc',
    'export-import.cc',
    'filetransfer.cc',
    'gc.cc',
    'globals.cc',
    'http-binary-cache-store.cc',
    'legacy-ssh-store.cc',
    'local-binary-cache-store.cc',
    'local-fs-store.cc',
    'local-store.cc',
    'lock.cc',
    'machines.cc',
    'misc.cc',
    'names.cc',
    'nar-accessor.cc',
    'nar-info.cc',
    'nar-info-disk-cache.cc',
    'optimise-store.cc',
    'path.cc',
    'pathlocks.cc',
    'profiles.cc',
    'references.cc',
    'remote-fs-accessor.cc',
    'remote-store.cc',
    's3-binary-cache-store.cc',
    'sqlite.cc',
    'ssh.cc',
    'ssh-store.cc',
    'store-api.cc',
    'uds-remote-store.cc',

    'build/derivation-goal.cc',
    'build/goal.cc',
    'build/hook-instance.cc',
    'build/local-store-build.cc',
    'build/substitution-goal.cc',
    'build/worker.cc',

    'builtins/buildenv.cc',
    'builtins/fetchurl.cc',
    'builtins/unpack-channel.cc',
]
if not build_on_windows
  libstore_src_files += [
    'daemon.cc',
    'parsed-derivations.cc',
  ]
endif
libstore_src = files(libstore_src_files)

libstore_headers_files = [
    'binary-cache-store.hh',
    'builtins.hh',
    'content-address.hh',
    'crypto.hh',
    'derivations.hh',
    'filetransfer.hh',
    'fs-accessor.hh',
    'globals.hh',
    'local-fs-store.hh',
    'local-store.hh',
    'lock.hh',
    'machines.hh',
    'names.hh',
    'nar-accessor.hh',
    'nar-info-disk-cache.hh',
    'nar-info.hh',
    'path.hh',
    'path-info.hh',
    'pathlocks.hh',
    'profiles.hh',
    'references.hh',
    'remote-fs-accessor.hh',
    'remote-store.hh',
    's3-binary-cache-store.hh',
    's3.hh',
    'serve-protocol.hh',
    'sqlite.hh',
    'ssh.hh',
    'store-api.hh',
    'uds-remote-store.hh',
    'worker-protocol.hh',

    'build/derivation-goal.hh',
    'build/goal.hh',
    'build/hook-instance.hh',
    'build/substitution-goal.hh',
    'build/worker.hh',

    'builtins/buildenv.hh',
]
if not build_on_windows
  libstore_headers_files += [
    'daemon.hh',
    'parsed-derivations.hh',
  ]
endif
libstore_headers = files(libstore_headers_files)

if not build_on_windows
  libstore_data = files(
    'sandbox-defaults.sb',
    'sandbox-minimal.sb',
    'sandbox-network.sb',
  )
endif

# dependancies
#============================================================================

libstore_dep_list = [
    boost_dep,
    libbz2_dep,
    libcurl_dep,
    libdl_dep,
    pthread_dep,
    sqlite3_dep,
    libsodium_dep
]

if sys_name.contains('linux')
    libstore_dep_list += libseccomp_dep
endif

if sys_name.contains('freebsd')
    libstore_dep_list += libdl_dep
endif




# Link args
#============================================================================

libstore_link_list = [
  libutil_lib,
]

libstore_link_args = []




# compiler args
#============================================================================

libstore_cxx_args = [
    '-DNIX_STORE_DIR="@0@" '.format(nixstoredir),
    '-DNIX_DATA_DIR="@0@" '.format(datadir),
    '-DNIX_STATE_DIR="@0@" '.format(join_paths(localstatedir, 'nix')),
    '-DNIX_LOG_DIR="@0@" '.format(join_paths(localstatedir, 'log/nix')),
    '-DNIX_CONF_DIR="@0@" '.format(join_paths(sysconfdir, 'nix')),
    '-DNIX_BIN_DIR="@0@" '.format(bindir),
]
if not build_on_windows
  libstore_cxx_args += [
    '-DNIX_PREFIX="@0@" '.format(prefix),
    '-DNIX_LIBEXEC_DIR="@0@" '.format(libexecdir),
    '-DNIX_MAN_DIR="@0@" '.format(mandir),
    '-DSANDBOX_SHELL="@0@" '.format(get_option('sandbox_shell')),
    '-DLSOF="@0@" '.format(lsof),
  ]
endif




# targets
#============================================================================

# FIXME: there is just no `sh`
if not build_on_windows
  libstore_src += custom_target(
    'schema.sql.gen.hh',
    output : 'schema.sql.gen.hh',
    input : 'schema.sql',
    command : [bash, '-c', gen_header, 'sh', '@OUTPUT@'],
  )
endif



# build
#============================================================================

libstore_lib = library(
    'nixstore',
    install : get_option('default_library') != 'static',
    install_mode : 'rwxr-xr-x',
    install_dir : libdir,
    include_directories : src_inc,
    link_with : libstore_link_list,
    sources : libstore_src,
    cpp_args : libstore_cxx_args,
    link_args : libstore_link_args,
    dependencies : libstore_dep_list)

if not build_on_windows
  install_headers(
    libstore_headers,
    install_dir : join_paths(includedir, 'nix'))

  install_data(
    libstore_data,
    install_dir : join_paths(datadir, 'nix/sandbox'))
endif

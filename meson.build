# Nix project build file
#============================================================================




# init
#============================================================================


# init project
#-------------------------------------------------
project(
    'nix',
    'cpp',

    default_options : [
        'cpp_std=c++17',
    ],
#   version : run_command('cat', './.version').stdout().strip(),
    version : '2.1.3', # no cat so far
    license : 'MIT'
)

# init compiler
#-------------------------------------------------
sys_name = host_machine.system().to_lower()

cpp = meson.get_compiler('cpp')
is_msvc = ['msvc', 'clang-cl', 'intel-cl'].contains(cpp.get_id())

if not is_msvc
  add_project_arguments(get_option('cxxflags'), language : 'cpp')
  add_project_link_arguments(get_option('ldflags'), language: 'cpp')

# We do a lot of args... we'd otherwise have to convert to __ va_args__
  add_project_arguments('-Wno-variadic-macros', language : 'cpp')
endif

if not sys_name.contains('windows')
  cmake = import('cmake')
  pkg = import('pkgconfig')
endif



# init configuration
#-------------------------------------------------
config_h = configuration_data()

config_h.set(
    'HAVE_CXX17', 1,
    description : 'Define if the compiler supports basic C++17 syntax')

package_name = meson.project_name()
config_h.set_quoted(
    'PACKAGE_NAME', package_name,
    description : 'Define to the full name of this package.'
    )

package_tarname = meson.project_name()
config_h.set_quoted(
    'PACKAGE_TARNAME', package_tarname,
    description : 'Define to the one symbol short name of this package.')

package_version = meson.project_version()
config_h.set_quoted(
    'PACKAGE_VERSION', package_version,
    description : 'Define to the version of this package.')

package_string = '@0@ @1@'.format(package_name, package_version)
config_h.set_quoted(
    'PACKAGE_STRING', package_string,
    description : 'Define to the full name and version of this package.')

package_url = 'https://nixos.org/nix/'
config_h.set_quoted(
    'PACKAGE_URL', package_url,
    description : 'Define to the home page for this package.')

package_bug_url = 'https://github.com/nixos/nix/issues'
config_h.set_quoted(
    'PACKAGE_BUGREPORT', package_bug_url,
    description : 'Define to the address where bug reports for this package should be sent.')




# env
#============================================================================


# set install directories
#-------------------------------------------------
if not sys_name.contains('windows')
  prefix = get_option('prefix')
  libdir = join_paths(prefix, get_option('libdir'))
  bindir = join_paths(prefix, get_option('bindir'))
  datadir = join_paths(prefix, get_option('datadir'))
  sysconfdir = join_paths(prefix, get_option('sysconfdir'))
  libexecdir = join_paths(prefix, get_option('libexecdir'))
  mandir = join_paths(prefix, get_option('mandir'))
  includedir = join_paths(prefix, get_option('includedir'))
else
  #prefix = get_option('prefix')
  libdir = 'C:\\work\\nix-bootstrap\\bin' # join_paths(prefix, get_option('libdir'))
  bindir = 'C:\\work\\nix-bootstrap\\bin' # join_paths(prefix, get_option('bindir'))
  datadir = 'C:\\work\\nix-bootstrap\\share' # join_paths(prefix, get_option('datadir'))
  sysconfdir = 'C:\\work\\nix-bootstrap\\etc' # join_paths(prefix, get_option('sysconfdir'))
  includedir = 'C:\\work\\nix-bootstrap\\include' # join_paths(prefix, get_option('includedir'))
endif


# set nix directories
#-------------------------------------------------
if not sys_name.contains('windows')
  # State should be stored in /nix/var, unless the user overrides it explicitly.
  if get_option('normal_var')
    localstatedir = '/nix/var'
  else
    localstatedir =  join_paths(prefix, get_option('localstatedir'))
  endif

  nixstoredir = get_option('nixstoredir')

  profiledir =  join_paths(sysconfdir, 'profile.d')
else
  localstatedir = 'C:\\nix\\var'
  nixstoredir = 'C:\\nix\\store' # get_option('nixstoredir')
endif

# Construct a Nix system name (like "i686-linux").
#-------------------------------------------------
if not sys_name.contains('windows')
  machine_name = host_machine.cpu()
  cpu_archs = ['x86_64', 'armv6', 'armv7', '']

  foreach cpu : cpu_archs
    if (host_machine.cpu().contains(cpu))
        if cpu.contains('armv')
            machine_name = cpu + '1'
        else
            machine_name = cpu
        endif
        break
    endif
  endforeach

  system= '"' + machine_name + '-' + sys_name + '"'
  message('system name: ' + system)
  config_h.set(
    'SYSTEM', system,
    description : 'platform identifier (`cpu-os`)')
endif



# required dependancies
#============================================================================


# look for required programs
#--------------------------------------------------
flex = find_program('flex', required : true)
bison = find_program('bison', required : true)

if not sys_name.contains('windows')
  cat = find_program('cat', required : true)
  ln = find_program('ln', required : true)
  cp = find_program('cp', required : true)
  rm = find_program('rm', required : true)
  bash = find_program('bash', required : true)
  echo = find_program('echo', required : true)
  patch = find_program('patch', required : true)
  xmllint = find_program('xmllint', required : true)
  sed = find_program('sed', required : true)
  tar = find_program('tar', required : true)
  bzip2 = find_program('bzip2', required : true)
  gzip = find_program('gzip', required : true)
  xz = find_program('xz', required : true)
  dot = find_program('dot', required : false)
  lsof = find_program('lsof', required : false)
  tr = find_program('tr', required : true)
  coreutils = run_command('dirname', cat.path()).stdout().strip()


  # Check whether the store optimiser can optimise symlinks.
  #-------------------------------------------------
  gen_header = '''
  ln -s bla tmp_link
  if ln tmp_link tmp_link2 2> /dev/null; then
    echo 1
  else
    echo 0
  fi
  '''

  run_command('sh', '-c', 'rm tmp_link*')
  can_link_symlink = run_command('sh', '-c', gen_header).stdout().strip()
  if can_link_symlink.to_int() == 1
    run_command('sh', '-c', 'rm tmp_link*')
  endif

  config_h.set('CAN_LINK_SYMLINK', can_link_symlink,
  description : 'Whether link() works on symlinks')
endif


# Look for boost, a required dependency.
#--------------------------------------------------
if (get_option('with_boost') != '' and is_msvc)
  boost_dep = declare_dependency(
    include_directories : [get_option('with_boost') + '/include'],
    link_args : ['-L' + get_option('with_boost') + '/lib' ])
elif (get_option('with_boost') != '')
  boost_dep = declare_dependency(
    variables : {'dlls': get_option('with_boost') + '/bin/libboost_context-mt.dll'},
    include_directories : [get_option('with_boost') + '/include'],
    link_args : ['-L' + get_option('with_boost') + '/lib', '-lboost_context-mt' ])
else
  boost_dep = declare_dependency(
    dependencies : [
        cpp.find_library('boost_system'),
        cpp.find_library('boost_context'),
        cpp.find_library('boost_thread')],
  )
endif

#if (boost_dep.found())
#    config_h.set('HAVE_BOOST', 1, description : 'Define if the Boost library is available.')
#endif


# Look for liblzma, a required dependency.
#--------------------------------------------------
if (get_option('with_lzma') != '') and is_msvc
  liblzma_dep = declare_dependency(
    variables : {'dlls': get_option('with_lzma') + '/bin/liblzma.dll' },
    include_directories : [get_option('with_lzma') + '/include'],
    link_args : ['-L' + get_option('with_lzma') + '/lib', '-lliblzma' ])
elif (get_option('with_lzma') != '')
  liblzma_dep = declare_dependency(
    variables : {'dlls': get_option('with_lzma') + '/bin/liblzma-5.dll' },
    include_directories : [get_option('with_lzma') + '/include'],
    link_args : ['-L' + get_option('with_lzma') + '/lib', '-llzma' ])
else
  liblzma_dep = declare_dependency(
    dependencies: dependency('liblzma'))
endif

# Look for libbrotli{enc,dec}.
#--------------------------------------------------
if sys_name.contains('windows')
  # todo
else
  libbrotli_dep = declare_dependency(
    dependencies: [
        dependency('libbrotlienc'),
        dependency('libbrotlidec')],
    link_args :  get_option('brotli_link_args'))
endif

# Look for OpenSSL, a required dependency.
#--------------------------------------------------
if get_option('with_openssl') != '' and is_msvc
  openssl_dep = declare_dependency(
    variables : {'dlls': ';'.join([ get_option('with_openssl') + '/bin/libeay32.dll',
                                    get_option('with_openssl') + '/bin/ssleay32.dll']) },
    include_directories : [get_option('with_openssl') + '/include'],
    link_args : ['-L' + get_option('with_openssl') + '/lib', '-llibeay32' ])
elif (get_option('with_openssl') != '')
  openssl_dep = declare_dependency(
    variables : {'dlls': ';'.join([ get_option('with_openssl') + '/bin/libssl-1_1.dll',
                                    get_option('with_openssl') + '/bin/libcrypto-1_1.dll']) },
    include_directories : [get_option('with_openssl') + '/include'],
    link_args : ['-L' + get_option('with_openssl') + '/lib', '-lssl', '-lcrypto' ])
else
  openssl_dep = declare_dependency(
     dependencies: cpp.find_library('ssl'))
endif


# Look for SQLite, a required dependency.
#--------------------------------------------------
if get_option('with_sqlite3') != '' and is_msvc
  sqlite3_dep = declare_dependency(
    variables : {'dlls': get_option('with_sqlite3') + '/bin/sqlite3.dll' },
    include_directories : [get_option('with_sqlite3') + '/include'],
    link_args : ['-L' + get_option('with_sqlite3') + '/lib', '-lsqlite3' ])
elif get_option('with_sqlite3') != ''
  sqlite3_dep = declare_dependency(
    variables : {'dlls': get_option('with_sqlite3') + '/bin/libsqlite3-0.dll' },
    include_directories : [get_option('with_sqlite3') + '/include'],
    link_args : ['-L' + get_option('with_sqlite3') + '/lib', '-lsqlite3' ])
else
  sqlite3_dep = declare_dependency(
    dependencies : dependency('sqlite3', version : '>= 3.6.19'))
endif

# Look for libcurl, a required dependency.
#--------------------------------------------------

if get_option('with_curl') != '' and is_msvc
  libcurl_dep = declare_dependency(
    variables : {'dlls': ';'.join([ get_option('with_curl') + '/bin/libcurl.dll',
                                    get_option('with_curl') + '/bin/zlib1.dll']) },
    include_directories : [get_option('with_curl') + '/include'],
    link_args : ['-L' + get_option('with_curl') + '/lib', '-llibcurl'])
elif (get_option('with_curl') != '') 
  libcurl_dep = declare_dependency(
    variables : {'dlls': ';'.join([ get_option('with_curl') + '/bin/libcurl-4.dll',
                                    get_option('with_curl') + '/bin/zlib1.dll',
                                    get_option('with_curl') + '/bin/libbrotlicommon.dll',
                                    get_option('with_curl') + '/bin/libbrotlidec.dll',
                                    get_option('with_curl') + '/bin/libbrotlienc.dll',
                                    get_option('with_curl') + '/bin/libidn2-0.dll',
                                    get_option('with_curl') + '/bin/libpsl-5.dll',
                                    get_option('with_curl') + '/bin/libnghttp2-14.dll',
                                    get_option('with_curl') + '/bin/libssh2-1.dll',
                                    get_option('with_curl') + '/bin/libidn2-0.dll',
                                    get_option('with_curl') + '/bin/libintl-8.dll',
                                    get_option('with_curl') + '/bin/libzstd.dll',
                                    get_option('with_curl') + '/bin/libunistring-2.dll',
                                    get_option('with_curl') + '/bin/libiconv-2.dll']) },
    include_directories : [get_option('with_curl') + '/include'],
    link_args : ['-L' + get_option('with_curl') + '/lib', '-lcurl'])
else
  libcurl_dep = declare_dependency(
    dependencies : dependency(
        'libcurl',
        # Meson was getting confused for some reason
        static : sys_name.contains('windows'),
    ))
endif


# Look for pthread, a required dependency.
#--------------------------------------------------

# Work around Meson not recognizing obscure alternative MCF threading.
# https://github.com/mesonbuild/meson/issues/553#issuecomment-552602641
if sys_name.contains('windows')
# pthread_dep = dependency('', required : false)
else
  pthread_dep = dependency('threads')
endif


# Look for libdl, a required dependency on Unix.
#--------------------------------------------------

libdl_needed = not sys_name.contains('windows')

if libdl_needed
  libdl_lib = cpp.find_library('dl', required : libdl_needed)

  if libdl_lib.found()
    libdl_dep = declare_dependency(
        dependencies : libdl_lib,
        link_args :  get_option('dl_link_args'),
        required : libdl_needed)
  else
    libdl_dep = dependency('', required: false)
  endif
endif


# Look for libbz2, a required dependency.
#--------------------------------------------------
if get_option('with_bz2') != '' and is_msvc
  libbz2_dep = declare_dependency(
    include_directories : [get_option('with_bz2') + '/include'],
    link_args : ['-L' + get_option('with_bz2') + '/lib', '-llibbz2'])
elif get_option('with_bz2') != ''
  libbz2_dep = declare_dependency(
    variables : {'dlls': get_option('with_bz2') + '/bin/libbz2-1.dll' },
    include_directories : [get_option('with_bz2') + '/include'],
    link_args : ['-L' + get_option('with_bz2') + '/lib', '-llibbz2'])
else
  libbz2_dep = declare_dependency(
    dependencies : cpp.find_library('bz2'))
endif

# Optional dependancies
#============================================================================

if not sys_name.contains('windows')
# Look for libsodium, an optional dependency.
#--------------------------------------------------
libsodium_lib = cpp.find_library('sodium', required: get_option('with_libsodium'))
if (libsodium_lib.found())
    libsodium_dep = declare_dependency(
        dependencies : libsodium_lib,
        link_args :  get_option('sodium_link_args'))
    config_h.set('HAVE_SODIUM', 1, description : 'Whether to use libsodium for cryptography.')
else
    libsodium_dep = dependency('', required: false)
endif


# Look for Boehm garbage collector, an optional dependency.
#--------------------------------------------------

gc_c_lib = dependency(
    'bdw-gc',
    required : get_option('with_gc'),
    disabler : true,
)
gc_cpp_lib = cpp.find_library(
    'gccpp',
    required : get_option('with_gc'),
)
if (gc_cpp_lib.found())
    gc_dep = declare_dependency(
        dependencies : [gc_cpp_lib, gc_c_lib],
        link_args : get_option('gc_link_args'),
    )
    config_h.set(
        'HAVE_BOEHMGC', 1,
        description : 'Whether to use the Boehm garbage collector.')
else
    gc_dep = dependency('', required: false)
endif


# Look for aws-cpp-sdk-s3.
#--------------------------------------------------
if (get_option('with_s3'))
  enable_s3 = meson.get_compiler('cpp').check_header('aws/s3/S3Client.h')

  aws_version = meson.get_compiler('cpp').get_define(
    'AWS_SDK_VERSION_STRING',
    prefix : '#include <aws/core/VersionConfig.h>'
  ).strip('"').split('.')

  conf_data.set('ENABLE_S3', 1, description : 'Whether to enable S3 support via aws-sdk-cpp.')
  conf_data.set('AWS_VERSION_MAJOR', aws_version[0], description : 'Major version of aws-sdk-cpp.')
  conf_data.set('AWS_VERSION_MINOR', aws_version[1], description : 'Minor version of aws-sdk-cpp.')
endif


# Look for editline, a dependency required just by nix-repl.
#--------------------------------------------------
# NOTE: The the libeditline.pc file was added only in libeditline >= 1.15.2, see
# https://github.com/troglobit/editline/commit/0a8f2ef4203c3a4a4726b9dd1336869cd0da8607,
# but e.g. Ubuntu 16.04 has an older version, so we fall back to searching for
# editline.h when the pkg-config approach fails.

editline_dep = dependency(
  'libeditline',
  include_type : 'system', # force -isystem so we don't forget to use <..>
  required : get_option('with_editline'),
)

if (editline_dep.found() and not cpp.has_function(
        'read_history',
        prefix : '#include <stdio.h>\n#include "editline.h"',
        args : '-lreadline'))
  warning('Nix requires libeditline; However, required functions do not work. Maybe ' +\
    'it is too old? >= 1.14 is required.')
endif

endif



# OS Specific checks 
#============================================================================
# Look for libsecppomp, required for Linux sandboxing.
if sys_name.contains('linux')
    libseccomp_dep = dependency(
        'libseccomp',
        version : '>= 2.3.1',
        not_found_message : 'Nix requires libseccomp on a linux host system')
    config_h.set(
        'HAVE_SECCOMP', 1,
        description : 'Whether seccomp is available and should be used for sandboxing.')
else
    libseccomp_dep = dependency('', required: false)
endif

if (sys_name.contains('freebsd'))
    add_project_arguments('-D_GNU_SOURCE', language : 'cpp')
    config_h.set('_GNU_SOURCE', 1)
endif

if (sys_name.contains('sunos'))
    # Solaris requires -lsocket -lnsl for network functions
endif

#if sys_name.contains('windows')
#    windows_version_desc = '''
#      Windows SDK version we will use.
#      Newer version will have their headers trimmed of new functionality.
#    '''
#    windows_version = '0x0A00' # Windows 10
#    config_h.set('WINVER', windows_version, description : windows_version_desc)
#    config_h.set('_WIN32_WINNT', windows_version, description : windows_version_desc)
#endif




# checking headers
#============================================================================
if (cpp.has_header('unistd.h'))
    config_h.set(
        'HAVE_UNISTD_H', 1,
        description: 'Define to 1 if you have the <unistd.h> header file.')
else
    config_h.set(
        'YY_NO_UNISTD_H', 1,
        description: 'Define to 1 if you have no <unistd.h> header file.')
endif


if not sys_name.contains('windows')
if (cpp.has_header('sys/stat.h'))
    config_h.set(
        'HAVE_SYS_STAT_H', 1,
        description : 'Define to 1 if you have the <sys/stat.h> header file.')
endif

if (cpp.has_header('sys/types.h'))
    config_h.set(
        'HAVE_SYS_TYPES_H', 1,
        description : 'Define to 1 if you have the <sys/types.h> header file.')
endif

if (cpp.has_header('sys/dir.h'))
    config_h.set(
        'HAVE_DIR_H', 1,
        description : 'Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR`')
endif

if (cpp.has_header('sys/ndir.h'))
    config_h.set(
        'HAVE_NDIR_H', 1,
        description : 'Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR`')
endif

has_dirent_h = cpp.has_header('dirent.h')
if (has_dirent_h)
    config_h.set(
        'HAVE_DIRENT_H', 1,
        description : 'Define to 1 if you have the <dirent.h> header file, and it defines `DIR`')
endif

if (cpp.has_header('locale.h'))
    config_h.set(
        'HAVE_LOCALE', 1,
        description : 'Define to 1 if you have the <locale.h> header file.')
endif

if (cpp.has_header('stdint.h'))
    config_h.set(
        'HAVE_STDINT_H', 1,
        description: 'Define to 1 if you have the <stdint.h> header file.')
endif

if (cpp.has_header('stdlib.h'))
    config_h.set(
        'HAVE_STDLIB_H', 1,
        description: 'Define to 1 if you have the <stdlib.h> header file.')
endif

if (cpp.has_header('strings.h'))
    config_h.set(
        'HAVE_STRINGS_H', 1,
        description: 'Define to 1 if you have the <strings.h> header file.')
endif

if (cpp.has_header('string.h'))
    config_h.set(
        'HAVE_STRING_H', 1,
        description: 'Define to 1 if you have the <strings.h> header file.')
endif

if (cpp.has_header('bzlib.h')) # dependencies : [ libbz2_dep ]
    # Unfortunately cannot results of `declare_dependency` above, and package
    # is missing pkg-config file.
    config_h.set(
        'HAVE_BZLIB_H', 1,
        description : 'Define to 1 if you have the <bzlib.h> header file.')
endif

if (cpp.has_header('inttypes.h'))
    config_h.set(
        'HAVE_INTTYPES_H', 1,
        description : 'Define to 1 if you have the <inttypes.h> header file.')
endif

if (cpp.has_header('memory.h'))
    config_h.set(
        'HAVE_MEMORY_H', 1,
        description : 'Define to 1 if you have the <memory.h> header file.')
endif

if (cpp.has_header('editline.h', dependencies : [ editline_dep ]))
    config_h.set(
        'HAVE_EDITLINE_H', 1,
        description : 'Define to 1 if you have the <editline.h> header file.')
endif




# checking functions
#============================================================================


if (cpp.has_function('lutimes'))
    config_h.set(
        'HAVE_LUTIMES', 1,
        description : 'Define to 1 if you have the `lutimes` function.')
endif

if (cpp.has_function('lchown'))
    config_h.set(
        'HAVE_LCHOWN', 1,
        description : 'Define to 1 if you have the `lchown` function.')
endif

if (cpp.has_function('pipe2'))
    config_h.set(
        'HAVE_PIPE2', 1,
        description : 'Define to 1 if you have the `pipe2` function.')
endif

if (cpp.has_function('posix_fallocate'))
    config_h.set(
        'HAVE_POSIX_FALLOCATE', 1,
        description : 'Define to 1 if you have the `posix_fallocate` function.')
endif

if (cpp.has_function('setresuid'))
    config_h.set(
        'HAVE_SETRESUID', 1,
        description : 'Define to 1 if you have the `setresuid` function.')
endif

if (cpp.has_function('setreuid'))
    config_h.set(
        'HAVE_SETREUID', 1,
        description : 'Define to 1 if you have the `setreuid` function.')
endif

if (cpp.has_function('statvfs'))
    config_h.set(
        'HAVE_STATVFS', 1,
        description : 'Define to 1 if you have the `statvfs` function.')
endif

if (cpp.has_function('strsignal'))
    config_h.set(
        'HAVE_STRSIGNAL', 1,
        description : 'Define to 1 if you have the `strsignal` function.')
endif

if (cpp.has_function('sysconf'))
    config_h.set(
        'HAVE_SYSCONF', 1,
        description : 'Define to 1 if you have the `sysconf` function.')
endif

pubsetbuff_c = '''
    #include <iostream>
    using namespace std;
    static char buf[1024];
    void func() {
    cerr.rdbuf()->pubsetbuf(buf, sizeof(buf));
    }'''

if meson.get_compiler('cpp').compiles(
    pubsetbuff_c,
    name : 'pubsetbuf'
  )
    config_h.set(
        'HAVE_PUBSETBUF', 1,
        description : 'Define to 1 if you have the `pubsetbuf` function.')
endif




# checking data types
#============================================================================


dirent_h_prefix = '''
  #include <sys/types.h>
  #include <dirent.h>
'''

if has_dirent_h and meson.get_compiler('cpp').has_member('struct dirent', 'd_type', prefix: dirent_h_prefix)
  config_h.set('HAVE_STRUCT_DIRENT_D_TYPE', 1)
endif


endif


# build
#============================================================================

conf = configure_file(
    output : 'config.h',
    configuration : config_h)

if not sys_name.contains('windows')
  install_headers(
    conf,
    install_dir : join_paths(includedir, 'nix'))
endif

if is_msvc
  add_project_arguments('-FI', 'config.h', language : 'cpp')
else
  add_project_arguments('-include', 'config.h', language : 'cpp')
endif
src_inc = [include_directories('.', 'src')]

if not sys_name.contains('windows')
  project_dirs = [
    'src',
    'scripts',
    'corepkgs',
    'misc',
    'doc',
    'tests'
  ]
else
  project_dirs = [
    'src',
  ]
endif

foreach dir : project_dirs
    subdir(dir)
endforeach

if sys_name.contains('windows')
  install_data('corepkgs/fetchurl.nix',                   install_dir : bindir+'/../nix/corepkgs')
  install_data('corepkgs/derivation.nix',                 install_dir : bindir+'/../nix/corepkgs')
  install_data('corepkgs/imported-drv-to-derivation.nix', install_dir : bindir+'/../nix/corepkgs')
  install_data('corepkgs/buildenv.nix',                   install_dir : bindir+'/../nix/corepkgs')
endif

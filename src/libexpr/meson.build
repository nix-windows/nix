# Nix lib expr build file
#============================================================================




# src files
#============================================================================

src_inc += include_directories('.', 'primops')

libexpr_src_files = [
    'flake/flake.cc',
    'flake/flakeref.cc',
    'flake/lockfile.cc',

    'primops/fetchMercurial.cc',
    'primops/fetchTree.cc',
    'primops/fromTOML.cc',

    'attr-path.cc',
    'attr-set.cc',
    'common-eval-args.cc',
    'eval-cache.cc',
    'eval.cc',
    'function-trace.cc',
    'get-drvs.cc',
    'json-to-value.cc',
    'nixexpr.cc',
    'primops.cc',
    'value-to-json.cc',
    'value-to-xml.cc',
]
if not build_on_windows
  libexpr_src_files += [
    'primops/context.cc',
  ]
endif
libexpr_src = files(libexpr_src_files)

libexpr_headers_files = [
    'flake/flake.hh',
    'flake/flakeref.hh',
    'flake/lockfile.hh',

    'attr-path.hh',
    'attr-set.hh',
    'common-eval-args.hh',
    'eval-cache.hh',
    'eval.hh',
    'eval-inline.hh',
    'get-drvs.hh',
    'json-to-value.hh',
    'nixexpr.hh',
    'primops.hh',
    'symbol-table.hh',
    'value.hh',
    'value-to-json.hh',
    'value-to-xml.hh',
]
if not build_on_windows
  libexpr_headers_files += [
    'function-trace.hh',
  ]
endif
libexpr_headers = files(libexpr_headers_files)



# dependancies
#============================================================================

libexpr_dep_list = [
    gc_dep,
    libdl_dep,
    libsodium_dep,
    boost_dep,
]

if sys_name == 'freebsd'
    libexpr_dep_list += libdl_dep
endif




# Link args
#============================================================================

libexpr_link_list = [
    libutil_lib,
    libstore_lib,
    libfetchers_lib,
    libmain_lib,
]

libexpr_link_args = []




# compiler args
#============================================================================

libexpr_cxx_args = []




# targets
#============================================================================

subdir('primops')
subdir('flake')

embeddded_files = [
  'imported-drv-to-derivation.nix',
]

foreach file : embeddded_files
  libexpr_src += custom_target(
    (file + '.gen.hh'),
    output : (file + '.gen.hh'),
    input : file,
    command : [bash, '-c', gen_header, 'sh', '@OUTPUT@'],
  )
endforeach


libexpr_src += custom_target(
    'parser_tab.[cchh]',
    output : ['parser-tab.cc', 'parser-tab.hh'],
    input : 'parser.y',
    command : [
        bison,
        '-v',
        '--output=@OUTPUT0@',
        '@INPUT@',
        '-d'])

libexpr_src += custom_target(
    'lexer_tab.[cchh]',
    output : ['lexer-tab.cc', 'lexer-tab.hh'],
    input : 'lexer.l',
    command : [
        flex,
        '--outfile=@OUTPUT0@',
        '--header-file=@OUTPUT1@',
        '@INPUT@'])




# build
#============================================================================

libexpr_lib = library(
    'nixexpr',
    install : get_option('default_library') != 'static',
    install_mode : 'rwxr-xr-x',
    install_dir : libdir,
    include_directories : src_inc,
    link_with : libexpr_link_list,
    sources : libexpr_src,
    link_args : libexpr_link_args,
    dependencies : libexpr_dep_list)

if not build_on_windows
  install_headers(
    libexpr_headers,
    install_dir : join_paths(includedir, 'nix'))
endif

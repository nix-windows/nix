# nix src build file
#============================================================================

src_dirs = [
  'libutil',
  'libstore',
  'libmain',
  'libexpr',
  'nix',
]
if not build_on_windows
  src_dirs += [
    'resolve-system-dependencies'
  ]
else
  src_dirs += [
    'nix-build',
    'nix-store',
    'nix-instantiate',
  ]
endif

foreach dir : src_dirs
    subdir(dir)
endforeach


if not build_on_windows
  libstore_config = pkg.generate(
    libstore_lib,
    libraries : [
        libutil_lib],
    version : meson.project_version(),
    name : 'Nix',
    subdirs : ['nix/'],
    filebase : 'nix-store',
    extra_cflags : '-std=c++17',
    description : 'Nix Package Manager.')

  libmain_config = pkg.generate(
    libmain_lib,
    version : meson.project_version(),
    name : 'Nix',
    subdirs : ['nix/'],
    filebase : 'nix-main',
    extra_cflags : '-std=c++17',
    description : 'Nix Package Manager.')

  libexpr_config = pkg.generate(
    libexpr_lib,
    libraries : [
        libstore_lib],
    version : meson.project_version(),
    name : 'Nix',
    subdirs : ['nix/'],
    filebase : 'nix-expr',
    extra_cflags : '-std=c++17',
    description : 'Nix Package Manager.')
endif

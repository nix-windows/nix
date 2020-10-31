@echo off


rem path to old nix (mingw's is ok)
set OLDNIX=C:\work\nix-bootstrap-golden

set NIX_BIN_DIR=%OLDNIX%\bin
set NIX_CONF_DIR=%OLDNIX%\etc
set NIX_DATA_DIR=%OLDNIX%\share

rem separate Nix Store without ACL experiments
set NIX_STORE_DIR=C:\nix2-data\store
set NIX_LOG_DIR=C:\nix2-data\var\log\nix
set NIX_STATE_DIR=C:\nix2-data\var\nix

set NIX_PATH=nixpkgs=C:\work\nixpkgs-windows


for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-stdenv-cc    -E "with (import <nixpkgs> { }); pkgsi686Windows.stdenv.cc"'              ) do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-stdenv-cc-cc -E "with (import <nixpkgs> { }); pkgsi686Windows.stdenv.cc.cc"'           ) do set STDENV_CC_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-boost        -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.boost   "'   ) do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-openssl      -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.openssl "'   ) do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-xz           -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.xz      "'   ) do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-bzip2        -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.bzip2   "'   ) do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-curl         -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.curl    "'   ) do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-sqlite       -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.sqlite3 "'   ) do set SQLITE=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-flex         -E "with (import <nixpkgs> { }); pkgsi686Windows. msysPacman.flex    "'   ) do set FLEX=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-bison        -E "with (import <nixpkgs> { }); pkgsi686Windows. msysPacman.bison   "'   ) do set BISON=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-gcc-meson        -E "with (import <nixpkgs> { }); pkgsi686Windows.mingwPacman.meson   "'   ) do set MESON=%%i
echo STDENV_CC=%STDENV_CC%
echo BOOST=%BOOST%
echo OPENSSL=%OPENSSL%
echo XZ=%XZ%
echo BZIP2=%BZIP2%
echo CURL=%CURL%
echo SQLITE=%SQLITE%
echo FLEX=%FLEX%
echo BISON=%BISON%
echo MESON=%MESON%


PATH=%STDENV_CC%\bin;%BISON%\usr\bin;%FLEX%\usr\bin;%PATH%

md                                       ..\builddir-gcc-32

    %MESON%\mingw32\bin\meson setup      ..\builddir-gcc-32 . --buildtype release --default-library static ^
                                                              -Dwith_boost=%BOOST%/mingw32 -Dwith_openssl=%OPENSSL%/mingw32 -Dwith_lzma=%XZ%/mingw32 -Dwith_bz2=%BZIP2%/mingw32 -Dwith_curl=%CURL%/mingw32 -Dwith_sqlite3=%SQLITE%/mingw32
rem %MESON%\mingw32\bin\meson compile -C ..\builddir-gcc-32 --clean
    %MESON%\mingw32\bin\meson compile -C ..\builddir-gcc-32
    %MESON%\mingw32\bin\meson install -C ..\builddir-gcc-32

rem remove garbage like downloaded .iso files
rem %OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize


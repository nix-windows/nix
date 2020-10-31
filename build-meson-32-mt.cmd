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

rem `--option system x86_64-windows` covers the case of 32-bit nix-build.exe (also, `pkgsCross.windows64.stdenv.cc` should allow to build 64-bit code on 32-bit Windows)

for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-stdenv-cc        -E "with (import <nixpkgs> { }); pkgsMsvc2019.stdenv.cc                                            "') do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-boost            -E "with (import <nixpkgs> { }); pkgsMsvc2019.boost174.override{ staticRuntime=true; static=true; }"') do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-openssl          -E "with (import <nixpkgs> { }); pkgsMsvc2019.openssl .override{ staticRuntime=true;              }"') do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-xz               -E "with (import <nixpkgs> { }); pkgsMsvc2019.xz      .override{ staticRuntime=true;              }"') do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-bzip2            -E "with (import <nixpkgs> { }); pkgsMsvc2019.bzip2   .override{ staticRuntime=true;              }"') do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-curl             -E "with (import <nixpkgs> { }); pkgsMsvc2019.curl    .override{ staticRuntime=true; zlib=pkgsMsvc2019.zlib.override{staticRuntime=true;}; openssl=pkgsMsvc2019.openssl.override{staticRuntime=true;}; }"') do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-sqlite           -E "with (import <nixpkgs> { }); pkgsMsvc2019.sqlite  .override{ staticRuntime=true;              }"') do set SQLITE=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-flex             -E "with (import <nixpkgs> { });  msysPacman.flex                                                  "') do set FLEX=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-bison            -E "with (import <nixpkgs> { });  msysPacman.bison                                                 "') do set BISON=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system i686-windows -o i686-meson            -E "with (import <nixpkgs> { }); mingwPacman.meson                                                 "') do set MESON=%%i
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

md                                       ..\builddir32-vs2019-mt

    %MESON%\mingw32\bin\meson setup      ..\builddir32-vs2019-mt . --backend vs2019 --default-library static --buildtype release ^
                                                                   -Db_vscrt=mt -Db_lto=true ^
                                                                   -Dwith_boost=%BOOST% -Dwith_openssl=%OPENSSL% -Dwith_lzma=%XZ% -Dwith_bz2=%BZIP2% -Dwith_curl=%CURL% -Dwith_sqlite3=%SQLITE%

rem %MESON%\mingw32\bin\meson compile -C ..\builddir32-vs2019-mt --clean
    %MESON%\mingw32\bin\meson compile -C ..\builddir32-vs2019-mt --verbose
    %MESON%\mingw32\bin\meson install -C ..\builddir32-vs2019-mt

rem remove garbage like downloaded .iso files
rem %OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize


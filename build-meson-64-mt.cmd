@echo off


rem path to old nix (mingw's is ok)
rem "build.cmd install" replaces OLDNIX
set OLDNIX=C:\work\nix-bootstrap

set NIX_BIN_DIR=%OLDNIX%\bin
set NIX_CONF_DIR=%OLDNIX%\etc
set NIX_DATA_DIR=%OLDNIX%\share

rem separate Nix Store without ACL experiments
set NIX_STORE_DIR=C:\nix2-data\store
set NIX_LOG_DIR=C:\nix2-data\var\log\nix
set NIX_STATE_DIR=C:\nix2-data\var\nix

set NIX_PATH=nixpkgs=C:\work\nixpkgs-windows

rem `--option system x86_64-windows` covers the case of 32-bit nix-build.exe (also, `pkgsCross.windows64.stdenv.cc` should allow to build 64-bit code on 32-bit Windows)

rem TODO: change stdenv to explicit stdenvVC2019

for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-stdenv-cc        -E "with (import <nixpkgs> { }); stdenv.cc                                            "') do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-boost            -E "with (import <nixpkgs> { }); boost172.override{ staticRuntime=true; static=true; }"') do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-openssl          -E "with (import <nixpkgs> { }); openssl .override{ staticRuntime=true;              }"') do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-xz               -E "with (import <nixpkgs> { }); xz      .override{ staticRuntime=true;              }"') do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-bzip2            -E "with (import <nixpkgs> { }); bzip2   .override{ staticRuntime=true;              }"') do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-curl             -E "with (import <nixpkgs> { }); curl    .override{ staticRuntime=true; zlib=zlib.override{staticRuntime=true;}; openssl=openssl.override{staticRuntime=true;}; }"') do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-sqlite           -E "with (import <nixpkgs> { }); sqlite  .override{ staticRuntime=true;              }"') do set SQLITE=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-flex             -E "with (import <nixpkgs> { });  msysPacman.flex                                     "') do set FLEX=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-bison            -E "with (import <nixpkgs> { });  msysPacman.bison                                    "') do set BISON=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-meson            -E "with (import <nixpkgs> { }); mingwPacman.meson                                    "') do set MESON=%%i
echo STDENV_CC=%STDENV_CC%
echo STDENV_CC_REDIST=%STDENV_CC_REDIST%
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

md                                       ..\builddir64-vs2019-64-mt

    %MESON%\mingw64\bin\meson setup      ..\builddir64-vs2019-64-mt . --backend vs2019 --default-library static --buildtype release ^
                                                                      -Db_vscrt=mt -Db_lto=true ^
                                                                      -Dwith_boost=%BOOST% -Dwith_openssl=%OPENSSL% -Dwith_lzma=%XZ% -Dwith_bz2=%BZIP2% -Dwith_curl=%CURL% -Dwith_sqlite3=%SQLITE%

rem %MESON%\mingw64\bin\meson compile -C ..\builddir64-vs2019-64-mt --clean
    %MESON%\mingw64\bin\meson compile -C ..\builddir64-vs2019-64-mt --verbose
    %MESON%\mingw64\bin\meson install -C ..\builddir64-vs2019-64-mt

rem remove garbage like downloaded .iso files
rem %OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize


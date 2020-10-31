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


for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-stdenv-cc    -E "with (import <nixpkgs> { }); stdenv.cc"'              ) do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-boost        -E "with (import <nixpkgs> { }); mingwPacman.boost   "'   ) do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-openssl      -E "with (import <nixpkgs> { }); mingwPacman.openssl "'   ) do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-xz           -E "with (import <nixpkgs> { }); mingwPacman.xz      "'   ) do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-bzip2        -E "with (import <nixpkgs> { }); mingwPacman.bzip2   "'   ) do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-curl         -E "with (import <nixpkgs> { }); mingwPacman.curl    "'   ) do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-sqlite       -E "with (import <nixpkgs> { }); mingwPacman.sqlite3 "'   ) do set SQLITE=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-flex         -E "with (import <nixpkgs> { });  msysPacman.flex    "'   ) do set FLEX=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-bison        -E "with (import <nixpkgs> { });  msysPacman.bison   "'   ) do set BISON=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed --option system x86_64-windows -o x86_64-gcc-meson        -E "with (import <nixpkgs> { }); mingwPacman.meson   "'   ) do set MESON=%%i
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

md                                       ..\builddir-gcc-64

    %MESON%\mingw64\bin\meson setup      ..\builddir-gcc-64 . --buildtype release --default-library static ^
                                                              -Dwith_boost=%BOOST%/mingw64 -Dwith_openssl=%OPENSSL%/mingw64 -Dwith_lzma=%XZ%/mingw64 -Dwith_bz2=%BZIP2%/mingw64 -Dwith_curl=%CURL%/mingw64 -Dwith_sqlite3=%SQLITE%/mingw64
rem %MESON%\mingw64\bin\meson compile -C ..\builddir-gcc-64 --clean
    %MESON%\mingw64\bin\meson compile -C ..\builddir-gcc-64
    %MESON%\mingw64\bin\meson install -C ..\builddir-gcc-64

rem remove garbage like downloaded .iso files
rem %OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize


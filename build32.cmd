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

rem for /f %%i in ('%OLDNIX%\bin\nix-build.exe --no-out-link -E "(import <nixpkgs> { }).msysPackages.coreutils"') do set COREUTILS=%%i
rem echo COREUTILS=%COREUTILS%
rem exit

for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-stdenv-cc -E "with (import <nixpkgs> { }).pkgsi686Windows; stdenv.cc"'                                 ) do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-boost     -E "with (import <nixpkgs> { }).pkgsi686Windows; boost  .override{ staticRuntime=true; }"'   ) do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-openssl   -E "with (import <nixpkgs> { }).pkgsi686Windows; openssl.override{ staticRuntime=true; }"'   ) do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-zlib      -E "with (import <nixpkgs> { }).pkgsi686Windows; zlib   .override{ staticRuntime=true; }"'   ) do set ZLIB=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-xz        -E "with (import <nixpkgs> { }).pkgsi686Windows; xz     .override{ staticRuntime=true; }"'   ) do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-bzip2     -E "with (import <nixpkgs> { }).pkgsi686Windows; bzip2  .override{ staticRuntime=true; }"'   ) do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-curl      -E "with (import <nixpkgs> { }).pkgsi686Windows; curl   .override{ staticRuntime=true; zlib=zlib.override{staticRuntime=true;}; openssl=openssl.override{staticRuntime=true;}; }"') do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o i686-sqlite    -E "with (import <nixpkgs> { }).pkgsi686Windows; sqlite .override{ staticRuntime=true; }"'   ) do set SQLITE=%%i
echo STDENV_CC=%STDENV_CC%
echo BOOST=%BOOST%
echo OPENSSL=%OPENSSL%
echo XZ=%XZ%
echo BZIP2=%BZIP2%
echo CURL=%CURL%
echo SQLITE=%SQLITE%


%STDENV_CC%\bin\nmake -f Makefile.win STDENV_CC=%STDENV_CC% BOOST=%BOOST% OPENSSL=%OPENSSL% XZ=%XZ% BZIP2=%BZIP2% CURL=%CURL% SQLITE=%SQLITE% %*

rem remove garbage like downloaded .iso files
rem %OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize


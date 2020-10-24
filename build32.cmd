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
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-stdenv-cc -E "(import <nixpkgs> { }).pkgsi686Windows.stdenv.cc"') do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-boost     -E "(import <nixpkgs> { }).pkgsi686Windows.boost"'    ) do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-openssl   -E "(import <nixpkgs> { }).pkgsi686Windows.openssl"'  ) do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-xz        -E "(import <nixpkgs> { }).pkgsi686Windows.xz"'       ) do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-bzip2     -E "(import <nixpkgs> { }).pkgsi686Windows.bzip2"'    ) do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-curl      -E "(import <nixpkgs> { }).pkgsi686Windows.curl"'     ) do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe -o i686-sqlite    -E "(import <nixpkgs> { }).pkgsi686Windows.sqlite"'   ) do set SQLITE=%%i
echo STDENV_CC=%STDENV_CC%
echo BOOST=%BOOST%
echo OPENSSL=%OPENSSL%
echo XZ=%XZ%
echo BZIP2=%BZIP2%
echo CURL=%CURL%
echo SQLITE=%SQLITE%

rem remove garbage like downloaded .iso files
%OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize

%STDENV_CC%\bin\nmake -f Makefile.win STDENV_CC=%STDENV_CC% BOOST=%BOOST% OPENSSL=%OPENSSL% XZ=%XZ% BZIP2=%BZIP2% CURL=%CURL% SQLITE=%SQLITE% %*

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

rem TODO: change stdenv to explicit stdenvVC2019
rem redist is for VCRUNTIME140D.DLL and UCRTBASED.DLL. to avoid "meson.build:13:0: ERROR: Executables created by cpp compiler cl are not runnable."
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-stdenv-cc        -E "with (import <nixpkgs> { }); stdenv.cc                        "') do set STDENV_CC=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-stdenv-cc-redist -E "with (import <nixpkgs> { }); stdenv.cc.redist                 "') do set STDENV_CC_REDIST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-boost            -E "with (import <nixpkgs> { }); boost172.override{ static=true; }"') do set BOOST=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-openssl          -E "with (import <nixpkgs> { }); openssl                          "') do set OPENSSL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-zlib             -E "with (import <nixpkgs> { }); zlib                             "') do set ZLIB=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-xz               -E "with (import <nixpkgs> { }); xz                               "') do set XZ=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-bzip2            -E "with (import <nixpkgs> { }); bzip2                            "') do set BZIP2=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-curl             -E "with (import <nixpkgs> { }); curl                             "') do set CURL=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-sqlite           -E "with (import <nixpkgs> { }); sqlite                           "') do set SQLITE=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-flex             -E "with (import <nixpkgs> { });  msysPacman.flex                 "') do set FLEX=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-bison            -E "with (import <nixpkgs> { });  msysPacman.bison                "') do set BISON=%%i
for /f %%i in ('%OLDNIX%\bin\nix-build.exe --keep-failed -o x86_64-meson            -E "with (import <nixpkgs> { }); mingwPacman.meson                "') do set MESON=%%i
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


PATH=%STDENV_CC%\bin;%STDENV_CC_REDIST%\x64\Microsoft.VC142.DebugCRT;%STDENV_CC_REDIST%\x64\Microsoft.UniversalCRT.Debug;%BISON%\usr\bin;%FLEX%\usr\bin;%PATH%

md                                       ..\builddir64-vs2019-64-md

    %MESON%\mingw64\bin\meson setup      ..\builddir64-vs2019-64-md . --backend vs2019 --default-library static --buildtype release
rem --optimization 2

rem %MESON%\mingw64\bin\meson compile -C ..\builddir64-vs2019-64-md --clean
    %MESON%\mingw64\bin\meson compile -C ..\builddir64-vs2019-64-md --verbose
rem %MESON%\mingw64\bin\meson install -C ..\builddir64-vs2019-64-md

rem remove garbage like downloaded .iso files
rem %OLDNIX%\bin\nix-store.exe --gc
rem %OLDNIX%\bin\nix-store.exe --optimize


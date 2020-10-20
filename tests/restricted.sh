source common.sh

clearStore

nix-instantiate --restrict-eval --eval -E '1 + 2'
(! nix-instantiate --restrict-eval ./restricted.nix)
(! nix-instantiate --eval --restrict-eval <(echo '1 + 2'))
nix-instantiate --restrict-eval ./simple.nix -I src=.
nix-instantiate --restrict-eval ./simple.nix -I src1=simple.nix -I src2=config.nix -I src3=./simple.builder.sh

(! nix-instantiate --restrict-eval --eval -E 'builtins.readFile ./simple.nix')
nix-instantiate --restrict-eval --eval -E 'builtins.readFile ./simple.nix' -I src=..

(! nix-instantiate --restrict-eval --eval -E 'builtins.readDir ../src/nix-channel')
nix-instantiate --restrict-eval --eval -E 'builtins.readDir ../src/nix-channel' -I src=../src

(! nix-instantiate --restrict-eval --eval -E 'let __nixPath = [ { prefix = "foo"; path = ./.; } ]; in <foo>')
nix-instantiate --restrict-eval --eval -E 'let __nixPath = [ { prefix = "foo"; path = ./.; } ]; in <foo>' -I src=.

if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    p=$(nix eval --raw --expr "builtins.fetchurl file://$(cygpath -m $(pwd))/restricted.sh" --impure --restrict-eval --allowed-uris "file://$(cygpath -m $(pwd))")
else
    p=$(nix eval --raw --expr "builtins.fetchurl file://$(pwd)/restricted.sh" --impure --restrict-eval --allowed-uris "file://$(pwd)")
fi
cmp $p restricted.sh

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    (! nix eval --raw "(builtins.fetchurl file://$(cygpath -m $(pwd))/restricted.sh)" --restrict-eval)
else
    (! nix eval --raw "(builtins.fetchurl file://$(pwd)/restricted.sh)" --restrict-eval)
fi
||||||| merged common ancestors
(! nix eval --raw "(builtins.fetchurl file://$(pwd)/restricted.sh)" --restrict-eval)
=======
(! nix eval --raw --expr "builtins.fetchurl file://$(pwd)/restricted.sh" --impure --restrict-eval)
>>>>>>> meson

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    (! nix eval --raw "(builtins.fetchurl file://$(cygpath -m $(pwd))/restricted.sh)" --restrict-eval --allowed-uris "file://$(cygpath -m $(pwd))/restricted.sh/")
else
    (! nix eval --raw "(builtins.fetchurl file://$(pwd)/restricted.sh)" --restrict-eval --allowed-uris "file://$(pwd)/restricted.sh/")
fi
||||||| merged common ancestors
(! nix eval --raw "(builtins.fetchurl file://$(pwd)/restricted.sh)" --restrict-eval --allowed-uris "file://$(pwd)/restricted.sh/")
=======
(! nix eval --raw --expr "builtins.fetchurl file://$(pwd)/restricted.sh" --impure --restrict-eval --allowed-uris "file://$(pwd)/restricted.sh/")
>>>>>>> meson

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    nix eval --raw "(builtins.fetchurl file://$(cygpath -m $(pwd))/restricted.sh)" --restrict-eval --allowed-uris "file://$(cygpath -m $(pwd))/restricted.sh"
else
    nix eval --raw "(builtins.fetchurl file://$(pwd)/restricted.sh)" --restrict-eval --allowed-uris "file://$(pwd)/restricted.sh"
fi
||||||| merged common ancestors
nix eval --raw "(builtins.fetchurl file://$(pwd)/restricted.sh)" --restrict-eval --allowed-uris "file://$(pwd)/restricted.sh"
=======
nix eval --raw --expr "builtins.fetchurl file://$(pwd)/restricted.sh" --impure --restrict-eval --allowed-uris "file://$(pwd)/restricted.sh"
>>>>>>> meson

(! nix eval --raw --expr "builtins.fetchurl https://github.com/NixOS/patchelf/archive/master.tar.gz" --impure --restrict-eval)
(! nix eval --raw --expr "builtins.fetchTarball https://github.com/NixOS/patchelf/archive/master.tar.gz" --impure --restrict-eval)
(! nix eval --raw --expr "fetchGit git://github.com/NixOS/patchelf.git" --impure --restrict-eval)

if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    nix ln $(pwd)/restricted.nix $TEST_ROOT/restricted.nix
else
    ln -sfn $(pwd)/restricted.nix $TEST_ROOT/restricted.nix
fi

[[ $(nix-instantiate --eval $TEST_ROOT/restricted.nix) == 3 ]]
(! nix-instantiate --eval --restrict-eval $TEST_ROOT/restricted.nix)
(! nix-instantiate --eval --restrict-eval $TEST_ROOT/restricted.nix -I $TEST_ROOT)
(! nix-instantiate --eval --restrict-eval $TEST_ROOT/restricted.nix -I .)
nix-instantiate --eval --restrict-eval $TEST_ROOT/restricted.nix -I $TEST_ROOT -I .

[[ $(nix eval --raw --impure --restrict-eval -I . --expr 'builtins.readFile "${import ./simple.nix}/hello"') == 'Hello World!' ]]

# Check whether we can leak symlink information through directory traversal.
<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    echo "this fails on MSYS Windows<->Unix path translation"
else
    traverseDir="$(pwd)/restricted-traverse-me"
    ln -sfn "$(pwd)/restricted-secret" "$(pwd)/restricted-innocent"
    mkdir -p "$traverseDir"
    goUp="..$(echo "$traverseDir" | sed -e 's,[^/]\+,..,g')"
    output="$(nix eval --raw --restrict-eval -I "$traverseDir" \
        "(builtins.readFile \"$traverseDir/$goUp$(pwd)/restricted-innocent\")" \
        2>&1 || :)"
    echo "$output" | grep "is forbidden"
    ! echo "$output" | grep -F restricted-secret
fi
||||||| merged common ancestors
traverseDir="$(pwd)/restricted-traverse-me"
ln -sfn "$(pwd)/restricted-secret" "$(pwd)/restricted-innocent"
mkdir -p "$traverseDir"
goUp="..$(echo "$traverseDir" | sed -e 's,[^/]\+,..,g')"
output="$(nix eval --raw --restrict-eval -I "$traverseDir" \
    "(builtins.readFile \"$traverseDir/$goUp$(pwd)/restricted-innocent\")" \
    2>&1 || :)"
echo "$output" | grep "is forbidden"
! echo "$output" | grep -F restricted-secret
=======
traverseDir="$(pwd)/restricted-traverse-me"
ln -sfn "$(pwd)/restricted-secret" "$(pwd)/restricted-innocent"
mkdir -p "$traverseDir"
goUp="..$(echo "$traverseDir" | sed -e 's,[^/]\+,..,g')"
output="$(nix eval --raw --restrict-eval -I "$traverseDir" \
    --expr "builtins.readFile \"$traverseDir/$goUp$(pwd)/restricted-innocent\"" \
    2>&1 || :)"
echo "$output" | grep "is forbidden"
! echo "$output" | grep -F restricted-secret
>>>>>>> meson

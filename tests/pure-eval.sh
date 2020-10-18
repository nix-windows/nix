source common.sh

clearStore

nix eval --expr 'assert 1 + 2 == 3; true'

[[ $(nix eval --impure --expr 'builtins.readFile ./pure-eval.sh') =~ clearStore ]]

(! nix eval --expr 'builtins.readFile ./pure-eval.sh')

(! nix eval --expr builtins.currentTime)
(! nix eval --expr builtins.currentSystem)

(! nix-instantiate --pure-eval ./simple.nix)

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    [[ $(nix eval "((import (builtins.fetchurl { url = file://$(cygpath -m $(pwd))/pure-eval.nix; })).x)") == 123 ]]
    (! nix eval --pure-eval "((import (builtins.fetchurl { url = file://$(cygpath -m $(pwd))/pure-eval.nix; })).x)")
    nix eval --pure-eval "((import (builtins.fetchurl { url = file://$(cygpath -m $(pwd))/pure-eval.nix; sha256 = \"$(nix hash-file pure-eval.nix --type sha256)\"; })).x)"
else
    [[ $(nix eval "((import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; })).x)") == 123 ]]
    (! nix eval --pure-eval "((import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; })).x)")
    nix eval --pure-eval "((import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; sha256 = \"$(nix hash-file pure-eval.nix --type sha256)\"; })).x)"
fi
||||||| merged common ancestors
[[ $(nix eval "((import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; })).x)") == 123 ]]
(! nix eval --pure-eval "((import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; })).x)")
nix eval --pure-eval "((import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; sha256 = \"$(nix hash-file pure-eval.nix --type sha256)\"; })).x)"
=======
[[ $(nix eval --impure --expr "(import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; })).x") == 123 ]]
(! nix eval --expr "(import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; })).x")
nix eval --expr "(import (builtins.fetchurl { url = file://$(pwd)/pure-eval.nix; sha256 = \"$(nix hash-file pure-eval.nix --type sha256)\"; })).x"
>>>>>>> meson

source common.sh

drvPath=$(nix-instantiate simple.nix)

test "$(nix-store -q --binding system "$drvPath")" = "$system"

echo "derivation is $drvPath"

outPath=$(nix-store -rvv "$drvPath")

echo "output path is $outPath"

(! [ -w $outPath ])

text=$(cat "$outPath"/hello)
if test "$text" != "Hello World!"; then exit 1; fi

# Directed delete: $outPath is not reachable from a root, so it should
# be deleteable.
nix-store --delete $outPath
(! [ -e $outPath/hello ])

if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
  # msys is able to see "/foo" as path and magically translate it to Windows path, but not "\&real=$TEST_ROOT/real-store"
  # also ':' beina the delimiter is to be escaped
  outPath="$(NIX_REMOTE=local?store=/foo\&real=$(cygpath -m $TEST_ROOT/real-store | sed 's,:,%3A,g') nix-instantiate --readonly-mode hash-check.nix)"
  if test "$(cygpath -u "$outPath")" != "/foo/lch1b3153ax6jy3snx89p5mgsg6a6fg1-dependencies.drv"; then
    echo "hashDerivationModulo appears broken, got $outPath"
    exit 1
  fi
else
  outPath="$(NIX_REMOTE=local?store=/foo\&real=$TEST_ROOT/real-store nix-instantiate --readonly-mode hash-check.nix)"
  if test "$outPath" != "/foo/lfy1s6ca46rm5r6w4gg9hc0axiakjcnm-dependencies.drv"; then
    echo "hashDerivationModulo appears broken, got $outPath"
    exit 1
  fi
fi

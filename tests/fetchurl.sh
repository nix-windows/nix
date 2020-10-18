source common.sh

clearStore

# Test fetching a flat file.
hash=$(nix-hash --flat --type sha256 ./fetchurl.sh)

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(cygpath -m $(pwd))/fetchurl.sh --argstr sha256 $hash --no-out-link --hashed-mirrors '')
else
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr sha256 $hash --no-out-link --hashed-mirrors '')
fi

||||||| merged common ancestors
outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr sha256 $hash --no-out-link --hashed-mirrors '')
=======
outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr sha256 $hash --no-out-link)
>>>>>>> meson

cmp $outPath fetchurl.sh

# Now using a base-64 hash.
clearStore

hash=$(nix hash-file --type sha512 --base64 ./fetchurl.sh)

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(cygpath -m $(pwd))/fetchurl.sh --argstr sha512 $hash --no-out-link --hashed-mirrors '')
else
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr sha512 $hash --no-out-link --hashed-mirrors '')
fi
||||||| merged common ancestors
outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr sha512 $hash --no-out-link --hashed-mirrors '')
=======
outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr sha512 $hash --no-out-link)
>>>>>>> meson

cmp $outPath fetchurl.sh

# Now using an SRI hash.
clearStore

hash=$(nix hash-file ./fetchurl.sh)

[[ $hash =~ ^sha256- ]]

outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(pwd)/fetchurl.sh --argstr hash $hash --no-out-link)

cmp $outPath fetchurl.sh

# Test that we can substitute from a different store dir.
clearStore

other_store=file://$TEST_ROOT/other_store?store=/fnord/store

hash=$(nix hash-file --type sha256 --base16 ./fetchurl.sh)

<<<<<<< HEAD
mirror=$TMPDIR/hashed-mirror
rm -rf $mirror
mkdir -p $mirror/sha512
||||||| merged common ancestors
mirror=$TMPDIR/hashed-mirror
rm -rf $mirror
mkdir -p $mirror/sha512
ln -s $(pwd)/fetchurl.sh $mirror/sha512/$hash32
=======
storePath=$(nix --store $other_store add-to-store --flat ./fetchurl.sh)
>>>>>>> meson

<<<<<<< HEAD
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    nix ln $(pwd)/fetchurl.sh $mirror/sha512/$hash32
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file:///no-such-dir/fetchurl.sh --argstr sha512 $hash --no-out-link --hashed-mirrors "file://$(cygpath -m $mirror)")
else
    ln -s $(pwd)/fetchurl.sh $mirror/sha512/$hash32
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file:///no-such-dir/fetchurl.sh --argstr sha512 $hash --no-out-link --hashed-mirrors "file://$mirror")
fi
||||||| merged common ancestors
outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file:///no-such-dir/fetchurl.sh --argstr sha512 $hash --no-out-link --hashed-mirrors "file://$mirror")
=======
outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file:///no-such-dir/fetchurl.sh --argstr sha256 $hash --no-out-link --substituters $other_store)
>>>>>>> meson

# Test hashed mirrors with an SRI hash.
nix-build '<nix/fetchurl.nix>' --argstr url file:///no-such-dir/fetchurl.sh --argstr hash $(nix to-sri --type sha256 $hash) \
          --no-out-link --substituters $other_store

# Test unpacking a NAR.
rm -rf $TEST_ROOT/archive
mkdir -p $TEST_ROOT/archive
cp ./fetchurl.sh $TEST_ROOT/archive
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    echo "no executable bit on Windows"
    nix ln foo $TEST_ROOT/archive/symlink
else
    chmod +x $TEST_ROOT/archive/fetchurl.sh
    ln -s foo $TEST_ROOT/archive/symlink
fi
nar=$TEST_ROOT/archive.nar
nix-store --dump $TEST_ROOT/archive > $nar

hash=$(nix-hash --flat --type sha256 $nar)

if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(cygpath -m $nar) --argstr sha256 $hash \
          --arg unpack true --argstr name xyzzy --no-out-link)
else
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$nar --argstr sha256 $hash \
          --arg unpack true --argstr name xyzzy --no-out-link)
fi

echo $outPath | grep -q 'xyzzy'

if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    echo "no executable bit on Windows"
else
    test -x $outPath/fetchurl.sh
fi
test -L $outPath/symlink

nix-store --delete $outPath

# Test unpacking a compressed NAR.
narxz=$TEST_ROOT/archive.nar.xz
rm -f $narxz
xz --keep $nar
if [[ "$(uname)" =~ ^MINGW|^MSYS ]]; then
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$(cygpath -m $narxz) --argstr sha256 $hash \
          --arg unpack true --argstr name xyzzy --no-out-link)

    echo "no executable bit on Windows"
else
    outPath=$(nix-build '<nix/fetchurl.nix>' --argstr url file://$narxz --argstr sha256 $hash \
          --arg unpack true --argstr name xyzzy --no-out-link)

    test -x $outPath/fetchurl.sh
fi

test -L $outPath/symlink

{ useClang ? false, crossSystem ? null }:

let
  pkgsSrc = builtins.fetchTarball https://github.com/NixOS/nixpkgs/archive/bb1013511e1e5edcf314df8321acf2f3c536df0d.tar.gz) {};
in

with import pkgsSrc { inherit crossSystem; };

with import ./release-common.nix { inherit pkgs; };

(if useClang then clangStdenv else stdenv).mkDerivation {
  name = "nix";

  nativeBuildInputs = nativeBuildDeps ++ tarballDeps ++ [ pkgs.rustfmt ];

  buildInputs = buildDeps ++ perlDeps;

  inherit mesonFlags configureFlags;

  enableParallelBuilding = true;

  installFlags = "sysconfdir=$(out)/etc";

  shellHook =
    ''
      export prefix=$(pwd)/inst
      configureFlags+=" --prefix=$prefix"
      PKG_CONFIG_PATH=$prefix/lib/pkgconfig:$PKG_CONFIG_PATH
      PATH=$prefix/bin:$PATH
    '';
}

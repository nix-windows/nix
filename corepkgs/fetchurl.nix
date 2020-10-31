{ system ? "" # obsolete
, urls ? [url]
, url ? builtins.head urls
, md5 ? "", sha1 ? "", sha256 ? "", sha512 ? ""
, outputHash ?
    if sha512 != "" then sha512 else if sha1 != "" then sha1 else if md5 != "" then md5 else sha256
, outputHashAlgo ?
    if sha512 != "" then "sha512" else if sha1 != "" then "sha1" else if md5 != "" then "md5" else "sha256"
, executable ? false
, unpack ? false
, name ? baseNameOf (toString (builtins.head urls))
}:

derivation {
  builder = "builtin:fetchurl";

  # New-style output content requirements.
  inherit outputHashAlgo outputHash;
  outputHashMode = if unpack || executable then "recursive" else "flat";

  inherit name urls executable unpack;

  system = "builtin";

  # No need to double the amount of network traffic
  preferLocalBuild = true;

  impureEnvVars = [
    # We borrow these environment variables from the caller to allow
    # easy proxy configuration.  This is impure, but a fixed-output
    # derivation like fetchurl is allowed to do so since its result is
    # by definition pure.
    "http_proxy" "https_proxy" "ftp_proxy" "all_proxy" "no_proxy"
  ];

  # to support older nix.exe
  url = builtins.head urls;
}

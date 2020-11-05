ifeq (MINGW,$(findstring MINGW,$(OS)))

# these do not pass (yet) on MINGW:

#  gc-concurrent.sh         <-- fails trying to delete open .lock-file, need runtimeRoots
#  gc-runtime.sh            <-- runtimeRoots not implemented yet
#  user-envs.sh             <-- nix-env is not implemented yet
#  remote-store.sh          <-- not implemented yet
#  secure-drv-outputs.sh    <-- needs nix-daemin which is not ported yet
#  nix-channel.sh           <-- nix-channel is not implemented yet
#  nix-profile.sh           <-- not implemented yet
#  case-hack.sh             <-- not implemented yet (it might have Windows-specific)
#  nix-shell.sh             <-- not implemented yet
#  linux-sandbox.sh         <-- not implemented (Docker can be use on Windows for sandboxing)
#  plugins.sh               <-- not implemented yet
#  nix-copy-ssh.sh          <-- not implemented yet
#  build-remote.sh          <-- not implemented yet
#  binary-cache.sh          <-- \* does not work in MSYS Bash (https://superuser.com/questions/897599/escaping-asterisk-in-bash-on-windows)
#  nar-access.sh            <-- not possible to have args '/foo/data' (paths inside nar) without magic msys path translation (maybe `bash -c '...'` will work?)
endif

nix_tests = \
  hash.sh lang.sh add.sh simple.sh dependencies.sh \
  config.sh \
  gc.sh \
  gc-concurrent.sh \
  gc-auto.sh \
  referrers.sh user-envs.sh logging.sh nix-build.sh misc.sh fixed.sh \
  gc-runtime.sh check-refs.sh filter-source.sh \
  local-store.sh remote-store.sh export.sh export-graph.sh \
  timeout.sh secure-drv-outputs.sh nix-channel.sh \
  multiple-outputs.sh import-derivation.sh fetchurl.sh optimise-store.sh \
  binary-cache.sh nix-profile.sh repair.sh dump-db.sh case-hack.sh \
  check-reqs.sh pass-as-file.sh tarball.sh restricted.sh \
  placeholders.sh nix-shell.sh \
  linux-sandbox.sh \
  build-dry.sh \
  build-remote-input-addressed.sh \
  ssh-relay.sh \
  nar-access.sh \
  structured-attrs.sh \
  fetchGit.sh \
  fetchGitRefs.sh \
  fetchGitSubmodules.sh \
  fetchMercurial.sh \
  signing.sh \
  shell.sh \
  brotli.sh \
  pure-eval.sh \
  check.sh \
  plugins.sh \
  search.sh \
  nix-copy-ssh.sh \
  post-hook.sh \
  function-trace.sh \
  recursive.sh \
  describe-stores.sh \
  flakes.sh \
  content-addressed.sh
  # parallel.sh
  # build-remote-content-addressed-fixed.sh \

install-tests += $(foreach x, $(nix_tests), tests/$(x))

tests-environment = NIX_REMOTE= $(bash) -e

clean-files += $(d)/common.sh $(d)/config.nix

test-deps += tests/common.sh tests/config.nix
ifneq (MINGW,$(findstring MINGW,$(OS)))
test-deps += tests/plugins/libplugintest.$(SO_EXT)
endif

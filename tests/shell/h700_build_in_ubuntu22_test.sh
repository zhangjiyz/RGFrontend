#!/bin/sh
set -eu

SCRIPT="H700/build_in_ubuntu22.sh"
ROOT="${TMPDIR:-/tmp}/rgfrontend_h700_builder_test"
rm -rf "$ROOT"
mkdir -p "$ROOT/sysroot/usr/include" "$ROOT/bin"

FAKE_RUNTIME="$ROOT/bin/fake-container-runtime"
cat >"$FAKE_RUNTIME" <<'EOS'
#!/bin/sh
printf '%s\n' "$*" >>"$MPL_FAKE_RUNTIME_LOG"
case "$1" in
  image)
    [ "$2" = "inspect" ] || exit 64
    [ "${MPL_FAKE_IMAGE_EXISTS:-1}" = "1" ]
    ;;
  build)
    exit 0
    ;;
  run)
    exit 0
    ;;
  *)
    exit 64
    ;;
esac
EOS
chmod 755 "$FAKE_RUNTIME"

run_builder() {
  MPL_CONTAINER_RUNTIME="$FAKE_RUNTIME" \
  MPL_FAKE_RUNTIME_LOG="$ROOT/runtime.log" \
  MPL_H700_SYSROOT="$ROOT/sysroot" \
  MPL_H700_DOCKER_CONTEXT="$ROOT/docker-context" \
  MPL_H700_BUILDER_IMAGE="rgfrontend-test-builder:ubuntu22" \
  "$@"
}

: >"$ROOT/runtime.log"
run_builder env MPL_H700_PREPARE_BUILDER_ONLY=1 MPL_FAKE_IMAGE_EXISTS=1 sh "$SCRIPT"
grep -q '^image inspect rgfrontend-test-builder:ubuntu22$' "$ROOT/runtime.log"
! grep -q '^build ' "$ROOT/runtime.log"
! grep -q '^run ' "$ROOT/runtime.log"

: >"$ROOT/runtime.log"
run_builder env MPL_H700_PREPARE_BUILDER_ONLY=1 MPL_FAKE_IMAGE_EXISTS=0 sh "$SCRIPT"
grep -q '^image inspect rgfrontend-test-builder:ubuntu22$' "$ROOT/runtime.log"
grep -q '^build ' "$ROOT/runtime.log"
grep -q -- "$ROOT/docker-context" "$ROOT/runtime.log"
[ -d "$ROOT/docker-context" ]
! grep -q '^run ' "$ROOT/runtime.log"

: >"$ROOT/runtime.log"
run_builder env MPL_FAKE_IMAGE_EXISTS=1 sh "$SCRIPT"
grep -q '^image inspect rgfrontend-test-builder:ubuntu22$' "$ROOT/runtime.log"
! grep -q '^build ' "$ROOT/runtime.log"
grep -q '^run ' "$ROOT/runtime.log"

rm -rf "$ROOT"

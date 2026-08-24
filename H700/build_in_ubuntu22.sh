#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BASE_IMAGE="${MPL_H700_BASE_IMAGE:-ubuntu:22.04}"
BUILDER_IMAGE="${MPL_H700_BUILDER_IMAGE:-mpl-h700-builder:ubuntu22}"
RUNTIME="${MPL_CONTAINER_RUNTIME:-}"
SYSROOT="${MPL_H700_SYSROOT:-$REPO_ROOT/H700/sysroot}"
DOCKERFILE="$SCRIPT_DIR/Dockerfile.ubuntu22"
DOCKER_CONTEXT="${MPL_H700_DOCKER_CONTEXT:-$REPO_ROOT/build/h700-docker-context}"
PREPARE_ONLY="${MPL_H700_PREPARE_BUILDER_ONLY:-0}"

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

[ -d "$SYSROOT/usr/include" ] || fail "sysroot is missing usr/include: $SYSROOT"

if [ -z "$RUNTIME" ]; then
  if command -v docker >/dev/null 2>&1; then
    RUNTIME="docker"
  elif command -v podman >/dev/null 2>&1; then
    RUNTIME="podman"
  else
    fail "docker or podman is required for the Ubuntu 22.04 build path"
  fi
fi

if [ "${MPL_H700_REBUILD_BUILDER:-0}" = "1" ] ||
   ! "$RUNTIME" image inspect "$BUILDER_IMAGE" >/dev/null 2>&1; then
  printf 'building cached H700 builder image %s from %s with %s\n' \
    "$BUILDER_IMAGE" "$BASE_IMAGE" "$RUNTIME"
  mkdir -p "$DOCKER_CONTEXT"
  "$RUNTIME" build \
    -f "$DOCKERFILE" \
    --build-arg "BASE_IMAGE=$BASE_IMAGE" \
    -t "$BUILDER_IMAGE" \
    "$DOCKER_CONTEXT"
else
  printf 'using cached H700 builder image %s with %s\n' "$BUILDER_IMAGE" "$RUNTIME"
fi

if [ "$PREPARE_ONLY" = "1" ]; then
  printf 'H700 builder image is ready: %s\n' "$BUILDER_IMAGE"
  exit 0
fi

printf 'building H700 package in %s with %s\n' "$BUILDER_IMAGE" "$RUNTIME"

"$RUNTIME" run --rm \
  -v "$REPO_ROOT:/work" \
  -w /work \
  -e MPL_H700_SYSROOT=/work/H700/sysroot \
  -e MPL_H700_VERSION="${MPL_H700_VERSION:-1.0.1}" \
  -e MPL_H700_APP_ID="${MPL_H700_APP_ID:-RGFrontend}" \
  -e MPL_H700_ENTRY_NAME="${MPL_H700_ENTRY_NAME:-RGFrontend}" \
  -e MPL_H700_BINARY_NAME="${MPL_H700_BINARY_NAME:-mpl_h700_frontend}" \
  -e MPL_HOST_UID="$(id -u)" \
  -e MPL_HOST_GID="$(id -g)" \
  "$BUILDER_IMAGE" \
  sh -c '
set -eu
sh H700/build_app.sh
[ -d build/h700 ] && chown -R "$MPL_HOST_UID:$MPL_HOST_GID" build/h700 || true
[ -d H700/dist_app ] && chown -R "$MPL_HOST_UID:$MPL_HOST_GID" H700/dist_app || true
'

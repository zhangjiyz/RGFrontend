#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
APP_ID="${MPL_H700_APP_ID:-RGFrontend}"
ENTRY_NAME="${MPL_H700_ENTRY_NAME:-RGFrontend}"
BINARY_NAME="${MPL_H700_BINARY_NAME:-mpl_h700_frontend}"
STAGE_ROOT="${MPL_H700_STAGE_ROOT:-$SCRIPT_DIR/dist_app/release_stage}"
TARGET="${MPL_H700_TARGET:-root@10.1.1.233}"
REMOTE_ROOT="${MPL_H700_REMOTE_ROOT:-/mnt/mmc}"
DEPLOY_CHMOD="${MPL_H700_DEPLOY_CHMOD:-0}"
REMOTE_APPS="$REMOTE_ROOT/Roms/APPS"
REMOTE_IMGS="$REMOTE_APPS/Imgs"

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

local_hash() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

write_local_manifest() {
  runtime_root="$STAGE_ROOT/Roms/APPS/$APP_ID"
  (
    cd "$runtime_root"
    find . -type f | sort | while IFS= read -r file; do
      rel="$APP_ID/${file#./}"
      printf '%s\t%s\n' "$(local_hash "$file")" "$rel"
    done
  )
  printf '%s\t%s\n' \
    "$(local_hash "$STAGE_ROOT/Roms/APPS/$ENTRY_NAME.sh")" \
    "$ENTRY_NAME.sh"
  printf '%s\t%s\n' \
    "$(local_hash "$STAGE_ROOT/Roms/APPS/Imgs/$ENTRY_NAME.png")" \
    "Imgs/$ENTRY_NAME.png"
}

remote_manifest() {
  ssh "$TARGET" "
    mkdir -p '$REMOTE_APPS' '$REMOTE_IMGS'
    cd '$REMOTE_APPS' || exit 1
    command -v sha256sum >/dev/null 2>&1 || exit 0
    {
      [ -d '$APP_ID' ] && find '$APP_ID' -type f
      [ -f '$ENTRY_NAME.sh' ] && printf '%s\n' '$ENTRY_NAME.sh'
      [ -f 'Imgs/$ENTRY_NAME.png' ] && printf '%s\n' 'Imgs/$ENTRY_NAME.png'
    } | sort | while IFS= read -r file; do
      sha256sum \"\$file\" | awk -v f=\"\$file\" '{print \$1 \"\t\" f}'
    done
  "
}

local_path_for_rel() {
  case "$1" in
    "$APP_ID"/*)
      printf '%s\n' "$STAGE_ROOT/Roms/APPS/$1"
      ;;
    *)
      printf '%s\n' "$STAGE_ROOT/Roms/APPS/$1"
      ;;
  esac
}

[ -d "$STAGE_ROOT/Roms/APPS/$APP_ID" ] || fail "runtime package not found; run H700/build_app.sh first"
[ -f "$STAGE_ROOT/Roms/APPS/$ENTRY_NAME.sh" ] || fail "entry script not found; run H700/build_app.sh first"
[ -f "$STAGE_ROOT/Roms/APPS/Imgs/$ENTRY_NAME.png" ] || fail "entry icon not found; run H700/build_app.sh first"

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/rgfrontend-deploy.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM
LOCAL_MANIFEST="$TMP_DIR/local.manifest"
REMOTE_MANIFEST="$TMP_DIR/remote.manifest"
CHANGED_LIST="$TMP_DIR/changed.list"

write_local_manifest >"$LOCAL_MANIFEST"
remote_manifest >"$REMOTE_MANIFEST"
: >"$CHANGED_LIST"

while IFS= read -r line; do
  rel="${line#*	}"
  if grep -Fqx "$line" "$REMOTE_MANIFEST"; then
    printf 'skip unchanged: %s\n' "$rel"
  else
    printf '%s\n' "$rel" >>"$CHANGED_LIST"
  fi
done <"$LOCAL_MANIFEST"

if [ -s "$CHANGED_LIST" ]; then
  REMOTE_DIRS="'$REMOTE_APPS' '$REMOTE_IMGS'"
  while IFS= read -r rel; do
    remote_dir="$(dirname -- "$REMOTE_APPS/$rel")"
    REMOTE_DIRS="$REMOTE_DIRS '$remote_dir'"
  done <"$CHANGED_LIST"
  ssh "$TARGET" "mkdir -p $REMOTE_DIRS"

  while IFS= read -r rel; do
    local_path="$(local_path_for_rel "$rel")"
    printf 'copy changed: %s\n' "$rel"
    scp "$local_path" "$TARGET:$REMOTE_APPS/$rel"
  done <"$CHANGED_LIST"
else
  printf 'all deployed files unchanged\n'
fi

if [ "$DEPLOY_CHMOD" = "1" ]; then
  ssh "$TARGET" "chmod 755 '$REMOTE_APPS/$ENTRY_NAME.sh' '$REMOTE_APPS/$APP_ID/run_frontend.sh' '$REMOTE_APPS/$APP_ID/launch_request.sh' '$REMOTE_APPS/$APP_ID/prepare_ra_launcher.sh' '$REMOTE_APPS/$APP_ID/check_system_immutability.sh' '$REMOTE_APPS/$APP_ID/$BINARY_NAME' && chmod 644 '$REMOTE_IMGS/$ENTRY_NAME.png'"
else
  printf 'skipped remote chmod; set MPL_H700_DEPLOY_CHMOD=1 to force permission repair\n'
fi

printf 'deployed to %s:%s\n' "$TARGET" "$REMOTE_APPS"

#!/bin/sh
set -eu

SOURCE="${MPL_H700_RA_LAUNCHER_SOURCE:-/mnt/mod/ctrl/RA_launch.sh}"
TARGET="${MPL_H700_MPL_RA_LAUNCHER:-}"
SOURCE_HASH_PATH="$TARGET.source.sha256"
TARGET_HASH_PATH="$TARGET.sha256"

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

[ -n "$TARGET" ] || fail "target launcher path is empty"
[ -f "$SOURCE" ] || fail "source launcher not found: $SOURCE"
[ -r "$SOURCE" ] || fail "source launcher is not readable: $SOURCE"

hash_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    output="$(sha256sum "$1")" || return 1
    printf '%s\n' "$output" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    output="$(shasum -a 256 "$1")" || return 1
    printf '%s\n' "$output" | awk '{print $1}'
  else
    fail "sha256 tool unavailable"
  fi
}

source_hash="$(hash_file "$SOURCE")"
mkdir -p "$(dirname -- "$TARGET")"
temporary="$TARGET.tmp.$$"
source_hash_temporary="$SOURCE_HASH_PATH.tmp.$$"
target_hash_temporary="$TARGET_HASH_PATH.tmp.$$"
trap 'rm -f "$temporary" "$source_hash_temporary" "$target_hash_temporary"' EXIT HUP INT TERM

sed \
  -e 's~^EMU=.*cut -d .* -f 5.*~EMU="${MPL_FORCE_EMU:-$(echo "$ROMFILE" | cut -d / -f 5)}"  # patched by RGFrontend~' \
  -e 's~^EMU_DIR="${ROMPATH##\*/}".*~EMU_DIR="${MPL_FORCE_EMU_DIR:-${ROMPATH##*/}}"  # patched by RGFrontend~' \
  -e 's~RACONFIG="$RA_DIR/retroarch_${EMU}.cfg"~RACONFIG="$RA_DIR/retroarch_${MPL_FORCE_RA_CONFIG_EMU:-$EMU}.cfg"  # patched by RGFrontend~' \
  -e 's~^    case \$VARC in~    if [ -n "${MPL_FORCE_VARC:-}" ]; then VARC="$MPL_FORCE_VARC"; fi  # patched by RGFrontend\
    case $VARC in~' \
  -e '/^other_readey$/a\
if [ -n "${MPL_FORCE_VIDEO_ROTATION:-}" ]; then\
    sed -i "/video_rotation = /d" "${RACONFIG}"\
    printf "%s\\n" "video_rotation = \\"${MPL_FORCE_VIDEO_ROTATION}\\"" >> "${RACONFIG}"\
fi' \
  "$SOURCE" >"$temporary"

grep -q 'MPL_FORCE_EMU' "$temporary" ||
  fail "failed to patch EMU assignment in $SOURCE"
grep -q 'MPL_FORCE_EMU_DIR' "$temporary" ||
  fail "failed to patch EMU_DIR assignment in $SOURCE"
grep -q 'MPL_FORCE_RA_CONFIG_EMU' "$temporary" ||
  fail "failed to patch RACONFIG assignment in $SOURCE"
grep -q 'MPL_FORCE_VARC' "$temporary" ||
  fail "failed to patch VARC override in $SOURCE"
grep -q 'MPL_FORCE_VIDEO_ROTATION' "$temporary" ||
  fail "failed to patch video rotation override in $SOURCE"

chmod 755 "$temporary"
target_hash="$(hash_file "$temporary")"
printf '%s\n' "$source_hash" >"$source_hash_temporary"
printf '%s\n' "$target_hash" >"$target_hash_temporary"
mv -f "$temporary" "$TARGET"
mv -f "$source_hash_temporary" "$SOURCE_HASH_PATH"
mv -f "$target_hash_temporary" "$TARGET_HASH_PATH"
trap - EXIT HUP INT TERM

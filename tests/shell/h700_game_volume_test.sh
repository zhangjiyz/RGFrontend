#!/bin/sh
set -eu

SCRIPT="H700/launcher/game_volume.sh"
ROOT="${TMPDIR:-/tmp}/mpl-h700-game-volume-test-$$"
STATE="$ROOT/state"
RA_VOLUME="$ROOT/retroarch_volume.cfg"
SYSTEM_VOLUME="$ROOT/openbor_volume"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$STATE"

run_volume() {
  MPL_STATE_DIR="$STATE" \
    MPL_H700_VOLUME_PATH="$SYSTEM_VOLUME" \
    MPL_RA_VOLUME_CFG="$RA_VOLUME" \
    sh "$SCRIPT" "$1"
}

printf '0\n' >"$SYSTEM_VOLUME"
printf '7\n' >"$STATE/volume.level"
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -qx 'audio_volume = "-80.0"' "$RA_VOLUME"
test ! -e "$STATE/game-volume.db"
test ! -e "$STATE/game-volume.schema"
test ! -e "$STATE/game-volume.frontend-level"

printf '7\n' >"$SYSTEM_VOLUME"
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -qx 'audio_volume = "-3.0"' "$RA_VOLUME"

printf 'audio_volume = "-1.9"\n' >"$RA_VOLUME"
run_volume prepare
grep -qx 'audio_volume = "-3.0"' "$RA_VOLUME"

printf '3\n' >"$SYSTEM_VOLUME"
printf -- '-29.500000\n' >"$STATE/game-volume.db"
printf '2\n' >"$STATE/game-volume.schema"
printf '7\n' >"$STATE/game-volume.frontend-level"
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -qx 'audio_volume = "-10.7"' "$RA_VOLUME"
grep -qx -- '-29.500000' "$STATE/game-volume.db"
grep -qx -- '7' "$STATE/game-volume.frontend-level"

rm -f "$SYSTEM_VOLUME"
printf '10\n' >"$STATE/volume.level"
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -qx 'audio_volume = "0.0"' "$RA_VOLUME"

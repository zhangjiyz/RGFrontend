#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/rgfrontend-autostart-test-$$"
APP="$ROOT/RGFrontend"
STATE="$ROOT/state"
TARGET="$ROOT/autostart.sh"
CONTROL="H700/launcher/autostart_ctl.sh"
LAUNCHER="H700/launcher/autostart_launch.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$APP" "$STATE"
printf '#!/bin/sh\nif false; then\n  exit 0\nfi\nprintf before\nexit 0\n' >"$TARGET"
chmod 755 "$TARGET"
cp "$CONTROL" "$APP/autostart_ctl.sh"
cp "$LAUNCHER" "$APP/autostart_launch.sh"
chmod 755 "$APP/autostart_ctl.sh" "$APP/autostart_launch.sh"

MPL_STATE_DIR="$STATE" MPL_H700_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" enable
test -f "$STATE/autostart.enabled"
test -f "$STATE/autostart.original"
grep -q '^# BEGIN RGFRONTEND AUTOSTART$' "$TARGET"
grep -n '^# BEGIN RGFRONTEND AUTOSTART$' "$TARGET" | cut -d: -f1 >"$ROOT/begin.line"
grep -n '^exit 0$' "$TARGET" | tail -n 1 | cut -d: -f1 >"$ROOT/exit.line"
test "$(cat "$ROOT/begin.line")" -lt "$(cat "$ROOT/exit.line")"
test "$(cat "$ROOT/begin.line")" -gt 4

MPL_STATE_DIR="$STATE" MPL_H700_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" enable
test "$(grep -c '^# BEGIN RGFRONTEND AUTOSTART$' "$TARGET")" -eq 1

rm -f "$APP/run_frontend.sh" "$APP/mpl_h700_frontend"
(
  sleep 0.15
  printf '#!/bin/sh\nprintf "%%s\\n" "$1" >"%s"\n' "$ROOT/launched.args" >"$APP/run_frontend.sh"
  : >"$APP/mpl_h700_frontend"
  chmod 755 "$APP/run_frontend.sh" "$APP/mpl_h700_frontend"
) &
MPL_STATE_DIR="$STATE" MPL_H700_AUTOSTART_WAIT_STEPS=20 \
  MPL_H700_AUTOSTART_WAIT_INTERVAL=0.05 sh "$STATE/autostart_launch.sh"
grep -q '^--autostart$' "$ROOT/launched.args"

MPL_STATE_DIR="$STATE" MPL_H700_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" disable
test ! -e "$STATE/autostart.enabled"

MPL_STATE_DIR="$STATE" MPL_H700_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" uninstall
test ! -e "$STATE/autostart.enabled"
test "$(grep -c '^# BEGIN RGFRONTEND AUTOSTART$' "$TARGET")" -eq 0

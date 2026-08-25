#!/bin/sh
set -eu

SOURCE_SCRIPT="H700/launcher/run_frontend.sh"
ROOT="${TMPDIR:-/tmp}/mpl-h700-run-frontend-test-$$"
APP="$ROOT/app"
STATE="$ROOT/state"
COUNT="$ROOT/count"
LOG="$STATE/logs/frontend.log"
trap 'rm -rf "$ROOT"' EXIT HUP INT TERM

mkdir -p "$APP" "$STATE"
cp "$SOURCE_SCRIPT" "$APP/run_frontend.sh"

cat >"$APP/mpl_h700_frontend" <<'APP'
#!/bin/sh
count_file="${MPL_TEST_COUNT:?}"
count="$(cat "$count_file" 2>/dev/null || printf 0)"
printf '[fake-bin] count=%s request=%s args=%s\n' "$count" "${MPL_LAUNCH_REQUEST:-}" "$*"
if [ "$count" = "0" ]; then
  printf 1 >"$count_file"
  exit 20
fi
exit 0
APP

cat >"$APP/launch_request.sh" <<'LAUNCH'
#!/bin/sh
printf '[fake-launch] request=%s\n' "${MPL_LAUNCH_REQUEST:-}"
exit 0
LAUNCH

chmod 755 "$APP/run_frontend.sh" "$APP/mpl_h700_frontend" "$APP/launch_request.sh"

MPL_STATE_DIR="$STATE" \
MPL_ROM_CARD_ROOT="$ROOT/card" \
MPL_TEST_COUNT="$COUNT" \
sh "$APP/run_frontend.sh" --surface

grep -q '\[frontend\] exit rc=20' "$LOG"
grep -q '\[fake-launch\] request='"$STATE"'/state/launch.request' "$LOG"
grep -q '\[frontend\] exit rc=0' "$LOG"

cat >"$APP/mpl_h700_frontend" <<'APP'
#!/bin/sh
printf '[fake-bin] reboot-request\n'
exit 30
APP

cat >"$APP/fake-reboot" <<'REBOOT'
#!/bin/sh
printf '[fake-reboot]\n'
exit 0
REBOOT

chmod 755 "$APP/mpl_h700_frontend" "$APP/fake-reboot"

MPL_STATE_DIR="$STATE" \
MPL_ROM_CARD_ROOT="$ROOT/card" \
MPL_H700_REBOOT_CMD="$APP/fake-reboot" \
sh "$APP/run_frontend.sh" --surface || [ "$?" -eq 30 ]

grep -q '\[frontend\] reboot requested' "$LOG"
grep -q '\[fake-reboot\]' "$LOG"

cat >"$APP/mpl_h700_frontend" <<'APP'
#!/bin/sh
count_file="${MPL_TEST_COUNT:?}"
count="$(cat "$count_file" 2>/dev/null || printf 0)"
printf '[fake-bin] suspend-count=%s\n' "$count"
if [ "$count" = "1" ]; then
  printf 2 >"$count_file"
  exit 21
fi
exit 0
APP

cat >"$APP/fake-power" <<'POWER'
#!/bin/sh
printf '[fake-power] args=%s\n' "$*"
exit 0
POWER

chmod 755 "$APP/mpl_h700_frontend" "$APP/fake-power"

MPL_STATE_DIR="$STATE" \
MPL_ROM_CARD_ROOT="$ROOT/card" \
MPL_TEST_COUNT="$COUNT" \
MPL_H700_POWER_SCRIPT="$APP/fake-power" \
sh "$APP/run_frontend.sh" --surface

grep -q '\[frontend\] exit rc=21' "$LOG"
grep -q '\[frontend\] suspending reason=power' "$LOG"
grep -q '\[fake-power\] args=' "$LOG"
grep -q '\[frontend\] resumed rc=0; restarting frontend with fresh SDL state' "$LOG"
grep -q '\[fake-bin\] suspend-count=2' "$LOG"

cat >"$APP/mpl_h700_frontend" <<'APP'
#!/bin/sh
count_file="${MPL_TEST_COUNT:?}"
count="$(cat "$count_file" 2>/dev/null || printf 0)"
printf '[fake-bin] hall-count=%s\n' "$count"
if [ "$count" = "2" ]; then
  printf 3 >"$count_file"
  exit 22
fi
exit 0
APP

chmod 755 "$APP/mpl_h700_frontend"
OS_SLEEP="$ROOT/os_sleep"
: >"$OS_SLEEP"

MPL_STATE_DIR="$STATE" \
MPL_ROM_CARD_ROOT="$ROOT/card" \
MPL_TEST_COUNT="$COUNT" \
MPL_H700_POWER_SCRIPT="$APP/fake-power" \
MPL_H700_OS_SLEEP_NODE="$OS_SLEEP" \
sh "$APP/run_frontend.sh" --surface

grep -q '\[frontend\] exit rc=22' "$LOG"
grep -q '\[frontend\] suspending reason=hall' "$LOG"
grep -q '\[fake-power\] args=auto' "$LOG"
grep -q '\[fake-bin\] hall-count=3' "$LOG"
test "$(cat "$OS_SLEEP")" = "16"

CLEAR_COUNT="$ROOT/clear-count"
cat >"$APP/mpl_h700_frontend" <<'APP'
#!/bin/sh
count_file="${MPL_TEST_COUNT:?}"
count="$(cat "$count_file" 2>/dev/null || printf 0)"
printf '[fake-bin] clear-count=%s args=%s\n' "$count" "$*"
if [ "$count" = "0" ]; then
  printf 1 >"$count_file"
  exit 32
fi
exit 0
APP

chmod 755 "$APP/mpl_h700_frontend"

MPL_STATE_DIR="$STATE" \
MPL_ROM_CARD_ROOT="$ROOT/card" \
MPL_TEST_COUNT="$CLEAR_COUNT" \
sh "$APP/run_frontend.sh" --surface

grep -q '\[frontend\] exit rc=32' "$LOG"
grep -q '\[frontend\] clear cache rescan requested; restarting with startup screen' "$LOG"
grep -q '\[fake-bin\] clear-count=1' "$LOG"
if grep '\[fake-bin\] clear-count=1' "$LOG" | grep -q -- '--restore-ui'; then
  echo "clear-cache rescan restart must show startup screen" >&2
  exit 1
fi

cat >"$APP/mpl_h700_frontend" <<'APP'
#!/bin/sh
printf '[fake-bin] screen-args=%s\n' "$*"
exit 0
APP

chmod 755 "$APP/mpl_h700_frontend"

FAKE_BIN="$ROOT/bin"
mkdir -p "$FAKE_BIN"
cat >"$FAKE_BIN/file" <<'FILE'
#!/bin/sh
case "$2" in
  *compact*) printf '%s\n' 'PNG image data, 640 x 480, 8-bit/color RGBA' ;;
  *square*) printf '%s\n' 'PNG image data, 720 x 720, 8-bit/color RGBA' ;;
  *) printf '%s\n' 'JPEG image data, baseline, precision 8, 720x480, components 3' ;;
esac
FILE
chmod 755 "$FAKE_BIN/file"

assert_detected_screen() {
  case_name="$1"
  virtual_size="$2"
  expected_width="$3"
  expected_height="$4"
  case_root="$ROOT/screen-$case_name"
  fb_dir="$case_root/fb0"
  case_state="$case_root/state"
  lcd_image="$case_root/lcd-$case_name.png"

  mkdir -p "$fb_dir"
  printf '%s\n' 'U:1280x1024p-59' 'U:720x480p-59' >"$fb_dir/modes"
  printf '%s\n' "$virtual_size" >"$fb_dir/virtual_size"
  : >"$lcd_image"

  PATH="$FAKE_BIN:$PATH" \
  MPL_STATE_DIR="$case_state" \
  MPL_ROM_CARD_ROOT="$ROOT/card" \
  MPL_FB_SYSFS_DIR="$fb_dir" \
  MPL_LCD_REFERENCE_IMAGE="$lcd_image" \
  sh "$APP/run_frontend.sh" --surface

  case_log="$case_state/logs/frontend.log"
  grep -q "screen=${expected_width}x${expected_height}" "$case_log"
  grep -q -- "--width $expected_width --height $expected_height" "$case_log"
}

assert_detected_screen stock-lcd-asset 1280,1024 720 480
assert_detected_screen compact-lcd-asset 1280,1024 640 480
assert_detected_screen square-lcd-asset 1280,1024 720 720

fallback_root="$ROOT/screen-virtual-fallback"
mkdir -p "$fallback_root/fb0"
printf '%s\n' 'U:1280x1024p-59' >"$fallback_root/fb0/modes"
printf '%s\n' '720,960' >"$fallback_root/fb0/virtual_size"
MPL_STATE_DIR="$fallback_root/state" \
MPL_ROM_CARD_ROOT="$ROOT/card" \
MPL_FB_SYSFS_DIR="$fallback_root/fb0" \
MPL_LCD_REFERENCE_IMAGE="$fallback_root/missing.png" \
sh "$APP/run_frontend.sh" --surface
grep -q 'screen=720x480' "$fallback_root/state/logs/frontend.log"

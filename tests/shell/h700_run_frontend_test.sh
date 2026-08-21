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

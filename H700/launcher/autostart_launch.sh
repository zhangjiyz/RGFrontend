#!/bin/sh
set -u

STATE_DIR="${MPL_STATE_DIR:-/mnt/data/multiplatform-launcher}"
WAIT_STEPS="${MPL_H700_AUTOSTART_WAIT_STEPS:-80}"
WAIT_INTERVAL="${MPL_H700_AUTOSTART_WAIT_INTERVAL:-0.25}"
[ -f "$STATE_DIR/autostart.enabled" ] || exit 0

app_dir=""
if [ -f "$STATE_DIR/app.path" ]; then
  IFS= read -r app_dir <"$STATE_DIR/app.path" || app_dir=""
fi

step=0
while [ "$step" -lt "$WAIT_STEPS" ]; do
  for candidate in \
    "$app_dir" \
    "/mnt/mmc/Roms/APPS/RGFrontend" \
    "/mnt/sdcard/Roms/APPS/RGFrontend"; do
    if [ -n "$candidate" ] && [ -x "$candidate/run_frontend.sh" ] &&
        [ -x "$candidate/mpl_h700_frontend" ]; then
      exec "$candidate/run_frontend.sh" --autostart
    fi
  done
  step=$((step + 1))
  sleep "$WAIT_INTERVAL"
done

exit 0

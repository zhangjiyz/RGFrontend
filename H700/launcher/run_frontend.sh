#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BIN="${MPL_H700_FRONTEND_BIN:-$APP_DIR/mpl_h700_frontend}"
LAUNCHER="$APP_DIR/launch_request.sh"
STATE_DIR="${MPL_STATE_DIR:-/mnt/data/multiplatform-launcher}"
if [ -n "${MPL_ROM_CARD_ROOTS:-}" ]; then
  ROM_CARD_ROOTS="$MPL_ROM_CARD_ROOTS"
elif [ -n "${MPL_ROM_CARD_ROOT:-}" ]; then
  ROM_CARD_ROOTS="$MPL_ROM_CARD_ROOT"
else
  ROM_CARD_ROOTS="/mnt/mmc:/mnt/sdcard"
fi
REQUEST_PATH="${MPL_LAUNCH_REQUEST:-$STATE_DIR/state/launch.request}"
LOG_DIR="${MPL_LOG_DIR:-$STATE_DIR/logs}"
LOG_FILE="$LOG_DIR/frontend.log"
REBOOT_CMD="${MPL_H700_REBOOT_CMD:-reboot}"
POWEROFF_CMD="${MPL_H700_POWEROFF_CMD:-poweroff}"
POWER_SCRIPT="${MPL_H700_POWER_SCRIPT:-/mnt/vendor/ctrl/pwr_new.sh}"
OS_SLEEP_NODE="${MPL_H700_OS_SLEEP_NODE:-/sys/class/power_supply/axp2202-battery/os_sleep}"
FB_SYSFS_DIR="${MPL_FB_SYSFS_DIR:-/sys/class/graphics/fb0}"
LCD_REFERENCE_IMAGE="${MPL_LCD_REFERENCE_IMAGE:-/mnt/vendor/res1/wallpapers/lcd/0.jpg}"

mkdir -p "$STATE_DIR/library" "$STATE_DIR/state" "$LOG_DIR"

if [ -f "$LOG_FILE" ] && [ "$(wc -c <"$LOG_FILE" 2>/dev/null || printf 0)" -gt 1048576 ]; then
  mv "$LOG_FILE" "$LOG_FILE.1"
fi

export LD_LIBRARY_PATH="/lib:/lib/aarch64-linux-gnu:/usr/lib/aarch64-linux-gnu:/usr/lib:/mnt/vendor/lib:/usr/lib32:${LD_LIBRARY_PATH:-}"

if [ -f /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.2800.5 ]; then
  export LD_PRELOAD="/usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.2800.5${LD_PRELOAD:+:$LD_PRELOAD}"
fi

export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-mali}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
export SDL_NOMOUSE="${SDL_NOMOUSE:-1}"
export MPL_ENABLE_LAUNCH=1
export MPL_ENABLE_SYSTEM_SERVICE="${MPL_ENABLE_SYSTEM_SERVICE:-1}"
export MPL_LAUNCH_REQUEST="$REQUEST_PATH"
RESTORE_UI_ONCE=0
SCREEN_WIDTH="${MPL_SCREEN_WIDTH:-}"
SCREEN_HEIGHT="${MPL_SCREEN_HEIGHT:-}"

is_positive_int() {
  case "$1" in
    ''|*[!0-9]*) return 1 ;;
  esac
  [ "$1" -gt 0 ]
}

use_screen_size() {
  if is_positive_int "$1" && is_positive_int "$2"; then
    SCREEN_WIDTH="$1"
    SCREEN_HEIGHT="$2"
    return 0
  fi
  return 1
}

use_supported_lcd_size() {
  case "$1x$2" in
    640x480|720x480|720x720) use_screen_size "$1" "$2" ;;
    *) return 1 ;;
  esac
}

detect_screen_size() {
  if use_screen_size "$SCREEN_WIDTH" "$SCREEN_HEIGHT"; then
    return
  fi

  if [ -r "$LCD_REFERENCE_IMAGE" ] && command -v file >/dev/null 2>&1; then
    lcd_size="$(file -b "$LCD_REFERENCE_IMAGE" 2>/dev/null |
      sed -n 's/.*[^0-9]\([0-9][0-9]*\)[[:space:]]*x[[:space:]]*\([0-9][0-9]*\)[^0-9].*/\1 \2/p' || true)"
    if [ -n "$lcd_size" ]; then
      set -- $lcd_size
      if use_supported_lcd_size "$1" "$2"; then
        return
      fi
    fi
  fi

  if [ -r "$FB_SYSFS_DIR/virtual_size" ]; then
    virtual_size="$(sed -n '1{s/[^0-9]*\([0-9][0-9]*\),\([0-9][0-9]*\).*/\1 \2/p;q;}' "$FB_SYSFS_DIR/virtual_size" 2>/dev/null || true)"
    if [ -n "$virtual_size" ]; then
      set -- $virtual_size
      width="$1"
      height="$2"
      if is_positive_int "$width" && is_positive_int "$height"; then
        if [ "$height" -gt "$width" ] && [ $((height % 2)) -eq 0 ] &&
           [ $((height / 2)) -ge 240 ]; then
          height=$((height / 2))
        fi
        if use_screen_size "$width" "$height"; then
          return
        fi
      fi
    fi
  fi

  if [ -r "$FB_SYSFS_DIR/modes" ]; then
    mode="$(sed -n '1{s/[^0-9]*\([0-9][0-9]*\)x\([0-9][0-9]*\).*/\1 \2/p;q;}' "$FB_SYSFS_DIR/modes" 2>/dev/null || true)"
    if [ -n "$mode" ]; then
      set -- $mode
      if use_screen_size "$1" "$2"; then
        return
      fi
    fi
  fi

  SCREEN_WIDTH=720
  SCREEN_HEIGHT=480
}

screen_size_args() {
  printf '%s\n%s\n%s\n%s\n' --width "$SCREEN_WIDTH" --height "$SCREEN_HEIGHT"
}

rom_card_root_args() {
  old_ifs="$IFS"
  IFS=:
  for root in $ROM_CARD_ROOTS; do
    [ -n "$root" ] || continue
    printf '%s\n%s\n' --rom-card-root "$root"
  done
  IFS="$old_ifs"
}

suspend_system() {
  automatic="$1"
  if [ ! -x "$POWER_SCRIPT" ]; then
    printf '[frontend] power script missing: %s\n' "$POWER_SCRIPT" >>"$LOG_FILE"
    return 1
  fi
  if [ "$automatic" -eq 1 ]; then
    if [ -w "$OS_SLEEP_NODE" ]; then
      printf 16 >"$OS_SLEEP_NODE"
    fi
    printf '[frontend] suspending reason=hall\n' >>"$LOG_FILE"
    (unset LD_PRELOAD LD_LIBRARY_PATH; "$POWER_SCRIPT" auto >>"$LOG_FILE" 2>&1)
  else
    printf '[frontend] suspending reason=power\n' >>"$LOG_FILE"
    (unset LD_PRELOAD LD_LIBRARY_PATH; "$POWER_SCRIPT" >>"$LOG_FILE" 2>&1)
  fi
  rc=$?
  printf '[frontend] resumed rc=%s; restarting frontend with fresh SDL state\n' "$rc" >>"$LOG_FILE"
  return "$rc"
}

detect_screen_size

while :; do
  printf '[frontend] start app=%s rom_card_roots=%s state_dir=%s restore_ui=%s screen=%sx%s\n' "$APP_DIR" "$ROM_CARD_ROOTS" "$STATE_DIR" "$RESTORE_UI_ONCE" "$SCREEN_WIDTH" "$SCREEN_HEIGHT" >>"$LOG_FILE"
  set +e
  if [ "$RESTORE_UI_ONCE" -eq 1 ]; then
    MPL_DISABLE_STARTUP_SCREEN=1 "$BIN" $(screen_size_args) $(rom_card_root_args) --state-dir "$STATE_DIR" --restore-ui "$@" >>"$LOG_FILE" 2>&1
  else
    "$BIN" $(screen_size_args) $(rom_card_root_args) --state-dir "$STATE_DIR" "$@" >>"$LOG_FILE" 2>&1
  fi
  rc=$?
  set -e
  RESTORE_UI_ONCE=0
  printf '[frontend] exit rc=%s\n' "$rc" >>"$LOG_FILE"
  if [ "$rc" -eq 20 ]; then
    if [ -x "$LAUNCHER" ]; then
      "$LAUNCHER" >>"$LOG_FILE" 2>&1 || true
    else
      printf '[frontend] launch script missing: %s\n' "$LAUNCHER" >>"$LOG_FILE"
    fi
    RESTORE_UI_ONCE=1
    continue
  fi
  if [ "$rc" -eq 21 ]; then
    suspend_system 0 || true
    RESTORE_UI_ONCE=1
    continue
  fi
  if [ "$rc" -eq 22 ]; then
    suspend_system 1 || true
    RESTORE_UI_ONCE=1
    continue
  fi
  if [ "$rc" -eq 32 ]; then
    printf '[frontend] clear cache rescan requested; restarting with startup screen\n' >>"$LOG_FILE"
    RESTORE_UI_ONCE=0
    continue
  fi
  if [ "$rc" -eq 30 ]; then
    printf '[frontend] reboot requested\n' >>"$LOG_FILE"
    "$REBOOT_CMD" >>"$LOG_FILE" 2>&1 || true
    exit "$rc"
  fi
  if [ "$rc" -eq 31 ]; then
    printf '[frontend] poweroff requested\n' >>"$LOG_FILE"
    "$POWEROFF_CMD" >>"$LOG_FILE" 2>&1 || true
    exit "$rc"
  fi
  exit "$rc"
done

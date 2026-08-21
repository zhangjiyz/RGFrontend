#!/bin/sh
set -eu

ACTION="${1:-}"
STATE_DIR="${MPL_STATE_DIR:-/mnt/data/multiplatform-launcher}"
RA_VOLUME_CFG="${MPL_RA_VOLUME_CFG:-/.config/retroarch/retroarch_volume.cfg}"
SYSTEM_VOLUME_PATH="${MPL_H700_VOLUME_PATH:-/sys/class/power_supply/axp2202-battery/openbor_volume}"

frontend_attenuation() {
  case "$1" in
    0) printf '%s\n' '-80.0' ;;
    1) printf '%s\n' '-20.3' ;;
    2) printf '%s\n' '-14.3' ;;
    3) printf '%s\n' '-10.7' ;;
    4) printf '%s\n' '-8.2' ;;
    5) printf '%s\n' '-5.7' ;;
    6) printf '%s\n' '-4.3' ;;
    7) printf '%s\n' '-3.0' ;;
    8) printf '%s\n' '-1.9' ;;
    9) printf '%s\n' '-0.9' ;;
    *) printf '%s\n' '0.0' ;;
  esac
}

write_ra_volume() {
  value="$1"
  mkdir -p "$(dirname -- "$RA_VOLUME_CFG")"
  temporary="$RA_VOLUME_CFG.rgfrontend.tmp.$$"
  printf 'audio_volume = "%s"\n' "$value" >"$temporary"
  mv -f "$temporary" "$RA_VOLUME_CFG"
}

current_frontend_volume() {
  level=""
  if [ -r "$SYSTEM_VOLUME_PATH" ]; then
    level="$(sed -n '1p' "$SYSTEM_VOLUME_PATH" 2>/dev/null || true)"
  fi
  if [ -z "$level" ] && [ -r "$STATE_DIR/volume.level" ]; then
    level="$(sed -n '1p' "$STATE_DIR/volume.level" 2>/dev/null || true)"
  fi
  case "$level" in
    0|1|2|3|4|5|6|7|8|9|10) ;;
    *) level=6 ;;
  esac
  printf '%s\n' "$level"
}

prepare_volume() {
  level="$(current_frontend_volume)"
  write_ra_volume "$(frontend_attenuation "$level")"
}

case "$ACTION" in
  prepare) prepare_volume ;;
  *) echo "usage: $0 prepare" >&2; exit 2 ;;
esac

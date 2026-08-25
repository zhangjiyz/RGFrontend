#!/bin/sh
set -eu

SCRIPT="H700/tools/collect_capabilities.sh"
ROOT="${TMPDIR:-/tmp}/multiplatform_launcher_h700_collect_test"

rm -rf "$ROOT"
mkdir -p "$ROOT/mnt/mmc/Roms/GBA" "$ROOT/mnt/sdcard/Roms/GBC" \
  "$ROOT/mnt/mmc/APPS" "$ROOT/mnt/mod/ctrl/configs" \
  "$ROOT/mnt/vendor/deep/retro/cores" "$ROOT/mnt/vendor/deep/retro/config"

printf 'rom data' >"$ROOT/mnt/mmc/Roms/GBA/SecretGame.gba"
printf 'rom data' >"$ROOT/mnt/sdcard/Roms/GBC/AnotherSecret.gbc"
printf 'core data' >"$ROOT/mnt/vendor/deep/retro/cores/mgba_libretro.so"
printf 'core data' >"$ROOT/mnt/vendor/deep/retro/cores/gambatte_libretro.so"
printf 'video_driver = mali\n' >"$ROOT/mnt/vendor/deep/retro/config/retroarch_GBA.cfg"
printf 'stock retroarch' >"$ROOT/mnt/vendor/deep/retro/retroarch"
chmod 755 "$ROOT/mnt/vendor/deep/retro/retroarch"
printf '#!/bin/sh\n' >"$ROOT/mnt/mod/ctrl/RA_launch.sh"
chmod 755 "$ROOT/mnt/mod/ctrl/RA_launch.sh"
printf '%s\n' '-GBA,mgba_libretro.so' >"$ROOT/mnt/mod/ctrl/configs/CORES.txt"

REPORT="$ROOT/report/h700-capabilities.txt"
MPL_H700_ROOT="$ROOT" MPL_H700_REPORT="$REPORT" sh "$SCRIPT" >/dev/null

test -f "$REPORT"
grep -q 'mmc_root=dir:/mnt/mmc' "$REPORT"
grep -q 'sdcard_root=dir:/mnt/sdcard' "$REPORT"
grep -q 'ra_launcher_order=mod_then_stock' "$REPORT"
grep -q 'ra_launcher_mod=file executable:/mnt/mod/ctrl/RA_launch.sh' "$REPORT"
grep -q 'ra_launcher_stock=file executable:/mnt/vendor/deep/retro/retroarch' "$REPORT"
grep -q 'platform_dir=/mnt/mmc/Roms/GBA name=GBA file_count=1' "$REPORT"
grep -q 'platform_dir=/mnt/sdcard/Roms/GBC name=GBC file_count=1' "$REPORT"
grep -q 'core=mgba_libretro.so' "$REPORT"
grep -q 'core=gambatte_libretro.so' "$REPORT"
grep -q 'supplemental_core_map=present path=/mnt/mod/ctrl/configs/CORES.txt authoritative=0' "$REPORT"
grep -q 'core_defaults=dmenu_builtin owner=RGFrontend priority=1' "$REPORT"
grep -q 'config=/mnt/vendor/deep/retro/config/retroarch_GBA.cfg' "$REPORT"
grep -q 'hint=/mnt/vendor/deep/retro/retroarch' "$REPORT"

if grep -q 'SecretGame.gba\|AnotherSecret.gbc' "$REPORT"; then
  echo "collector leaked individual ROM filenames" >&2
  exit 1
fi

if grep -E '(^|[[:space:]])(rm|cp|mv|chmod|chown|dd|mkfs)([[:space:]]|$)' "$SCRIPT"; then
  echo "collector contains a modifying command outside report creation" >&2
  exit 1
fi

rm -rf "$ROOT"

#!/bin/sh
set -eu

SCRIPT="H700/tools/check_system_immutability.sh"
ROOT="${TMPDIR:-/tmp}/mpl-h700-immutability-test-$$"
FS="$ROOT/rootfs"
BEFORE="$ROOT/before"
AFTER_RUNTIME="$ROOT/after-runtime"
AFTER_PROTECTED="$ROOT/after-protected"
trap 'rm -rf "$ROOT"' EXIT HUP INT TERM

mkdir -p "$FS/mnt/vendor/deep/retro/cores" \
  "$FS/mnt/vendor/deep/retro/config" \
  "$FS/mnt/vendor/ctrl" \
  "$FS/mnt/mod/ctrl/configs" \
  "$FS/.config/retroarch/config/GBA" \
  "$FS/etc/init.d"

printf 'retroarch-bin' >"$FS/mnt/vendor/deep/retro/retroarch"
printf 'core' >"$FS/mnt/vendor/deep/retro/cores/mgba_libretro.so"
printf '#!/bin/bash\n' >"$FS/mnt/mod/ctrl/RA_launch.sh"
printf -- '-GBA,mgba_libretro.so\n' >"$FS/mnt/mod/ctrl/configs/CORES.txt"
printf 'global.ssh=0\n' >"$FS/mnt/mod/ctrl/configs/system.cfg"
printf '#!/bin/sh\n' >"$FS/mnt/vendor/ctrl/loadapp.sh"
printf '#!/bin/sh\n' >"$FS/etc/init.d/S99app"
printf 'video_driver = "mali"\n' >"$FS/.config/retroarch/retroarch_GBA.cfg"

sh "$SCRIPT" snapshot "$BEFORE" "$FS" >/dev/null

printf 'video_driver = "mali"\nmenu_driver = "rgui"\n' >"$FS/.config/retroarch/retroarch_GBA.cfg"
sh "$SCRIPT" snapshot "$AFTER_RUNTIME" "$FS" >/dev/null
sh "$SCRIPT" compare "$BEFORE" "$AFTER_RUNTIME" >/dev/null
grep -q 'protected_hashes=unchanged' "$AFTER_RUNTIME/compare-report/summary.txt"
grep -q 'runtime_hashes=changed_observed' "$AFTER_RUNTIME/compare-report/summary.txt"

printf 'core changed' >"$FS/mnt/vendor/deep/retro/cores/mgba_libretro.so"
sh "$SCRIPT" snapshot "$AFTER_PROTECTED" "$FS" >/dev/null
if sh "$SCRIPT" compare "$BEFORE" "$AFTER_PROTECTED" >/dev/null 2>&1; then
  echo "protected change was not detected" >&2
  exit 1
fi
grep -q 'protected_hashes=changed' "$AFTER_PROTECTED/compare-report/summary.txt"

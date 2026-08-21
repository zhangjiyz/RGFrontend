#!/bin/sh
set -eu

SCRIPT="H700/launcher/launch_request.sh"
ROOT="${TMPDIR:-/tmp}/multiplatform_launcher_h700_launch_test"
rm -rf "$ROOT"
mkdir -p "$ROOT/mnt/mmc/Roms/GBA" "$ROOT/mnt/sdcard/Roms/GBC" \
  "$ROOT/mnt/mmc/Roms/FBNEO" "$ROOT/mnt/mmc/Roms/FBNEO FLY" \
  "$ROOT/mnt/mmc/Roms/FBNEO ETC V" \
  "$ROOT/mnt/mmc/Roms/MAME ETC" \
  "$ROOT/mnt/sdcard/Roms/FC-HD" "$ROOT/mnt/sdcard/Roms/MD hack(picodrive)" \
  "$ROOT/mnt/sdcard/Roms/N64" "$ROOT/mnt/sdcard/Roms/NDS" \
  "$ROOT/mnt/sdcard/Roms/PSP" "$ROOT/mnt/sdcard/Roms/OPENBOR" \
  "$ROOT/mnt/sdcard/Roms/PORTS" "$ROOT/mnt/sdcard/Roms/JAVA/240x320" \
  "$ROOT/mnt/sdcard/Roms/SATURN" "$ROOT/mnt/sdcard/Roms/SS" \
  "$ROOT/system/ppsspp" \
  "$ROOT/bin" "$ROOT/system" "$ROOT/state" "$ROOT/logs" "$ROOT/cores"

write_fake_launcher() {
  cat >"$ROOT/system/RA_launch.sh" <<'EOS'
#!/bin/sh
CORE_NAME="${1}"
ROMFILE="${2}"
AUTO_LOAD="${3:-}"
ROMPATH="${ROMFILE%/*}"
FILENAME="${ROMFILE##*/}"
ROMNAME="${FILENAME%.*}"
EXTNAME="${FILENAME##*.}"
RA_DIR="${MPL_FAKE_RA_DIR:-/.config/retroarch}"
EMU="$(echo "$ROMFILE" | cut -d '/' -f 5)"  # CPS2
EMU_DIR="${ROMPATH##*/}"  # CPS2
RACONFIG="$RA_DIR/retroarch_${EMU}.cfg"
mkdir -p "$RA_DIR"
[ -f "$RACONFIG" ] || : >"$RACONFIG"
VARC=0
case "$EMU" in
    FBNEO)
      VARC=1
      ;;
esac
    case $VARC in
      *) ;;
    esac
other_readey() { :; }
other_readey
printf 'argc=%s\n' "$#" >"$MPL_FAKE_LAUNCH_ARGS"
printf 'core=%s\n' "$CORE_NAME" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'rom=%s\n' "$ROMFILE" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'arg3=%s\n' "$AUTO_LOAD" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'emu=%s\n' "$EMU" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'emu_dir=%s\n' "$EMU_DIR" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'raconfig=%s\n' "$RACONFIG" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'varc=%s\n' "$VARC" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'content=%s\n' "$(cat "$ROMFILE")" >>"$MPL_FAKE_LAUNCH_ARGS"
if [ -n "${MPL_TEST_RA_SET_VOLUME:-}" ]; then
  mkdir -p "$(dirname -- "$MPL_RA_VOLUME_CFG")"
  printf 'audio_volume = "%s"\n' "$MPL_TEST_RA_SET_VOLUME" >"$MPL_RA_VOLUME_CFG"
fi
EOS
  chmod 755 "$ROOT/system/RA_launch.sh"
}

write_fake_nds_launcher() {
  cat >"$ROOT/system/setNDS.sh" <<'EOS'
#!/bin/sh
printf 'cmd=%s rom=%s\n' "$1" "$2" >>"$MPL_FAKE_NDS_ARGS"
EOS
  chmod 755 "$ROOT/system/setNDS.sh"
}

write_fake_psp_launcher() {
  cat >"$ROOT/system/ppsspp/PPSSPPSDL" <<'EOS'
#!/bin/sh
printf 'cwd=%s\n' "$(pwd)" >"$MPL_FAKE_PSP_ARGS"
printf 'rom=%s\n' "$1" >>"$MPL_FAKE_PSP_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_PSP_ARGS"
EOS
  chmod 755 "$ROOT/system/ppsspp/PPSSPPSDL"
}

write_fake_openbor_launcher() {
  cat >"$ROOT/system/openbor.sh" <<'EOS'
#!/bin/sh
printf 'setup=%s\n' "$1" >>"$MPL_FAKE_OPENBOR_ARGS"
EOS
  cat >"$ROOT/system/OpenBOR.dge" <<'EOS'
#!/bin/sh
printf 'run=%s\n' "$1" >>"$MPL_FAKE_OPENBOR_ARGS"
EOS
  chmod 755 "$ROOT/system/openbor.sh" "$ROOT/system/OpenBOR.dge"
}

write_fake_ports_shell() {
cat >"$ROOT/system/bash" <<'EOS'
#!/bin/sh
printf 'script=%s\n' "$1" >"$MPL_FAKE_PORTS_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_PORTS_ARGS"
for _ in 1 2 3 4 5; do
  [ -f "$MPL_FAKE_PORTS_JOY_ARGS" ] && break
  sleep 0.1
done
EOS
  cat >"$ROOT/system/joy" <<'EOS'
#!/bin/sh
printf 'joy=started\n' >"$MPL_FAKE_PORTS_JOY_ARGS"
EOS
  chmod 755 "$ROOT/system/bash" "$ROOT/system/joy"
}

write_fake_java_launcher() {
  cat >"$ROOT/system/launch_java.sh" <<'EOS'
#!/bin/sh
printf 'rom=%s\n' "$1" >"$MPL_FAKE_JAVA_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_JAVA_ARGS"
EOS
  chmod 755 "$ROOT/system/launch_java.sh"
}

write_fake_saturn_launcher() {
  cat >"$ROOT/system/setSaturn.sh" <<'EOS'
#!/bin/sh
printf 'script=setSaturn\n' >"$MPL_FAKE_SATURN_ARGS"
printf 'mode=%s\n' "$1" >>"$MPL_FAKE_SATURN_ARGS"
printf 'rom=%s\n' "$2" >>"$MPL_FAKE_SATURN_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_SATURN_ARGS"
EOS
  cat >"$ROOT/system/yabasanshiro" <<'EOS'
#!/bin/sh
printf 'script=yabasanshiro\n' >"$MPL_FAKE_SATURN_ARGS"
printf 'home=%s\n' "$HOME" >>"$MPL_FAKE_SATURN_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_SATURN_ARGS"
while [ "$#" -gt 0 ]; do
  printf 'arg=%s\n' "$1" >>"$MPL_FAKE_SATURN_ARGS"
  shift
done
EOS
  printf 'bios' >"$ROOT/system/saturn_bios.bin"
  chmod 755 "$ROOT/system/setSaturn.sh" "$ROOT/system/yabasanshiro"
}

write_fake_amixer() {
  cat >"$ROOT/bin/amixer" <<'EOS'
#!/bin/sh
printf '%s\n' "$*" >>"$MPL_FAKE_AMIXER_ARGS"
EOS
  chmod 755 "$ROOT/bin/amixer"
}

write_request() {
  platform="$1"
  launcher="$2"
  rom="$3"
  core_hint="${4:-}"
  cat >"$ROOT/state/launch.request" <<EOF
request_version=1
platform_id=$platform
game_id=test-game
rom_path=$rom
launcher_id=$launcher
EOF
  [ -z "$core_hint" ] || printf 'core_hint=%s\n' "$core_hint" >>"$ROOT/state/launch.request"
}

run_script() {
  MPL_STATE_DIR="$ROOT" \
  MPL_LAUNCH_REQUEST="$ROOT/state/launch.request" \
  MPL_LOG_DIR="$ROOT/logs" \
  MPL_MMC_ROOT="$ROOT/mnt/mmc" \
  MPL_SDCARD_ROOT="$ROOT/mnt/sdcard" \
  MPL_H700_RA_LAUNCHER="$ROOT/system/RA_launch.sh" \
  MPL_H700_MPL_RA_LAUNCHER="$ROOT/system/RA_launch_mpl.sh" \
  MPL_H700_GAME_VOLUME_SCRIPT="H700/launcher/game_volume.sh" \
  MPL_H700_CORE_DIR="$ROOT/cores" \
  MPL_H700_CORES_MAP="$ROOT/system/CORES.txt" \
  MPL_H700_NDS_LAUNCHER="$ROOT/system/setNDS.sh" \
  MPL_H700_PSP_LAUNCHER="$ROOT/system/ppsspp/PPSSPPSDL" \
  MPL_H700_BOARD_INI="$ROOT/system/board.ini" \
  MPL_H700_PSP_SDL_PRELOAD="$ROOT/system/libSDL2-preload.so" \
  MPL_H700_OPENBOR_SETUP="$ROOT/system/openbor.sh" \
  MPL_H700_OPENBOR_LAUNCHER="$ROOT/system/OpenBOR.dge" \
  MPL_H700_PORTS_SHELL="$ROOT/system/bash" \
  MPL_H700_PORTS_JOY_HELPER="$ROOT/system/joy" \
  MPL_H700_JAVA_LAUNCHER="$ROOT/system/launch_java.sh" \
  MPL_H700_SATURN_LAUNCHER="$ROOT/system/setSaturn.sh" \
  MPL_H700_SATURN_EMULATOR="$ROOT/system/yabasanshiro" \
  MPL_H700_SATURN_BIOS="$ROOT/system/saturn_bios.bin" \
  MPL_H700_SATURN_MODE="${MPL_H700_SATURN_MODE:-HLE}" \
  MPL_H700_SATURN_USE_SET_SCRIPT="${MPL_H700_SATURN_USE_SET_SCRIPT:-1}" \
  MPL_H700_SATURN_FULLSCREEN="${MPL_H700_SATURN_FULLSCREEN:-0}" \
  MPL_H700_ENABLE_SATURN="${MPL_H700_ENABLE_SATURN:-1}" \
  MPL_H700_VOLUME_PATH="$ROOT/system/openbor_volume" \
  MPL_RA_VOLUME_CFG="$ROOT/retroarch/retroarch_volume.cfg" \
  MPL_FAKE_RA_DIR="$ROOT/retroarch" \
  MPL_FAKE_LAUNCH_ARGS="$ROOT/launched.args" \
  MPL_FAKE_NDS_ARGS="$ROOT/nds.args" \
  MPL_FAKE_PSP_ARGS="$ROOT/psp.args" \
  MPL_FAKE_OPENBOR_ARGS="$ROOT/openbor.args" \
  MPL_FAKE_PORTS_ARGS="$ROOT/ports.args" \
  MPL_FAKE_PORTS_JOY_ARGS="$ROOT/ports_joy.args" \
  MPL_FAKE_JAVA_ARGS="$ROOT/java.args" \
  MPL_FAKE_SATURN_ARGS="$ROOT/saturn.args" \
  MPL_FAKE_AMIXER_ARGS="$ROOT/amixer.args" \
  MPL_TEST_RA_SET_VOLUME="${MPL_TEST_RA_SET_VOLUME:-}" \
  PATH="$ROOT/bin:$PATH" \
  sh "$SCRIPT"
}

expect_failure() {
  if run_script; then
    echo "expected failure but launch succeeded" >&2
    exit 1
  fi
  return 0
}

write_fake_launcher
write_fake_nds_launcher
write_fake_psp_launcher
write_fake_openbor_launcher
write_fake_ports_shell
write_fake_java_launcher
write_fake_saturn_launcher
write_fake_amixer
PSP_WORKDIR="$(cd "$ROOT/system/ppsspp" && pwd)"
printf '6\n' >"$ROOT/system/openbor_volume"
printf '6\n' >"$ROOT/volume.level"
cat >"$ROOT/system/CORES.txt" <<EOF
-GBA,mgba_libretro.so
-GBC,gambatte_libretro.so
-FBNEO,fbalpha2012_libretro.so
-MD,genesis_plus_gx_libretro.so
-N64,parallel_n64_libretro.so
-NDS,drastic
-PSP,PPSSPPSDL
-OPENBOR,OpenBOR.dge
-PORTS,bash
-JAVA,launch.sh
-SATURN,yabasanshiro_libretro.so
EOF
printf 'core' >"$ROOT/cores/mgba_libretro.so"
printf 'core' >"$ROOT/cores/gambatte_libretro.so"
printf 'core' >"$ROOT/cores/mesen_libretro.so"
printf 'core' >"$ROOT/cores/fbalpha2012_libretro.so"
printf 'core' >"$ROOT/cores/fbneo_libretro.so"
printf 'core' >"$ROOT/cores/picodrive_libretro.so"
printf 'core' >"$ROOT/cores/genesis_plus_gx_libretro.so"
printf 'core' >"$ROOT/cores/mupen64plus_next_libretro.so"
printf 'core' >"$ROOT/cores/parallel_n64_libretro.so"
printf 'core' >"$ROOT/cores/mame2022xtreme_libretro.so"
printf 'rom' >"$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
printf 'rom' >"$ROOT/mnt/sdcard/Roms/GBC/Oracle.gbc"
printf 'hd' >"$ROOT/mnt/sdcard/Roms/FC-HD/HD.nes"
printf 'md-pico' >"$ROOT/mnt/sdcard/Roms/MD hack(picodrive)/Streets.zip"
printf 'n64' >"$ROOT/mnt/sdcard/Roms/N64/Smash.z64"
printf 'nds' >"$ROOT/mnt/sdcard/Roms/NDS/Ys.nds"
printf 'psp' >"$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso"
printf 'openbor' >"$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak"
printf 'ports' >"$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh"
printf 'java' >"$ROOT/mnt/sdcard/Roms/JAVA/240x320/DoomRPG.jar"
printf 'saturn' >"$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd"
printf 'old-saturn' >"$ROOT/mnt/sdcard/Roms/SS/Nights.chd"
printf 'fly' >"$ROOT/mnt/mmc/Roms/FBNEO FLY/s1945p.zip"
printf 'vertical' >"$ROOT/mnt/mmc/Roms/FBNEO ETC V/arkanoid.zip"
printf 'mame' >"$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
printf '7\n' >"$ROOT/system/openbor_volume"
printf '7\n' >"$ROOT/volume.level"
mkdir -p "$ROOT/retroarch"
printf 'audio_volume = "0.0"\n' >"$ROOT/retroarch/retroarch_volume.cfg"
rm -f "$ROOT/amixer.args" "$ROOT/game-volume.db" "$ROOT/game-volume.schema" \
  "$ROOT/game-volume.frontend-level"
run_script
test ! -f "$ROOT/state/launch.request"
grep -q 'MPL_FORCE_EMU' "$ROOT/system/RA_launch_mpl.sh"
test -s "$ROOT/system/RA_launch_mpl.sh.source.sha256"
test -s "$ROOT/system/RA_launch_mpl.sh.sha256"
grep -q 'MPL_FORCE_RA_CONFIG_EMU' "$ROOT/system/RA_launch_mpl.sh"
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=mgba_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/GBA/Metroid.gba" "$ROOT/launched.args"
grep -qx 'arg3=' "$ROOT/launched.args"
grep -qx 'emu=GBA' "$ROOT/launched.args"
grep -qx 'emu_dir=GBA' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch_GBA.cfg" "$ROOT/launched.args"
grep -qx -- '-q -c 0 set lineout volume 31' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set lineout volume 22' "$ROOT/amixer.args"
grep -qx 'audio_volume = "-3.0"' "$ROOT/retroarch/retroarch_volume.cfg"
test ! -e "$ROOT/game-volume.db"

rm -f "$ROOT/launched.args" "$ROOT/amixer.args"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
MPL_TEST_RA_SET_VOLUME=-1.9 run_script
grep -qx 'audio_volume = "-1.9"' "$ROOT/retroarch/retroarch_volume.cfg"
unset MPL_TEST_RA_SET_VOLUME

rm -f "$ROOT/launched.args" "$ROOT/amixer.args"
printf '3\n' >"$ROOT/system/openbor_volume"
printf '3\n' >"$ROOT/volume.level"
printf -- '-29.500000\n' >"$ROOT/game-volume.db"
printf '2\n' >"$ROOT/game-volume.schema"
printf '7\n' >"$ROOT/game-volume.frontend-level"
printf 'audio_volume = "0.0"\n' >"$ROOT/retroarch/retroarch_volume.cfg"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
run_script
grep -qx 'audio_volume = "-10.7"' "$ROOT/retroarch/retroarch_volume.cfg"
grep -qx -- '-29.500000' "$ROOT/game-volume.db"
grep -qx -- '7' "$ROOT/game-volume.frontend-level"
grep -qx -- '-q -c 0 set lineout volume 9' "$ROOT/amixer.args"

rm -f "$ROOT/launched.args"
write_request gbc h700-retroarch-gbc "$ROOT/mnt/sdcard/Roms/GBC/Oracle.gbc"
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=gambatte_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/GBC/Oracle.gbc" "$ROOT/launched.args"
grep -qx 'emu=GBC' "$ROOT/launched.args"
grep -qx 'emu_dir=GBC' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request fc_hd h700-retroarch-fc_hd \
  "$ROOT/mnt/sdcard/Roms/FC-HD/HD.nes" mesen_libretro.so
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=mesen_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/FC-HD/HD.nes" "$ROOT/launched.args"
grep -qx 'emu=FC-HD' "$ROOT/launched.args"
grep -qx 'emu_dir=FC-HD' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request md h700-retroarch-md \
  "$ROOT/mnt/sdcard/Roms/MD hack(picodrive)/Streets.zip" picodrive_libretro.so
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=picodrive_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/MD hack(picodrive)/Streets.zip" "$ROOT/launched.args"
grep -qx 'emu=MD' "$ROOT/launched.args"
grep -qx 'emu_dir=MD' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request n64 h700-retroarch-n64 "$ROOT/mnt/sdcard/Roms/N64/Smash.z64"
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=mupen64plus_next_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/N64/Smash.z64" "$ROOT/launched.args"
grep -qx 'emu=N64' "$ROOT/launched.args"
grep -qx 'emu_dir=N64' "$ROOT/launched.args"

rm -f "$ROOT/nds.args"
write_request nds h700-standalone-nds "$ROOT/mnt/sdcard/Roms/NDS/Ys.nds"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "cmd=savedir rom=$ROOT/mnt/sdcard/Roms/NDS/Ys.nds" "$ROOT/nds.args"
grep -qx "cmd=run rom=$ROOT/mnt/sdcard/Roms/NDS/Ys.nds" "$ROOT/nds.args"

rm -f "$ROOT/psp.args"
write_request psp h700-standalone-psp "$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "cwd=$PSP_WORKDIR" "$ROOT/psp.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso" "$ROOT/psp.args"
grep -qx 'ld_preload=' "$ROOT/psp.args"

printf 'RG28xx\n' >"$ROOT/system/board.ini"
printf 'preload' >"$ROOT/system/libSDL2-preload.so"
rm -f "$ROOT/psp.args"
write_request psp h700-standalone-psp "$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso"
run_script
grep -qx "ld_preload=$ROOT/system/libSDL2-preload.so" "$ROOT/psp.args"
rm -f "$ROOT/system/board.ini" "$ROOT/system/libSDL2-preload.so"

rm -f "$ROOT/openbor.args"
write_request openbor h700-standalone-openbor \
  "$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "setup=$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak" "$ROOT/openbor.args"
grep -qx "run=$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak" "$ROOT/openbor.args"

rm -f "$ROOT/ports.args" "$ROOT/ports_joy.args"
write_request ports h700-standalone-ports "$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "script=$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh" "$ROOT/ports.args"
grep -qx 'ld_preload=' "$ROOT/ports.args"
test ! -f "$ROOT/ports_joy.args"

printf 'RG34xx\n' >"$ROOT/system/board.ini"
rm -f "$ROOT/ports.args" "$ROOT/ports_joy.args"
write_request ports h700-standalone-ports "$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh"
run_script
grep -qx "script=$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh" "$ROOT/ports.args"
for _ in 1 2 3 4 5; do
  [ -f "$ROOT/ports_joy.args" ] && break
  sleep 0.1
done
grep -qx 'joy=started' "$ROOT/ports_joy.args"
rm -f "$ROOT/system/board.ini"

printf 'not-a-script' >"$ROOT/mnt/sdcard/Roms/PORTS/readme.txt"
write_request ports h700-standalone-ports "$ROOT/mnt/sdcard/Roms/PORTS/readme.txt"
expect_failure
grep -q 'unsupported_extension platform=ports' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/java.args"
write_request java h700-standalone-java "$ROOT/mnt/sdcard/Roms/JAVA/240x320/DoomRPG.jar"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/JAVA/240x320/DoomRPG.jar" "$ROOT/java.args"
grep -qx 'ld_preload=' "$ROOT/java.args"

printf 'not-java' >"$ROOT/mnt/sdcard/Roms/JAVA/240x320/readme.txt"
write_request java h700-standalone-java "$ROOT/mnt/sdcard/Roms/JAVA/240x320/readme.txt"
expect_failure
grep -q 'unsupported_extension platform=java' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/saturn.args"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd" \
  mednafen_saturn_libretro.so
MPL_H700_ENABLE_SATURN=0 expect_failure
grep -q 'platform_disabled platform=saturn' "$ROOT/logs/h700-launch.log"
test -f "$ROOT/state/launch.request"
unset MPL_H700_ENABLE_SATURN

rm -f "$ROOT/saturn.args"
printf '0\n' >"$ROOT/system/openbor_volume"
rm -f "$ROOT/amixer.args"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd" \
  mednafen_saturn_libretro.so
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx 'script=setSaturn' "$ROOT/saturn.args"
grep -qx 'mode=HLE' "$ROOT/saturn.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd" "$ROOT/saturn.args"
grep -qx 'ld_preload=' "$ROOT/saturn.args"
if grep -qx 'script=yabasanshiro' "$ROOT/saturn.args"; then
  echo "Saturn default must use vendor setSaturn.sh" >&2
  exit 1
fi
grep -qx -- '-q -c 0 set lineout volume 0' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 0' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK off' "$ROOT/amixer.args"
if grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"; then
  echo "Saturn launch must keep speaker muted when system volume is 0" >&2
  exit 1
fi

rm -f "$ROOT/saturn.args"
printf '7\n' >"$ROOT/system/openbor_volume"
rm -f "$ROOT/amixer.args"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd" \
  mednafen_saturn_libretro.so
MPL_H700_SATURN_USE_SET_SCRIPT=0 MPL_H700_SATURN_MODE=BIOS run_script
test ! -f "$ROOT/state/launch.request"
grep -qx 'script=yabasanshiro' "$ROOT/saturn.args"
grep -qx "home=$ROOT/mnt/sdcard" "$ROOT/saturn.args"
grep -qx 'ld_preload=' "$ROOT/saturn.args"
grep -qx 'arg=-r' "$ROOT/saturn.args"
grep -qx 'arg=3' "$ROOT/saturn.args"
grep -qx 'arg=-i' "$ROOT/saturn.args"
grep -qx "arg=$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd" "$ROOT/saturn.args"
grep -qx 'arg=-b' "$ROOT/saturn.args"
grep -qx "arg=$ROOT/system/saturn_bios.bin" "$ROOT/saturn.args"
grep -qx 'arg=-a' "$ROOT/saturn.args"
if grep -qx 'arg=-f' "$ROOT/saturn.args"; then
  echo "Saturn default must not force fullscreen" >&2
  exit 1
fi
grep -qx -- '-q -c 0 set lineout volume 22' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"

rm -f "$ROOT/saturn.args"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd"
MPL_H700_SATURN_USE_SET_SCRIPT=0 MPL_H700_SATURN_FULLSCREEN=1 run_script
grep -qx 'script=yabasanshiro' "$ROOT/saturn.args"
grep -qx 'arg=-f' "$ROOT/saturn.args"

rm -f "$ROOT/saturn.args"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SATURN/Nights.chd"
MPL_H700_SATURN_USE_SET_SCRIPT=0 MPL_H700_SATURN_MODE=HLE run_script
grep -qx 'script=yabasanshiro' "$ROOT/saturn.args"
grep -qx 'arg=-a' "$ROOT/saturn.args"
if grep -qx 'arg=-b' "$ROOT/saturn.args"; then
  echo "HLE mode must not pass BIOS argument" >&2
  exit 1
fi
unset MPL_H700_SATURN_USE_SET_SCRIPT MPL_H700_SATURN_MODE MPL_H700_SATURN_FULLSCREEN

rm -f "$ROOT/saturn.args"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SS/Nights.chd"
MPL_H700_SATURN_MODE=BIOS run_script
grep -qx 'script=setSaturn' "$ROOT/saturn.args"
grep -qx 'mode=BIOS' "$ROOT/saturn.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/SS/Nights.chd" "$ROOT/saturn.args"
unset MPL_H700_SATURN_MODE

printf 'not-saturn' >"$ROOT/mnt/sdcard/Roms/SS/readme.txt"
write_request saturn h700-standalone-saturn "$ROOT/mnt/sdcard/Roms/SS/readme.txt"
expect_failure
grep -q 'unsupported_extension platform=saturn' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/launched.args"
write_request mame h700-retroarch-mame \
  "$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip" mame2022xtreme_libretro.so
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=mame2022xtreme_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip" "$ROOT/launched.args"
grep -qx 'emu=MAME' "$ROOT/launched.args"
grep -qx 'emu_dir=MAME' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request mame h700-retroarch-mame \
  "$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip"
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=mame2022xtreme_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip" "$ROOT/launched.args"
grep -qx 'emu=MAME' "$ROOT/launched.args"
grep -qx 'emu_dir=MAME' "$ROOT/launched.args"

first_source_hash="$(sed -n '1p' "$ROOT/system/RA_launch_mpl.sh.source.sha256")"
printf '\n# source update with preserved mtime\n' >>"$ROOT/system/RA_launch.sh"
touch -r "$ROOT/system/RA_launch_mpl.sh" "$ROOT/system/RA_launch.sh" 2>/dev/null || true
rm -f "$ROOT/launched.args"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
run_script
second_source_hash="$(sed -n '1p' "$ROOT/system/RA_launch_mpl.sh.source.sha256")"
[ "$first_source_hash" != "$second_source_hash" ]
grep -q 'source update with preserved mtime' "$ROOT/system/RA_launch_mpl.sh"
grep -qx 'core=mgba_libretro.so' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
printf 'standard' >"$ROOT/mnt/mmc/Roms/FBNEO/s1945p.zip"
write_request fbneo h700-retroarch-fbneo \
  "$ROOT/mnt/mmc/Roms/FBNEO FLY/s1945p.zip" fbneo_plus_libretro.so
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=fbneo_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/FBNEO FLY/s1945p.zip" "$ROOT/launched.args"
grep -qx 'arg3=' "$ROOT/launched.args"
grep -qx 'emu=FBNEO' "$ROOT/launched.args"
grep -qx 'emu_dir=FBNEO' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch_FBNEO.cfg" "$ROOT/launched.args"
grep -qx 'varc=1' "$ROOT/launched.args"
grep -qx 'content=fly' "$ROOT/launched.args"
! grep -q 'temporary rom link' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/launched.args"
write_request fbneo h700-retroarch-fbneo \
  "$ROOT/mnt/mmc/Roms/FBNEO ETC V/arkanoid.zip" fbneo_libretro.so
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=fbneo_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/FBNEO ETC V/arkanoid.zip" "$ROOT/launched.args"
grep -qx 'arg3=' "$ROOT/launched.args"
grep -qx 'emu=FBNEO' "$ROOT/launched.args"
grep -qx 'emu_dir=FBNEO V' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch_FBNEO V.cfg" "$ROOT/launched.args"
grep -qx 'varc=2' "$ROOT/launched.args"
grep -qx 'content=vertical' "$ROOT/launched.args"

write_request gba h700-retroarch-gbc "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'launcher_mismatch' "$ROOT/logs/h700-launch.log"

write_request xbox h700-retroarch-xbox "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'unsupported_platform' "$ROOT/logs/h700-launch.log"

printf 'rom' >"$ROOT/outside.gba"
write_request gba h700-retroarch-gba "$ROOT/outside.gba"
expect_failure
grep -q 'rom_outside_roots' "$ROOT/logs/h700-launch.log"

write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Missing.gba"
expect_failure
grep -q 'rom_missing' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/system/RA_launch.sh"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'launcher_missing' "$ROOT/logs/h700-launch.log"

write_fake_launcher
cat >"$ROOT/system/CORES.txt" <<EOF
-GBA,../bad_libretro.so
EOF
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'core_unconfigured' "$ROOT/logs/h700-launch.log"

cat >"$ROOT/system/CORES.txt" <<EOF
-GBA,bad;core_libretro.so
EOF
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'core_unconfigured' "$ROOT/logs/h700-launch.log"

cat >"$ROOT/system/CORES.txt" <<EOF
-GBA,mgba_libretro.so
EOF
rm -f "$ROOT/cores/mgba_libretro.so"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'core_missing' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/cores/"*
if find "$ROOT" -path '*/cores/*' | grep -q .; then
  echo "script wrote protected core paths" >&2
  exit 1
fi

rm -rf "$ROOT"

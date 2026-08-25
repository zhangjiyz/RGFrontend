#!/bin/sh
set -eu

SCRIPT="H700/launcher/launch_request.sh"
ROOT="${TMPDIR:-/tmp}/multiplatform_launcher_h700_launch_test"
rm -rf "$ROOT"
mkdir -p "$ROOT/mnt/mmc/Roms/GBA" "$ROOT/mnt/sdcard/Roms/GBC" \
  "$ROOT/mnt/mmc/Roms/CPS1" "$ROOT/mnt/mmc/Roms/CPS2" \
  "$ROOT/mnt/mmc/Roms/CPS3" \
  "$ROOT/mnt/mmc/Roms/FDS" "$ROOT/mnt/mmc/Roms/HBMAME" \
  "$ROOT/mnt/mmc/Roms/NEOGEO" "$ROOT/mnt/mmc/Roms/SFC" \
  "$ROOT/mnt/mmc/Roms/FBNEO" "$ROOT/mnt/mmc/Roms/FBNEO FLY" \
  "$ROOT/mnt/mmc/Roms/FBNEO ETC V" \
  "$ROOT/mnt/mmc/Roms/MAME ETC" \
  "$ROOT/mnt/sdcard/Roms/FC-HD" "$ROOT/mnt/sdcard/Roms/MD hack(picodrive)" \
  "$ROOT/mnt/sdcard/Roms/N64" "$ROOT/mnt/sdcard/Roms/NDS" \
  "$ROOT/mnt/sdcard/Roms/PSP" "$ROOT/mnt/sdcard/Roms/OPENBOR" \
  "$ROOT/mnt/sdcard/Roms/PORTS" "$ROOT/mnt/sdcard/Roms/JAVA/240x320" \
  "$ROOT/mnt/sdcard/Roms/SATURN" "$ROOT/mnt/sdcard/Roms/SS" \
  "$ROOT/system/ppsspp" \
  "$ROOT/system/drastic" \
  "$ROOT/bin" "$ROOT/system" "$ROOT/state" "$ROOT/logs" "$ROOT/cores"

write_fake_launcher() {
  cat >"$ROOT/system/retroarch" <<'EOS'
#!/bin/sh
ARGC="$#"
RACONFIG=""
CORE_PATH=""
APPEND_CONFIG=""
ROMFILE=""
while [ "$#" -gt 0 ]; do
  case "$1" in
    -c) RACONFIG="$2"; shift 2 ;;
    -L) CORE_PATH="$2"; shift 2 ;;
    --appendconfig) APPEND_CONFIG="$2"; shift 2 ;;
    *) ROMFILE="$1"; shift ;;
  esac
done
CORE_NAME="${CORE_PATH##*/}"
RESOLVED_ROMFILE="$(readlink "$ROMFILE" 2>/dev/null || printf '%s' "$ROMFILE")"
ROMPATH="${ROMFILE%/*}"
FILENAME="${ROMFILE##*/}"
EMU_DIR="${ROMPATH##*/}"
EMU="${EMU_DIR% H}"
EMU="${EMU% V}"
case "$CORE_NAME" in
  fbalpha_libretro.so) CORE_FNAME="FB Alpha" ;;
  fbalpha2012_libretro.so) CORE_FNAME="FB Alpha 2012" ;;
  *) CORE_FNAME="Test Core" ;;
esac
REMAP_FILE="${MPL_FAKE_REMAP_ROOT}/${CORE_FNAME}/${EMU_DIR}.rmp"
ROTATION="$(sed -n 's/^video_rotation = "\([0-9][0-9]*\)"/\1/p' "$APPEND_CONFIG" | tail -n 1)"
VARC=0
INPUT_MODE=default
case "$EMU" in
    FBNEO|NEOGEO|MAME|HBMAME|PGM2|CPS*)
      VARC=1
      if [ "$ROTATION" = "3" ]; then
        [ "$EMU" = "HBMAME" ] && VARC=4 || VARC=2
      fi
      ;;
    VARCADE)
      VARC=3
      ;;
esac
printf 'argc=%s\n' "$ARGC" >"$MPL_FAKE_LAUNCH_ARGS"
printf 'core=%s\n' "$CORE_NAME" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'rom=%s\n' "$RESOLVED_ROMFILE" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'launch_rom=%s\n' "$ROMFILE" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'emu=%s\n' "$EMU" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'emu_dir=%s\n' "$EMU_DIR" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'raconfig=%s\n' "$RACONFIG" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'appendconfig=%s\n' "$APPEND_CONFIG" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'varc=%s\n' "$VARC" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'rotation=%s\n' "$ROTATION" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'cwd=%s\n' "$(pwd)" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_LAUNCH_ARGS"
if [ -f "$ROMPATH/neogeo.zip" ]; then
  printf 'neogeo_bios=present\n' >>"$MPL_FAKE_LAUNCH_ARGS"
else
  printf 'neogeo_bios=absent\n' >>"$MPL_FAKE_LAUNCH_ARGS"
fi
if [ -f "$REMAP_FILE" ]; then
  printf 'remap=present\n' >>"$MPL_FAKE_LAUNCH_ARGS"
else
  printf 'remap=absent\n' >>"$MPL_FAKE_LAUNCH_ARGS"
fi
printf 'content=%s\n' "$(cat "$ROMFILE")" >>"$MPL_FAKE_LAUNCH_ARGS"
if [ -n "${MPL_TEST_RA_SET_VOLUME:-}" ]; then
  mkdir -p "$(dirname -- "$MPL_RA_VOLUME_CFG")"
  printf 'audio_volume = "%s"\n' "$MPL_TEST_RA_SET_VOLUME" >"$MPL_RA_VOLUME_CFG"
fi
EOS
  chmod 755 "$ROOT/system/retroarch"
}

write_fake_mod_launcher() {
  cat >"$ROOT/system/mod-retroarch" <<'EOS'
#!/bin/sh
: >"$MPL_FAKE_MOD_RA_ARGS"
index=1
for argument in "$@"; do
  printf 'arg%s=%s\n' "$index" "$argument" >>"$MPL_FAKE_MOD_RA_ARGS"
  index=$((index + 1))
done
ROM_DIR="${argument%/*}"
if [ -f "$ROM_DIR/neogeo.zip" ]; then
  printf 'neogeo_bios=present\n' >>"$MPL_FAKE_MOD_RA_ARGS"
else
  printf 'neogeo_bios=absent\n' >>"$MPL_FAKE_MOD_RA_ARGS"
fi
EOS
  chmod 755 "$ROOT/system/mod-retroarch"
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
    FBNEO|CPS1|CPS2|CPS3)
      VARC=1
      ;;
esac
    case $VARC in
      1) INPUT_MODE=horizontal ;;
      2|3|4)
        INPUT_MODE=vertical
        sed -i '/video_rotation = /d' "$RACONFIG"
        printf 'video_rotation = "3"\n' >>"$RACONFIG"
        ;;
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
printf 'input_mode=%s\n' "$INPUT_MODE" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'forced_rotation=%s\n' "${MPL_FORCE_VIDEO_ROTATION:-}" >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'rotation=%s\n' \
  "$(sed -n 's/^video_rotation = "\([0-9][0-9]*\)"/\1/p' "$RACONFIG" | tail -n 1)" \
  >>"$MPL_FAKE_LAUNCH_ARGS"
printf 'content=%s\n' "$(cat "$ROMFILE")" >>"$MPL_FAKE_LAUNCH_ARGS"
RABIN="$MPL_FAKE_MOD_RA_BIN"
COREDIR="$MPL_FAKE_CORE_DIR"
RUNTHIS='${RABIN} -c ${RACONFIG} -L "${COREDIR}/${CORE_NAME}" "$ROMFILE"'
eval ${RUNTHIS}
EOS
  chmod 755 "$ROOT/system/RA_launch.sh"
}

write_fake_nds_launcher() {
  cat >"$ROOT/system/setNDS.sh" <<'EOS'
#!/bin/sh
printf 'cmd=%s rom=%s\n' "$1" "$2" >>"$MPL_FAKE_NDS_ARGS"
if [ "$1" = "run" ]; then
  printf 'cwd=%s\n' "$(pwd)" >"$MPL_FAKE_NDS_RUNTIME_ARGS"
  printf 'control_state=%s\n' "$(cat "$MPL_FAKE_NDS_CONTROL_PATH")" >>"$MPL_FAKE_NDS_RUNTIME_ARGS"
  printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_NDS_RUNTIME_ARGS"
  printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_NDS_RUNTIME_ARGS"
  printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_NDS_RUNTIME_ARGS"
fi
EOS
  chmod 755 "$ROOT/system/setNDS.sh"
}

write_fake_psp_launcher() {
  cat >"$ROOT/system/ppsspp/PPSSPPSDL" <<'EOS'
#!/bin/sh
printf 'cwd=%s\n' "$(pwd)" >"$MPL_FAKE_PSP_ARGS"
printf 'rom=%s\n' "$1" >>"$MPL_FAKE_PSP_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_PSP_ARGS"
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_PSP_ARGS"
printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_PSP_ARGS"
printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_PSP_ARGS"
printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_PSP_ARGS"
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
printf 'cwd=%s\n' "$(pwd)" >>"$MPL_FAKE_OPENBOR_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_OPENBOR_ARGS"
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_OPENBOR_ARGS"
printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_OPENBOR_ARGS"
printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_OPENBOR_ARGS"
printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_OPENBOR_ARGS"
EOS
  chmod 755 "$ROOT/system/openbor.sh" "$ROOT/system/OpenBOR.dge"
}

write_fake_ports_shell() {
cat >"$ROOT/system/bash" <<'EOS'
#!/bin/sh
printf 'script=%s\n' "$1" >"$MPL_FAKE_PORTS_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_PORTS_ARGS"
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_PORTS_ARGS"
printf 'cwd=%s\n' "$(pwd)" >>"$MPL_FAKE_PORTS_ARGS"
printf 'control_state=%s\n' "$(cat "$MPL_FAKE_PORTS_CONTROL_PATH")" >>"$MPL_FAKE_PORTS_ARGS"
printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_PORTS_ARGS"
printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_PORTS_ARGS"
printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_PORTS_ARGS"
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
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_JAVA_ARGS"
printf 'cwd=%s\n' "$(pwd)" >>"$MPL_FAKE_JAVA_ARGS"
printf 'control_state=%s\n' "$(cat "$MPL_FAKE_JAVA_CONTROL_PATH")" >>"$MPL_FAKE_JAVA_ARGS"
printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_JAVA_ARGS"
printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_JAVA_ARGS"
printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_JAVA_ARGS"
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
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'cwd=%s\n' "$(pwd)" >>"$MPL_FAKE_SATURN_ARGS"
printf 'control_state=%s\n' "$(cat "$MPL_FAKE_SATURN_CONTROL_PATH")" >>"$MPL_FAKE_SATURN_ARGS"
printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_SATURN_ARGS"
EOS
  cat >"$ROOT/system/yabasanshiro" <<'EOS'
#!/bin/sh
printf 'script=yabasanshiro\n' >"$MPL_FAKE_SATURN_ARGS"
printf 'home=%s\n' "$HOME" >>"$MPL_FAKE_SATURN_ARGS"
printf 'ld_preload=%s\n' "${LD_PRELOAD:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'ld_library_path=%s\n' "${LD_LIBRARY_PATH:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'cwd=%s\n' "$(pwd)" >>"$MPL_FAKE_SATURN_ARGS"
printf 'control_state=%s\n' "$(cat "$MPL_FAKE_SATURN_CONTROL_PATH")" >>"$MPL_FAKE_SATURN_ARGS"
printf 'sdl_video=%s\n' "${SDL_VIDEODRIVER:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'sdl_audio=%s\n' "${SDL_AUDIODRIVER:-}" >>"$MPL_FAKE_SATURN_ARGS"
printf 'sdl_nomouse=%s\n' "${SDL_NOMOUSE:-}" >>"$MPL_FAKE_SATURN_ARGS"
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
  MPL_H700_RA_LAUNCHER="$ROOT/system/retroarch" \
  MPL_H700_MOD_RA_LAUNCHER="${MPL_H700_MOD_RA_LAUNCHER:-$ROOT/system/missing-RA_launch.sh}" \
  MPL_H700_MPL_RA_LAUNCHER="$ROOT/system/RA_launch_mpl.sh" \
  MPL_H700_PREPARE_RA_LAUNCHER="H700/launcher/prepare_ra_launcher.sh" \
  MPL_H700_RA_WORKDIR="$ROOT/system" \
  MPL_H700_RA_CONFIG="$ROOT/retroarch/retroarch.cfg" \
  MPL_H700_RA_BASE_CONFIG="$ROOT/system/retroarch-base.cfg" \
  MPL_H700_RA_APPEND_CONFIG="$ROOT/runtime/retroarch-rgfrontend.cfg" \
  MPL_H700_GAME_VOLUME_SCRIPT="H700/launcher/game_volume.sh" \
  MPL_H700_CORE_DIR="$ROOT/cores" \
  MPL_FAKE_MOD_RA_BIN="$ROOT/system/mod-retroarch" \
  MPL_FAKE_MOD_RA_ARGS="$ROOT/mod-retroarch.args" \
  MPL_FAKE_CORE_DIR="$ROOT/cores" \
  MPL_H700_VERTICAL_ARCADE_LIST="$ROOT/system/varc.cfg" \
  MPL_H700_NDS_LAUNCHER="$ROOT/system/setNDS.sh" \
  MPL_H700_NDS_WORKDIR="$NDS_WORKDIR" \
  MPL_H700_NDS_CONTROL_STATE_PATH="$ROOT/system/nds_esckey" \
  MPL_H700_PSP_LAUNCHER="$ROOT/system/ppsspp/PPSSPPSDL" \
  MPL_H700_BOARD_INI="$ROOT/system/board.ini" \
  MPL_H700_PSP_SDL_PRELOAD="$ROOT/system/libSDL2-preload.so" \
  MPL_H700_OPENBOR_SETUP="$ROOT/system/openbor.sh" \
  MPL_H700_OPENBOR_LAUNCHER="$ROOT/system/OpenBOR.dge" \
  MPL_H700_OPENBOR_WORKDIR="$OPENBOR_WORKDIR" \
  MPL_H700_PORTS_SHELL="$ROOT/system/bash" \
  MPL_H700_PORTS_JOY_HELPER="$ROOT/system/joy" \
  MPL_H700_PORTS_WORKDIR="$PORTS_WORKDIR" \
  MPL_H700_PORTS_CONTROL_STATE_PATH="$ROOT/system/ports_esckey" \
  MPL_H700_JAVA_LAUNCHER="$ROOT/system/launch_java.sh" \
  MPL_H700_JAVA_WORKDIR="$JAVA_WORKDIR" \
  MPL_H700_JAVA_CONTROL_STATE_PATH="$ROOT/system/java_esckey" \
  MPL_H700_SATURN_LAUNCHER="$ROOT/system/setSaturn.sh" \
  MPL_H700_SATURN_EMULATOR="$ROOT/system/yabasanshiro" \
  MPL_H700_SATURN_BIOS="$ROOT/system/saturn_bios.bin" \
  MPL_H700_SATURN_WORKDIR="$SATURN_WORKDIR" \
  MPL_H700_SATURN_CONTROL_STATE_PATH="$ROOT/system/saturn_esckey" \
  MPL_H700_SATURN_MODE="${MPL_H700_SATURN_MODE:-HLE}" \
  MPL_H700_SATURN_USE_SET_SCRIPT="${MPL_H700_SATURN_USE_SET_SCRIPT:-1}" \
  MPL_H700_SATURN_FULLSCREEN="${MPL_H700_SATURN_FULLSCREEN:-0}" \
  MPL_H700_ENABLE_SATURN="${MPL_H700_ENABLE_SATURN:-1}" \
  MPL_H700_VOLUME_PATH="$ROOT/system/openbor_volume" \
  MPL_RA_VOLUME_CFG="$ROOT/retroarch/retroarch_volume.cfg" \
  MPL_FAKE_RA_DIR="$ROOT/retroarch" \
  MPL_FAKE_REMAP_ROOT="$ROOT/remaps" \
  MPL_FAKE_LAUNCH_ARGS="$ROOT/launched.args" \
  MPL_FAKE_NDS_ARGS="$ROOT/nds.args" \
  MPL_FAKE_NDS_RUNTIME_ARGS="$ROOT/nds-runtime.args" \
  MPL_FAKE_NDS_CONTROL_PATH="$ROOT/system/nds_esckey" \
  MPL_FAKE_PSP_ARGS="$ROOT/psp.args" \
  MPL_FAKE_OPENBOR_ARGS="$ROOT/openbor.args" \
  MPL_FAKE_PORTS_ARGS="$ROOT/ports.args" \
  MPL_FAKE_PORTS_JOY_ARGS="$ROOT/ports_joy.args" \
  MPL_FAKE_PORTS_CONTROL_PATH="$ROOT/system/ports_esckey" \
  MPL_FAKE_JAVA_ARGS="$ROOT/java.args" \
  MPL_FAKE_JAVA_CONTROL_PATH="$ROOT/system/java_esckey" \
  MPL_FAKE_SATURN_ARGS="$ROOT/saturn.args" \
  MPL_FAKE_SATURN_CONTROL_PATH="$ROOT/system/saturn_esckey" \
  MPL_FAKE_AMIXER_ARGS="$ROOT/amixer.args" \
  MPL_TEST_RA_SET_VOLUME="${MPL_TEST_RA_SET_VOLUME:-}" \
  MPL_H700_CORE_GBA="${MPL_H700_CORE_GBA:-}" \
  SDL_VIDEODRIVER=mali \
  SDL_AUDIODRIVER=alsa \
  SDL_NOMOUSE=1 \
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
NDS_WORKDIR="$(cd "$ROOT/system/drastic" && pwd)"
OPENBOR_WORKDIR="$(cd "$ROOT/system" && pwd)"
PORTS_WORKDIR="$(cd "$ROOT/system" && pwd)"
RETROARCH_WORKDIR="$(cd "$ROOT/system" && pwd)"
mkdir -p "$ROOT/system/saturn"
SATURN_WORKDIR="$(cd "$ROOT/system/saturn" && pwd)"
mkdir -p "$ROOT/system/emuJava"
JAVA_WORKDIR="$(cd "$ROOT/system/emuJava" && pwd)"
printf '6\n' >"$ROOT/system/openbor_volume"
printf '0\n' >"$ROOT/system/nds_esckey"
printf '0\n' >"$ROOT/system/saturn_esckey"
printf '0\n' >"$ROOT/system/java_esckey"
printf '0\n' >"$ROOT/system/ports_esckey"
printf '6\n' >"$ROOT/volume.level"
printf 'stock_config = "kept"\n' >"$ROOT/system/retroarch-base.cfg"
printf 'core' >"$ROOT/cores/mgba_libretro.so"
printf 'core' >"$ROOT/cores/gpsp_libretro.so"
printf 'core' >"$ROOT/cores/gambatte_libretro.so"
printf 'core' >"$ROOT/cores/nestopia_libretro.so"
printf 'core' >"$ROOT/cores/mesen_libretro.so"
printf 'core' >"$ROOT/cores/snes9x_libretro.so"
printf 'core' >"$ROOT/cores/fbalpha2012_libretro.so"
printf 'core' >"$ROOT/cores/fbalpha2012_neogeo_libretro.so"
printf 'core' >"$ROOT/cores/fbalpha_libretro.so"
printf 'core' >"$ROOT/cores/fbneo_libretro.so"
printf 'core' >"$ROOT/cores/nebularm_legacy_libretro.so"
printf 'core' >"$ROOT/cores/picodrive_libretro.so"
printf 'core' >"$ROOT/cores/genesis_plus_gx_libretro.so"
printf 'core' >"$ROOT/cores/mupen64plus_next_libretro.so"
printf 'core' >"$ROOT/cores/parallel_n64_libretro.so"
printf 'core' >"$ROOT/cores/mame2022xtreme_libretro.so"
printf 'core' >"$ROOT/cores/mame2003_plus_libretro.so"
printf 'rom' >"$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
printf 'rom' >"$ROOT/mnt/sdcard/Roms/GBC/Oracle.gbc"
printf 'vertical-cps1' >"$ROOT/mnt/mmc/Roms/CPS1/1941.zip"
printf 'horizontal-cps1' >"$ROOT/mnt/mmc/Roms/CPS1/3wonders.zip"
printf 'horizontal-cps2' >"$ROOT/mnt/mmc/Roms/CPS2/1944.zip"
printf 'horizontal-cps3' >"$ROOT/mnt/mmc/Roms/CPS3/sfiii3.zip"
printf 'fds' >"$ROOT/mnt/mmc/Roms/FDS/Metroid.fds"
printf 'fbneo' >"$ROOT/mnt/mmc/Roms/FBNEO/s1945p.zip"
printf 'hbmame' >"$ROOT/mnt/mmc/Roms/HBMAME/homebrew.zip"
printf 'neogeo' >"$ROOT/mnt/mmc/Roms/NEOGEO/kof98.zip"
printf 'neogeo-bios' >"$ROOT/mnt/mmc/Roms/NEOGEO/neogeo.zip"
printf 'sfc' >"$ROOT/mnt/mmc/Roms/SFC/Mario.sfc"
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
printf '1941.zip\narkanoid.zip\n' >"$ROOT/system/varc.cfg"
mkdir -p "$ROOT/remaps/FB Alpha"
printf 'vertical-remap\n' >"$ROOT/remaps/FB Alpha/CPS1 V.rmp"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
printf '7\n' >"$ROOT/system/openbor_volume"
printf '7\n' >"$ROOT/volume.level"
mkdir -p "$ROOT/retroarch"
printf 'audio_volume = "0.0"\n' >"$ROOT/retroarch/retroarch_volume.cfg"
rm -f "$ROOT/amixer.args" "$ROOT/game-volume.db" "$ROOT/game-volume.schema" \
  "$ROOT/game-volume.frontend-level"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx 'stock_config = "kept"' "$ROOT/retroarch/retroarch.cfg"
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=mgba_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/GBA/Metroid.gba" "$ROOT/launched.args"
grep -qx 'emu=GBA' "$ROOT/launched.args"
grep -qx 'emu_dir=GBA' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch.cfg" "$ROOT/launched.args"
grep -qx "appendconfig=$ROOT/runtime/retroarch-rgfrontend.cfg" "$ROOT/launched.args"
grep -qx "cwd=$RETROARCH_WORKDIR" "$ROOT/launched.args"
grep -qx 'ld_preload=' "$ROOT/launched.args"
grep -qx 'ld_library_path=/usr/lib32:/usr/lib:/mnt/vendor/lib' "$ROOT/launched.args"
grep -qx 'rotation=0' "$ROOT/launched.args"
grep -qx -- '-q -c 0 set lineout volume 31' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set lineout volume 22' "$ROOT/amixer.args"
grep -qx 'audio_volume = "-3.0"' "$ROOT/retroarch/retroarch_volume.cfg"
test ! -e "$ROOT/game-volume.db"

rm -f "$ROOT/launched.args"
write_fake_mod_launcher
printf '%s\n' '-GBA,gpsp_libretro.so' >"$ROOT/system/CORES.txt"
MPL_H700_MOD_RA_LAUNCHER="$ROOT/system/RA_launch.sh"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
run_script
grep -qx 'argc=2' "$ROOT/launched.args"
grep -qx 'core=mgba_libretro.so' "$ROOT/launched.args"
grep -qx 'emu=GBA' "$ROOT/launched.args"
grep -qx 'emu_dir=GBA' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch_GBA.cfg" "$ROOT/launched.args"
grep -q 'launching route=mod ' "$ROOT/logs/h700-launch.log"
grep -q "runner=$ROOT/system/RA_launch_mpl.sh" "$ROOT/logs/h700-launch.log"
grep -q 'MPL_FORCE_EMU' "$ROOT/system/RA_launch_mpl.sh"
grep -Fq -- '-c "${RACONFIG}"' "$ROOT/system/RA_launch_mpl.sh"
test -s "$ROOT/system/RA_launch_mpl.sh.source.sha256"

rm -f "$ROOT/launched.args"
printf '%s\n' '-CPS1,fbalpha2012_libretro.so' >"$ROOT/system/CORES.txt"
write_request cps1 h700-retroarch-cps1 "$ROOT/mnt/mmc/Roms/CPS1/1941.zip"
run_script
grep -qx 'emu=CPS1' "$ROOT/launched.args"
grep -qx 'emu_dir=CPS1 V' "$ROOT/launched.args"
grep -qx 'varc=2' "$ROOT/launched.args"
grep -qx 'input_mode=vertical' "$ROOT/launched.args"
grep -qx 'forced_rotation=' "$ROOT/launched.args"
grep -qx 'rotation=3' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/CPS1/1941.zip" "$ROOT/launched.args"
grep -qx "arg2=$ROOT/retroarch/retroarch_CPS1 V.cfg" "$ROOT/mod-retroarch.args"
grep -qx "arg5=$ROOT/runtime/arcade-roms/CPS1 V/1941.zip" "$ROOT/mod-retroarch.args"
test ! -e "$ROOT/runtime/arcade-roms/CPS1 V/1941.zip"

rm -f "$ROOT/launched.args"
write_request cps1 h700-retroarch-cps1 "$ROOT/mnt/mmc/Roms/CPS1/3wonders.zip"
run_script
grep -qx 'emu=CPS1' "$ROOT/launched.args"
grep -qx 'emu_dir=CPS1 H' "$ROOT/launched.args"
grep -qx 'varc=1' "$ROOT/launched.args"
grep -qx 'input_mode=horizontal' "$ROOT/launched.args"
grep -qx 'forced_rotation=0' "$ROOT/launched.args"
grep -qx 'rotation=0' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/CPS1/3wonders.zip" "$ROOT/launched.args"
grep -qx "arg2=$ROOT/retroarch/retroarch_CPS1 H.cfg" "$ROOT/mod-retroarch.args"
grep -qx "arg5=$ROOT/runtime/arcade-roms/CPS1 H/3wonders.zip" "$ROOT/mod-retroarch.args"
test ! -e "$ROOT/runtime/arcade-roms/CPS1 H/3wonders.zip"

rm -f "$ROOT/launched.args"
write_request cps2 h700-retroarch-cps2 "$ROOT/mnt/mmc/Roms/CPS2/1944.zip"
run_script
grep -qx 'core=fbalpha_libretro.so' "$ROOT/launched.args"
grep -qx 'emu=CPS2' "$ROOT/launched.args"
grep -qx 'emu_dir=CPS2 H' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request cps3 h700-retroarch-cps3 "$ROOT/mnt/mmc/Roms/CPS3/sfiii3.zip"
run_script
grep -qx 'core=fbalpha_libretro.so' "$ROOT/launched.args"
grep -qx 'emu=CPS3' "$ROOT/launched.args"
grep -qx 'emu_dir=CPS3 H' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request neogeo h700-retroarch-neogeo "$ROOT/mnt/mmc/Roms/NEOGEO/kof98.zip"
run_script
grep -qx 'core=fbalpha2012_neogeo_libretro.so' "$ROOT/launched.args"
grep -qx 'emu=NEOGEO' "$ROOT/launched.args"
grep -qx 'emu_dir=NEOGEO H' "$ROOT/launched.args"
grep -qx 'neogeo_bios=present' "$ROOT/mod-retroarch.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/NEOGEO/kof98.zip" "$ROOT/launched.args"
grep -qx "launch_rom=$ROOT/runtime/arcade-roms/NEOGEO H/kof98.zip" "$ROOT/launched.args"
test ! -e "$ROOT/runtime/arcade-roms/NEOGEO H/kof98.zip"
test ! -e "$ROOT/runtime/arcade-roms/NEOGEO H/neogeo.zip"

rm -f "$ROOT/system/RA_launch.sh" "$ROOT/system/CORES.txt" \
  "$ROOT/system/RA_launch_mpl.sh" "$ROOT/system/RA_launch_mpl.sh.source.sha256" \
  "$ROOT/system/RA_launch_mpl.sh.sha256"
unset MPL_H700_MOD_RA_LAUNCHER

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
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=gambatte_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/GBC/Oracle.gbc" "$ROOT/launched.args"
grep -qx 'emu=GBC' "$ROOT/launched.args"
grep -qx 'emu_dir=GBC' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request fc_hd h700-retroarch-fc_hd \
  "$ROOT/mnt/sdcard/Roms/FC-HD/HD.nes" mesen_libretro.so
expect_failure
grep -q 'unsupported_platform platform=fc_hd' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/launched.args"
write_request md h700-retroarch-md \
  "$ROOT/mnt/sdcard/Roms/MD hack(picodrive)/Streets.zip" picodrive_libretro.so
run_script
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=picodrive_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/MD hack(picodrive)/Streets.zip" "$ROOT/launched.args"
grep -qx 'emu=MD hack(picodrive)' "$ROOT/launched.args"
grep -qx 'emu_dir=MD hack(picodrive)' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request n64 h700-retroarch-n64 "$ROOT/mnt/sdcard/Roms/N64/Smash.z64"
run_script
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=parallel_n64_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/N64/Smash.z64" "$ROOT/launched.args"
grep -qx 'emu=N64' "$ROOT/launched.args"
grep -qx 'emu_dir=N64' "$ROOT/launched.args"

rm -f "$ROOT/nds.args"
write_request nds h700-standalone-nds "$ROOT/mnt/sdcard/Roms/NDS/Ys.nds"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "cmd=savedir rom=$ROOT/mnt/sdcard/Roms/NDS/Ys.nds" "$ROOT/nds.args"
grep -qx "cmd=run rom=$ROOT/mnt/sdcard/Roms/NDS/Ys.nds" "$ROOT/nds.args"
grep -qx "cwd=$NDS_WORKDIR" "$ROOT/nds-runtime.args"
grep -qx 'control_state=1' "$ROOT/nds-runtime.args"
grep -qx 'sdl_video=' "$ROOT/nds-runtime.args"
grep -qx 'sdl_audio=' "$ROOT/nds-runtime.args"
grep -qx 'sdl_nomouse=' "$ROOT/nds-runtime.args"
grep -qx '0' "$ROOT/system/nds_esckey"

rm -f "$ROOT/psp.args" "$ROOT/amixer.args"
write_request psp h700-standalone-psp "$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "cwd=$PSP_WORKDIR" "$ROOT/psp.args"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso" "$ROOT/psp.args"
grep -qx 'ld_preload=' "$ROOT/psp.args"
grep -qx 'ld_library_path=/usr/lib32:/usr/lib:/mnt/vendor/lib' "$ROOT/psp.args"
grep -qx 'sdl_video=' "$ROOT/psp.args"
grep -qx 'sdl_audio=' "$ROOT/psp.args"
grep -qx 'sdl_nomouse=' "$ROOT/psp.args"
grep -qx -- '-q -c 0 set lineout volume 31' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"

printf 'RG28xx\n' >"$ROOT/system/board.ini"
printf 'preload' >"$ROOT/system/libSDL2-preload.so"
rm -f "$ROOT/psp.args"
write_request psp h700-standalone-psp "$ROOT/mnt/sdcard/Roms/PSP/Ridge.iso"
run_script
grep -qx "ld_preload=$ROOT/system/libSDL2-preload.so" "$ROOT/psp.args"
rm -f "$ROOT/system/board.ini" "$ROOT/system/libSDL2-preload.so"

rm -f "$ROOT/openbor.args" "$ROOT/amixer.args"
write_request openbor h700-standalone-openbor \
  "$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "setup=$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak" "$ROOT/openbor.args"
grep -qx "run=$ROOT/mnt/sdcard/Roms/OPENBOR/Final Fight.pak" "$ROOT/openbor.args"
grep -qx "cwd=$OPENBOR_WORKDIR" "$ROOT/openbor.args"
grep -qx 'ld_preload=' "$ROOT/openbor.args"
grep -qx 'ld_library_path=/usr/lib32:/usr/lib:/mnt/vendor/lib' "$ROOT/openbor.args"
grep -qx 'sdl_video=' "$ROOT/openbor.args"
grep -qx 'sdl_audio=' "$ROOT/openbor.args"
grep -qx 'sdl_nomouse=' "$ROOT/openbor.args"
grep -qx -- '-q -c 0 set lineout volume 31' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"

rm -f "$ROOT/ports.args" "$ROOT/ports_joy.args" "$ROOT/amixer.args"
write_request ports h700-standalone-ports "$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "script=$ROOT/mnt/sdcard/Roms/PORTS/Balatro.sh" "$ROOT/ports.args"
grep -qx 'ld_preload=' "$ROOT/ports.args"
grep -qx 'ld_library_path=/usr/lib32:/usr/lib:/mnt/vendor/lib' "$ROOT/ports.args"
grep -qx "cwd=$PORTS_WORKDIR" "$ROOT/ports.args"
grep -qx 'control_state=1' "$ROOT/ports.args"
grep -qx 'sdl_video=' "$ROOT/ports.args"
grep -qx 'sdl_audio=' "$ROOT/ports.args"
grep -qx 'sdl_nomouse=' "$ROOT/ports.args"
grep -qx '0' "$ROOT/system/ports_esckey"
grep -qx -- '-q -c 0 set lineout volume 16' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"
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

printf '4\n' >"$ROOT/system/openbor_volume"
rm -f "$ROOT/java.args" "$ROOT/amixer.args"
write_request java h700-standalone-java "$ROOT/mnt/sdcard/Roms/JAVA/240x320/DoomRPG.jar"
run_script
test ! -f "$ROOT/state/launch.request"
grep -qx "rom=$ROOT/mnt/sdcard/Roms/JAVA/240x320/DoomRPG.jar" "$ROOT/java.args"
grep -qx 'ld_preload=' "$ROOT/java.args"
grep -qx 'ld_library_path=/usr/lib32:/usr/lib:/mnt/vendor/lib' "$ROOT/java.args"
grep -qx "cwd=$JAVA_WORKDIR" "$ROOT/java.args"
grep -qx 'control_state=1' "$ROOT/java.args"
grep -qx 'sdl_video=' "$ROOT/java.args"
grep -qx 'sdl_audio=' "$ROOT/java.args"
grep -qx 'sdl_nomouse=' "$ROOT/java.args"
grep -qx '0' "$ROOT/system/java_esckey"
grep -qx -- '-q -c 0 set lineout volume 19' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set digital volume 63' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set LINEOUT on' "$ROOT/amixer.args"
grep -qx -- '-q -c 0 set SPK on' "$ROOT/amixer.args"

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
grep -qx 'ld_library_path=/usr/lib32:/usr/lib:/mnt/vendor/lib' "$ROOT/saturn.args"
grep -qx "cwd=$SATURN_WORKDIR" "$ROOT/saturn.args"
grep -qx 'control_state=1' "$ROOT/saturn.args"
grep -qx 'sdl_video=' "$ROOT/saturn.args"
grep -qx 'sdl_audio=' "$ROOT/saturn.args"
grep -qx 'sdl_nomouse=' "$ROOT/saturn.args"
grep -qx '0' "$ROOT/system/saturn_esckey"
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
grep -qx -- '-q -c 0 set lineout volume 25' "$ROOT/amixer.args"
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
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=mame2022xtreme_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip" "$ROOT/launched.args"
grep -qx 'emu=MAME' "$ROOT/launched.args"
grep -qx 'emu_dir=MAME H' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request mame h700-retroarch-mame \
  "$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip"
run_script
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=mame2003_plus_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/MAME ETC/glpracr3.zip" "$ROOT/launched.args"
grep -qx 'emu=MAME' "$ROOT/launched.args"
grep -qx 'emu_dir=MAME H' "$ROOT/launched.args"

assert_platform_default_core() {
  default_platform="$1"
  default_launcher="$2"
  default_rom="$3"
  expected_core="$4"
  rm -f "$ROOT/launched.args"
  write_request "$default_platform" "$default_launcher" "$default_rom"
  run_script
  grep -qx "core=$expected_core" "$ROOT/launched.args"
}

assert_platform_default_core fds h700-retroarch-fds \
  "$ROOT/mnt/mmc/Roms/FDS/Metroid.fds" nestopia_libretro.so
assert_platform_default_core fbneo h700-retroarch-fbneo \
  "$ROOT/mnt/mmc/Roms/FBNEO/s1945p.zip" fbneo_libretro.so
assert_platform_default_core hbmame h700-retroarch-hbmame \
  "$ROOT/mnt/mmc/Roms/HBMAME/homebrew.zip" nebularm_legacy_libretro.so
assert_platform_default_core neogeo h700-retroarch-neogeo \
  "$ROOT/mnt/mmc/Roms/NEOGEO/kof98.zip" fbalpha2012_neogeo_libretro.so
grep -qx 'neogeo_bios=present' "$ROOT/launched.args"
assert_platform_default_core sfc h700-retroarch-sfc \
  "$ROOT/mnt/mmc/Roms/SFC/Mario.sfc" snes9x_libretro.so

rm -f "$ROOT/launched.args"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
run_script
grep -qx 'core=mgba_libretro.so' "$ROOT/launched.args"
grep -q "runner=$ROOT/system/retroarch" "$ROOT/logs/h700-launch.log"
test ! -e "$ROOT/mnt/mod/ctrl/RA_launch.sh"

rm -f "$ROOT/launched.args"
printf 'standard' >"$ROOT/mnt/mmc/Roms/FBNEO/s1945p.zip"
write_request fbneo h700-retroarch-fbneo \
  "$ROOT/mnt/mmc/Roms/FBNEO FLY/s1945p.zip" fbneo_plus_libretro.so
run_script
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=fbneo_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/FBNEO FLY/s1945p.zip" "$ROOT/launched.args"
grep -qx 'emu=FBNEO' "$ROOT/launched.args"
grep -qx 'emu_dir=FBNEO H' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch.cfg" "$ROOT/launched.args"
grep -qx 'varc=1' "$ROOT/launched.args"
grep -qx 'content=fly' "$ROOT/launched.args"
! grep -q 'temporary rom link' "$ROOT/logs/h700-launch.log"

rm -f "$ROOT/launched.args"
write_request fbneo h700-retroarch-fbneo \
  "$ROOT/mnt/mmc/Roms/FBNEO ETC V/arkanoid.zip" fbneo_libretro.so
run_script
grep -qx 'argc=7' "$ROOT/launched.args"
grep -qx 'core=fbneo_libretro.so' "$ROOT/launched.args"
grep -qx "rom=$ROOT/mnt/mmc/Roms/FBNEO ETC V/arkanoid.zip" "$ROOT/launched.args"
grep -qx 'emu=FBNEO' "$ROOT/launched.args"
grep -qx 'emu_dir=FBNEO V' "$ROOT/launched.args"
grep -qx "raconfig=$ROOT/retroarch/retroarch.cfg" "$ROOT/launched.args"
grep -qx 'varc=2' "$ROOT/launched.args"
grep -qx 'content=vertical' "$ROOT/launched.args"

rm -f "$ROOT/launched.args"
write_request cps1 h700-retroarch-cps1 \
  "$ROOT/mnt/mmc/Roms/CPS1/1941.zip"
run_script
grep -qx 'core=fbalpha_libretro.so' "$ROOT/launched.args"
grep -qx 'varc=2' "$ROOT/launched.args"
grep -qx 'emu_dir=CPS1 V' "$ROOT/launched.args"
grep -qx 'rotation=3' "$ROOT/launched.args"
grep -qx 'remap=present' "$ROOT/launched.args"
grep -q "/runtime/arcade-roms/CPS1 V/1941.zip$" "$ROOT/launched.args"
test ! -e "$ROOT/runtime/arcade-roms/CPS1 V/1941.zip"

rm -f "$ROOT/launched.args"
write_request cps1 h700-retroarch-cps1 \
  "$ROOT/mnt/mmc/Roms/CPS1/3wonders.zip"
run_script
grep -qx 'core=fbalpha_libretro.so' "$ROOT/launched.args"
grep -qx 'varc=1' "$ROOT/launched.args"
grep -qx 'emu_dir=CPS1 H' "$ROOT/launched.args"
grep -qx 'rotation=0' "$ROOT/launched.args"
grep -qx 'remap=absent' "$ROOT/launched.args"
grep -q "/runtime/arcade-roms/CPS1 H/3wonders.zip$" "$ROOT/launched.args"
test ! -e "$ROOT/runtime/arcade-roms/CPS1 H/3wonders.zip"
grep -q 'control_folder=CPS1 H orientation=horizontal varc=1 rotation=0' "$ROOT/logs/h700-launch.log"

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

rm -f "$ROOT/system/retroarch"
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
expect_failure
grep -q 'launcher_missing' "$ROOT/logs/h700-launch.log"

write_fake_launcher
write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
MPL_H700_CORE_GBA='../bad_libretro.so' expect_failure
grep -q 'core_unconfigured' "$ROOT/logs/h700-launch.log"
unset MPL_H700_CORE_GBA

write_request gba h700-retroarch-gba "$ROOT/mnt/mmc/Roms/GBA/Metroid.gba"
MPL_H700_CORE_GBA='bad;core_libretro.so' expect_failure
grep -q 'core_unconfigured' "$ROOT/logs/h700-launch.log"
unset MPL_H700_CORE_GBA

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

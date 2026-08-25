#!/bin/sh
set -u

STATE_DIR="${MPL_STATE_DIR:-/mnt/data/multiplatform-launcher}"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REQUEST_PATH="${MPL_LAUNCH_REQUEST:-$STATE_DIR/state/launch.request}"
LOG_DIR="${MPL_LOG_DIR:-$STATE_DIR/logs}"
LOG_FILE="$LOG_DIR/h700-launch.log"
MMC_ROOT="${MPL_MMC_ROOT:-/mnt/mmc}"
SDCARD_ROOT="${MPL_SDCARD_ROOT:-/mnt/sdcard}"
MOD_RA_LAUNCHER="${MPL_H700_MOD_RA_LAUNCHER:-/mnt/mod/ctrl/RA_launch.sh}"
MPL_RA_LAUNCHER="${MPL_H700_MPL_RA_LAUNCHER:-$SCRIPT_DIR/launchers/RA_launch_mpl.sh}"
PREPARE_RA_LAUNCHER="${MPL_H700_PREPARE_RA_LAUNCHER:-$SCRIPT_DIR/prepare_ra_launcher.sh}"
RETROARCH_BIN="${MPL_H700_RA_LAUNCHER:-/mnt/vendor/deep/retro/retroarch}"
RETROARCH_WORKDIR="${MPL_H700_RA_WORKDIR:-/mnt/vendor/deep/retro}"
RETROARCH_CONFIG="${MPL_H700_RA_CONFIG:-/.config/retroarch/retroarch.cfg}"
RETROARCH_BASE_CONFIG="${MPL_H700_RA_BASE_CONFIG:-/mnt/vendor/deep/retro/retroarch.cfg}"
RETROARCH_APPEND_CONFIG="${MPL_H700_RA_APPEND_CONFIG:-$STATE_DIR/runtime/retroarch-rgfrontend.cfg}"
GAME_VOLUME_SCRIPT="${MPL_H700_GAME_VOLUME_SCRIPT:-$SCRIPT_DIR/game_volume.sh}"
CORE_DIR="${MPL_H700_CORE_DIR:-/mnt/vendor/deep/retro/cores}"
VERTICAL_ARCADE_LIST="${MPL_H700_VERTICAL_ARCADE_LIST:-/mnt/mod/ctrl/configs/varc.cfg}"
ARCADE_ROM_LINK_ROOT="${MPL_H700_ARCADE_ROM_LINK_ROOT:-$STATE_DIR/runtime/arcade-roms}"
NDS_LAUNCHER="${MPL_H700_NDS_LAUNCHER:-/mnt/vendor/ctrl/setNDS.sh}"
NDS_WORKDIR="${MPL_H700_NDS_WORKDIR:-/mnt/vendor/deep/drastic}"
NDS_CONTROL_STATE_PATH="${MPL_H700_NDS_CONTROL_STATE_PATH:-/sys/class/power_supply/axp2202-battery/nds_esckey}"
PSP_LAUNCHER="${MPL_H700_PSP_LAUNCHER:-/mnt/vendor/deep/ppsspp/PPSSPPSDL}"
PSP_BOARD_INI="${MPL_H700_BOARD_INI:-/mnt/vendor/oem/board.ini}"
PSP_SDL_PRELOAD="${MPL_H700_PSP_SDL_PRELOAD:-/mnt/vendor/sdl2/libSDL2-2.0.so.0.2800.5}"
PSP_LD_LIBRARY_PATH="${MPL_H700_PSP_LD_LIBRARY_PATH:-/usr/lib32:/usr/lib:/mnt/vendor/lib}"
OPENBOR_SETUP="${MPL_H700_OPENBOR_SETUP:-/mnt/vendor/deep/openBOR/scripts/openbor.sh}"
OPENBOR_LAUNCHER="${MPL_H700_OPENBOR_LAUNCHER:-/mnt/vendor/deep/openBOR/OpenBOR.dge}"
OPENBOR_WORKDIR="${MPL_H700_OPENBOR_WORKDIR:-/mnt/vendor/deep/openBOR}"
OPENBOR_LD_LIBRARY_PATH="${MPL_H700_OPENBOR_LD_LIBRARY_PATH:-/usr/lib32:/usr/lib:/mnt/vendor/lib}"
PORTS_SHELL="${MPL_H700_PORTS_SHELL:-/bin/bash}"
PORTS_JOY_HELPER="${MPL_H700_PORTS_JOY_HELPER:-/mnt/mod/ctrl/joy}"
PORTS_WORKDIR="${MPL_H700_PORTS_WORKDIR:-/mnt/mod/ctrl}"
PORTS_CONTROL_STATE_PATH="${MPL_H700_PORTS_CONTROL_STATE_PATH:-/sys/class/power_supply/axp2202-battery/nds_esckey}"
PORTS_LD_LIBRARY_PATH="${MPL_H700_PORTS_LD_LIBRARY_PATH:-/usr/lib32:/usr/lib:/mnt/vendor/lib}"
JAVA_LAUNCHER="${MPL_H700_JAVA_LAUNCHER:-/mnt/vendor/deep/emuJava/launch.sh}"
JAVA_WORKDIR="${MPL_H700_JAVA_WORKDIR:-/mnt/vendor/deep/emuJava}"
JAVA_CONTROL_STATE_PATH="${MPL_H700_JAVA_CONTROL_STATE_PATH:-/sys/class/power_supply/axp2202-battery/nds_esckey}"
JAVA_LD_LIBRARY_PATH="${MPL_H700_JAVA_LD_LIBRARY_PATH:-/usr/lib32:/usr/lib:/mnt/vendor/lib}"
SATURN_LAUNCHER="${MPL_H700_SATURN_LAUNCHER:-/mnt/vendor/ctrl/setSaturn.sh}"
SATURN_EMULATOR="${MPL_H700_SATURN_EMULATOR:-/emuelec/saturn/yabasanshiro}"
SATURN_BIOS="${MPL_H700_SATURN_BIOS:-/emuelec/saturn/bios/saturn_bios.bin}"
SATURN_WORKDIR="${MPL_H700_SATURN_WORKDIR:-/emuelec/saturn}"
SATURN_CONTROL_STATE_PATH="${MPL_H700_SATURN_CONTROL_STATE_PATH:-/sys/class/power_supply/axp2202-battery/nds_esckey}"
SATURN_LD_LIBRARY_PATH="${MPL_H700_SATURN_LD_LIBRARY_PATH:-/usr/lib32:/usr/lib:/mnt/vendor/lib}"
SATURN_MODE="${MPL_H700_SATURN_MODE:-HLE}"
SATURN_USE_SET_SCRIPT="${MPL_H700_SATURN_USE_SET_SCRIPT:-1}"
SATURN_FULLSCREEN="${MPL_H700_SATURN_FULLSCREEN:-0}"
SATURN_ENABLED="${MPL_H700_ENABLE_SATURN:-1}"
SYSTEM_VOLUME_PATH="${MPL_H700_VOLUME_PATH:-/sys/class/power_supply/axp2202-battery/openbor_volume}"
GBA_CORE="${MPL_H700_CORE_GBA:-mgba_libretro.so}"
GB_CORE="${MPL_H700_CORE_GB:-}"
GBC_CORE="${MPL_H700_CORE_GBC:-}"

mkdir -p "$LOG_DIR"

log_line() {
  printf '%s\n' "$*" >>"$LOG_FILE"
}

read_value() {
  key="$1"
  file="$2"
  sed -n "s/^$key=//p" "$file" | sed -n '1p'
}

hash_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    output="$(sha256sum "$1")" || return 1
    printf '%s\n' "$output" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    output="$(shasum -a 256 "$1")" || return 1
    printf '%s\n' "$output" | awk '{print $1}'
  else
    return 1
  fi
}

read_first_line() {
  sed -n '1p' "$1" 2>/dev/null || true
}

platform_folder() {
  case "$1" in
    a2600) printf 'A2600\n' ;;
    a5200) printf 'A5200\n' ;;
    a7800) printf 'A7800\n' ;;
    a800) printf 'A800\n' ;;
    amiga) printf 'AMIGA\n' ;;
    atarist) printf 'ATARIST\n' ;;
    atomiswave) printf 'ATOMISWAVE\n' ;;
    c64) printf 'C64\n' ;;
    cps1) printf 'CPS1\n' ;;
    cps2) printf 'CPS2\n' ;;
    cps3) printf 'CPS3\n' ;;
    dos) printf 'DOS\n' ;;
    dreamcast) printf 'DREAMCAST\n' ;;
    easyrpg) printf 'EASYRPG\n' ;;
    fbneo) printf 'FBNEO\n' ;;
    fc) printf 'FC\n' ;;
    fds) printf 'FDS\n' ;;
    gb) printf 'GB\n' ;;
    gbc) printf 'GBC\n' ;;
    gba) printf 'GBA\n' ;;
    gg) printf 'GG\n' ;;
    gw) printf 'GW\n' ;;
    hbmame) printf 'HBMAME\n' ;;
    java) printf 'JAVA\n' ;;
    lynx) printf 'LYNX\n' ;;
    mame) printf 'MAME\n' ;;
    md) printf 'MD\n' ;;
    mdcd) printf 'MDCD\n' ;;
    msx) printf 'MSX\n' ;;
    n64) printf 'N64\n' ;;
    nds) printf 'NDS\n' ;;
    naomi) printf 'NAOMI\n' ;;
    neocd) printf 'NEOCD\n' ;;
    neogeo) printf 'NEOGEO\n' ;;
    ngp) printf 'NGP\n' ;;
    ons) printf 'ONS\n' ;;
    openbor) printf 'OPENBOR\n' ;;
    pce) printf 'PCE\n' ;;
    pcecd) printf 'PCECD\n' ;;
    pgm2) printf 'PGM2\n' ;;
    pico) printf 'PICO\n' ;;
    poke) printf 'POKE\n' ;;
    ps) printf 'PS\n' ;;
    psp) printf 'PSP\n' ;;
    ports) printf 'PORTS\n' ;;
    saturn) printf 'SATURN\n' ;;
    scummvm) printf 'SCUMMVM\n' ;;
    sega32x) printf 'SEGA32X\n' ;;
    sfc) printf 'SFC\n' ;;
    sms) printf 'SMS\n' ;;
    varcade) printf 'VARCADE\n' ;;
    vb) printf 'VB\n' ;;
    vic20) printf 'VIC20\n' ;;
    ws) printf 'WS\n' ;;
    *) return 1 ;;
  esac
}

core_is_safe_libretro() {
  case "$1" in
    */*|*' '*|''|.*|*[!A-Za-z0-9_.-]*) return 1 ;;
    *_libretro.so) return 0 ;;
    *) return 1 ;;
  esac
}

first_existing_core() {
  for core in "$@"; do
    core_is_safe_libretro "$core" || continue
    [ -f "$CORE_DIR/$core" ] || continue
    printf '%s\n' "$core"
    return 0
  done
  return 1
}

default_core_for_platform() {
  case "$1" in
    a2600) printf 'stella2014_libretro.so\n' ;;
    a5200) printf 'a5200_libretro.so\n' ;;
    a7800) printf 'prosystem_libretro.so\n' ;;
    a800) printf 'atari800_libretro.so\n' ;;
    amiga) printf 'puae_libretro.so\n' ;;
    atarist) printf 'hatari_libretro.so\n' ;;
    atomiswave|dreamcast|naomi) printf 'flycast_libretro.so\n' ;;
    c64) printf 'vice_x64_libretro.so\n' ;;
    cps1|cps2|cps3) printf 'fbalpha_libretro.so\n' ;;
    fbneo) printf 'fbneo_libretro.so\n' ;;
    neogeo) printf 'fbalpha2012_neogeo_libretro.so\n' ;;
    dos) printf 'dosbox_pure_libretro.so\n' ;;
    easyrpg) printf 'easyrpg_libretro.so\n' ;;
    fc) printf 'fceumm_libretro.so\n' ;;
    fds) printf 'nestopia_libretro.so\n' ;;
    gb|gbc) printf 'gambatte_libretro.so\n' ;;
    gba) printf 'mgba_libretro.so\n' ;;
    gg|md|mdcd|sms) printf 'genesis_plus_gx_libretro.so\n' ;;
    gw) printf 'gw_libretro.so\n' ;;
    hbmame) printf 'nebularm_legacy_libretro.so\n' ;;
    lynx) printf 'handy_libretro.so\n' ;;
    mame) printf 'mame2003_plus_libretro.so\n' ;;
    msx) printf 'bluemsx_libretro.so\n' ;;
    n64) printf 'parallel_n64_libretro.so\n' ;;
    neocd) printf 'neocd_libretro.so\n' ;;
    ngp) printf 'mednafen_ngp_libretro.so\n' ;;
    ons) printf 'onscripter_libretro.so\n' ;;
    pce|pcecd) printf 'mednafen_pce_fast_libretro.so\n' ;;
    pgm2) printf 'mame2022xtreme_libretro.so\n' ;;
    pico) printf 'fake08_libretro.so\n' ;;
    poke) printf 'pokemini_libretro.so\n' ;;
    ps) printf 'pcsx_rearmed_libretro.so\n' ;;
    scummvm) printf 'scummvm_libretro.so\n' ;;
    sega32x) printf 'picodrive_libretro.so\n' ;;
    sfc) printf 'snes9x_libretro.so\n' ;;
    varcade) printf 'fbneo_libretro.so\n' ;;
    vb) printf 'mednafen_vb_libretro.so\n' ;;
    vic20) printf 'vice_xvic_libretro.so\n' ;;
    ws) printf 'mednafen_wswan_libretro.so\n' ;;
    *) return 1 ;;
  esac
}

mixer_value_for_level() {
  level="$1"
  case "$level" in
    ''|*[!0-9]*) level=6 ;;
  esac
  [ "$level" -lt 0 ] && level=0
  [ "$level" -gt 10 ] && level=10
  if [ "$level" -eq 0 ]; then
    printf '0\n'
  else
    printf '%s\n' $(((level * 31 + 5) / 10))
  fi
}

current_volume_level() {
  level=""
  if [ -r "$SYSTEM_VOLUME_PATH" ]; then
    level="$(sed -n '1p' "$SYSTEM_VOLUME_PATH" 2>/dev/null || true)"
  fi
  if [ -z "$level" ] && [ -r "$STATE_DIR/volume.level" ]; then
    level="$(sed -n '1p' "$STATE_DIR/volume.level" 2>/dev/null || true)"
  fi
  case "$level" in
    ''|*[!0-9]*) level=6 ;;
  esac
  [ "$level" -lt 0 ] && level=0
  [ "$level" -gt 10 ] && level=10
  printf '%s\n' "$level"
}

release_game_audio() {
  command -v amixer >/dev/null 2>&1 || return 0
  unset LD_PRELOAD
  volume_level="$(current_volume_level)"
  lineout_level="$(mixer_value_for_level "$volume_level")"
  amixer -q -c 0 set "lineout volume" "$lineout_level" >/dev/null 2>&1 || true
  if [ "$volume_level" -eq 0 ]; then
    amixer -q -c 0 set "digital volume" 0 >/dev/null 2>&1 || true
    amixer -q -c 0 set "SPK" off >/dev/null 2>&1 || true
    log_line "[launch] released game audio muted volume=0 lineout=0 digital=0"
  else
    amixer -q -c 0 set "digital volume" 63 >/dev/null 2>&1 || true
    amixer -q -c 0 set "LINEOUT" on >/dev/null 2>&1 || true
    amixer -q -c 0 set "SPK" on >/dev/null 2>&1 || true
    log_line "[launch] released game audio volume=$volume_level lineout=$lineout_level digital=63"
  fi
}

set_full_game_hardware_volume() {
  command -v amixer >/dev/null 2>&1 || return 0
  unset LD_PRELOAD
  amixer -q -c 0 set "lineout volume" 31 >/dev/null 2>&1 &&
    amixer -q -c 0 set "digital volume" 63 >/dev/null 2>&1 &&
    amixer -q -c 0 set "LINEOUT" on >/dev/null 2>&1 &&
    amixer -q -c 0 set "SPK" on >/dev/null 2>&1
}

ports_control_mixer_value_for_level() {
  case "$1" in
    0) printf '0\n' ;;
    1) printf '5\n' ;;
    2) printf '10\n' ;;
    3) printf '16\n' ;;
    4) printf '19\n' ;;
    5) printf '21\n' ;;
    6) printf '23\n' ;;
    7) printf '25\n' ;;
    8) printf '27\n' ;;
    9) printf '29\n' ;;
    10) printf '31\n' ;;
    *) printf '23\n' ;;
  esac
}

prepare_ports_controlled_game_audio() {
  command -v amixer >/dev/null 2>&1 || return 0
  unset LD_PRELOAD
  ports_volume_level="$(current_volume_level)"
  ports_lineout_level="$(ports_control_mixer_value_for_level "$ports_volume_level")"
  amixer -q -c 0 set "lineout volume" "$ports_lineout_level" >/dev/null 2>&1 || return 1
  if [ "$ports_volume_level" -eq 0 ]; then
    amixer -q -c 0 set "digital volume" 0 >/dev/null 2>&1 || true
    amixer -q -c 0 set "SPK" off >/dev/null 2>&1 || true
  else
    amixer -q -c 0 set "digital volume" 63 >/dev/null 2>&1 || true
    amixer -q -c 0 set "LINEOUT" on >/dev/null 2>&1 || true
    amixer -q -c 0 set "SPK" on >/dev/null 2>&1 || true
  fi
  log_line "[launch] prepared ports-controlled game audio volume=$ports_volume_level lineout=$ports_lineout_level"
}

prepare_retroarch_game_audio() {
  [ -x "$GAME_VOLUME_SCRIPT" ] || return 1
  MPL_STATE_DIR="$STATE_DIR" "$GAME_VOLUME_SCRIPT" prepare || return 1
  set_full_game_hardware_volume || return 1
  log_line "[launch] prepared retroarch game audio hardware=lineout31 software=audio_volume"
  return 0
}

mapped_core_hint() {
  platform="$1"
  requested="$2"
  [ -n "$requested" ] || return 1
  core_is_safe_libretro "$requested" || return 1
  case "$platform:$requested" in
    fbneo:fbneo_plus_libretro.so)
      first_existing_core \
        fbneo_libretro.so \
        fbneo_G_libretro.so \
        fbneo_oem_libretro.so
      ;;
    fbneo:fbneo_libretro.so|fbneo:fbneo_G_libretro.so|fbneo:fbneo_oem_libretro.so|\
    fbneo:fbalpha_libretro.so|fbneo:fbalpha2012_libretro.so|fbneo:mame2003_xtreme_libretro.so|\
    fbneo:mame2003_plus_libretro.so|fbneo:mame2010_libretro.so|fbneo:mame2000_libretro.so|\
    cps1:fbneo_libretro.so|cps1:fbalpha_libretro.so|cps1:fbalpha2012_libretro.so|cps1:mame2003_xtreme_libretro.so|\
    cps1:mame2003_plus_libretro.so|cps1:mame2010_libretro.so|cps1:mame2000_libretro.so|\
    cps2:fbneo_libretro.so|cps2:fbalpha_libretro.so|cps2:fbalpha2012_libretro.so|cps2:mame2003_xtreme_libretro.so|\
    cps2:mame2003_plus_libretro.so|cps2:mame2010_libretro.so|cps2:mame2000_libretro.so|\
    cps3:fbneo_libretro.so|cps3:fbalpha_libretro.so|cps3:fbalpha2012_libretro.so|cps3:mame2003_xtreme_libretro.so|\
    cps3:mame2003_plus_libretro.so|cps3:mame2010_libretro.so|cps3:mame2000_libretro.so|\
    neogeo:fbneo_libretro.so|neogeo:fbalpha_libretro.so|neogeo:fbalpha2012_libretro.so|neogeo:mame2003_xtreme_libretro.so|\
    neogeo:mame2003_plus_libretro.so|neogeo:mame2010_libretro.so|neogeo:mame2000_libretro.so)
      printf '%s\n' "$requested"
      ;;
    gba:mgba_libretro.so|gba:gpsp_libretro.so|gba:vbam_libretro.so|gba:vba_next_libretro.so)
      printf '%s\n' "$requested"
      ;;
    gb:gambatte_libretro.so|gb:tgbdual_libretro.so|gb:gearboy_libretro.so|gb:sameboy_libretro.so)
      printf '%s\n' "$requested"
      ;;
    gbc:gambatte_libretro.so|gbc:tgbdual_libretro.so|gbc:gearboy_libretro.so|gbc:sameboy_libretro.so)
      printf '%s\n' "$requested"
      ;;
    fc:fceumm_libretro.so|fc:nestopia_libretro.so|fc:mesen_libretro.so|fc:quicknes_libretro.so|\
    fds:fceumm_libretro.so|fds:nestopia_libretro.so|fds:mesen_libretro.so|fds:quicknes_libretro.so)
      printf '%s\n' "$requested"
      ;;
    sfc:snes9x2005_plus_libretro.so|sfc:snes9x_libretro.so|sfc:snes9x2002_libretro.so|\
    sfc:snes9x2005_libretro.so|sfc:snes9x2010_libretro.so)
      printf '%s\n' "$requested"
      ;;
    md:picodrive_libretro.so|md:genesis_plus_gx_libretro.so|\
    mdcd:picodrive_libretro.so|mdcd:genesis_plus_gx_libretro.so|\
    sms:picodrive_libretro.so|sms:genesis_plus_gx_libretro.so|\
    gg:picodrive_libretro.so|gg:genesis_plus_gx_libretro.so|\
    sega32x:picodrive_libretro.so|sega32x:genesis_plus_gx_libretro.so)
      printf '%s\n' "$requested"
      ;;
    ps:pcsx_rearmed_libretro.so|ps:pcsx_rearmed_peops_libretro.so|\
    ps:pcsx_rearmed_rumble_libretro.so|ps:swanstation_libretro.so)
      printf '%s\n' "$requested"
      ;;
    n64:mupen64plus_next_libretro.so|n64:parallel_n64_libretro.so)
      printf '%s\n' "$requested"
      ;;
    mame:mame2022xtreme_libretro.so|mame:mame2003_xtreme_libretro.so|\
    mame:mame2003_plus_libretro.so|mame:mame2010_libretro.so|mame:mame2000_libretro.so|\
    mame:fbneo_libretro.so|mame:fbalpha2012_libretro.so|\
    varcade:mame2022xtreme_libretro.so|varcade:mame2003_xtreme_libretro.so|\
    varcade:mame2003_plus_libretro.so|varcade:mame2010_libretro.so|varcade:mame2000_libretro.so|\
    varcade:fbneo_libretro.so|varcade:fbalpha2012_libretro.so)
      printf '%s\n' "$requested"
      ;;
    *)
      return 1
      ;;
  esac
}

platform_core() {
  platform="$1"
  requested="${2:-}"
  hinted_core="$(mapped_core_hint "$platform" "$requested" || true)"
  if [ -n "$hinted_core" ]; then
    printf '%s\n' "$hinted_core"
    return 0
  fi

  case "$platform" in
    gb) core="${GB_CORE:-$(default_core_for_platform "$platform")}" ;;
    gbc) core="${GBC_CORE:-$(default_core_for_platform "$platform")}" ;;
    gba) core="${GBA_CORE:-$(default_core_for_platform "$platform")}" ;;
    *) core="$(default_core_for_platform "$platform" || true)" ;;
  esac
  [ -n "$core" ] || return 1
  core_is_safe_libretro "$core" || return 1
  printf '%s\n' "$core"
}

expected_launcher_id() {
  case "$1" in
    nds)
      printf 'h700-standalone-nds\n'
      ;;
    psp)
      printf 'h700-standalone-psp\n'
      ;;
    openbor)
      printf 'h700-standalone-openbor\n'
      ;;
    ports)
      printf 'h700-standalone-ports\n'
      ;;
    java)
      printf 'h700-standalone-java\n'
      ;;
    saturn)
      printf 'h700-standalone-saturn\n'
      ;;
    *)
      platform_folder "$1" >/dev/null || return 1
      printf 'h700-retroarch-%s\n' "$1"
      ;;
  esac
}

is_trusted_rom_path() {
  rom="$1"
  rom_dir="${rom%/*}"
  rom_base="${rom##*/}"
  [ -d "$rom_dir" ] || return 1
  canonical_rom_dir="$(cd -P "$rom_dir" 2>/dev/null && pwd -P)" || return 1
  canonical_rom="$canonical_rom_dir/$rom_base"
  for root in "$MMC_ROOT/Roms" "$SDCARD_ROOT/Roms"; do
    [ -d "$root" ] || continue
    canonical_root="$(cd -P "$root" 2>/dev/null && pwd -P)" || continue
    case "$canonical_rom" in
      "$canonical_root"/*) return 0 ;;
    esac
  done
  case "$rom" in
    "$MMC_ROOT"/Roms/*|"$SDCARD_ROOT"/Roms/*) return 0 ;;
    *) return 1 ;;
  esac
}

rom_collection_folder() {
  rom="$1"
  for root in "$MMC_ROOT/Roms" "$SDCARD_ROOT/Roms"; do
    case "$rom" in
      "$root"/*)
        relative="${rom#"$root"/}"
        printf '%s\n' "${relative%%/*}"
        return 0
        ;;
    esac
  done
  return 1
}

fbneo_collection_folder() {
  collection="$(rom_collection_folder "$1" || true)"
  case "$collection" in
    FBNEO|FBNEO\ *)
      printf '%s\n' "$collection"
      return 0
      ;;
  esac
  return 1
}

is_fbneo_vertical_collection() {
  collection="$1"
  case "$collection" in
    FBNEO*\ V) return 0 ;;
    *) return 1 ;;
  esac
}

is_varc_arcade_platform() {
  case "$1" in
    cps1|cps2|cps3|fbneo|hbmame|mame|neogeo|pgm2) return 0 ;;
    *) return 1 ;;
  esac
}

arcade_orientation() {
  platform="$1"
  rom="$2"
  if [ "$platform" = "varcade" ]; then
    printf 'vertical\n'
    return 0
  fi
  is_varc_arcade_platform "$platform" || return 1

  if [ "$platform" = "fbneo" ]; then
    collection="$(fbneo_collection_folder "$rom" || true)"
    if [ -n "$collection" ] && is_fbneo_vertical_collection "$collection"; then
      printf 'vertical\n'
      return 0
    fi
  fi

  [ -r "$VERTICAL_ARCADE_LIST" ] || return 1
  filename="${rom##*/}"
  if grep -Fqx "$filename" "$VERTICAL_ARCADE_LIST"; then
    printf 'vertical\n'
  else
    printf 'horizontal\n'
  fi
}

arcade_control_folder() {
  config_folder="$1"
  orientation="$2"
  case "$orientation" in
    horizontal) printf '%s H\n' "$config_folder" ;;
    vertical)
      case "$config_folder" in
        *\ V) printf '%s\n' "$config_folder" ;;
        *) printf '%s V\n' "$config_folder" ;;
      esac
      ;;
    *) printf '%s\n' "$config_folder" ;;
  esac
}

prepare_arcade_launch_rom() {
  rom="$1"
  control_folder="$2"
  case "$control_folder" in
    ''|.|..|*/*) return 1 ;;
  esac
  filename="${rom##*/}"
  [ -n "$filename" ] || return 1
  source_dir="${rom%/*}"
  link_dir="$ARCADE_ROM_LINK_ROOT/$control_folder"
  link_path="$link_dir/$filename"
  mkdir -p "$ARCADE_ROM_LINK_ROOT" || return 1
  if [ -L "$link_dir" ]; then
    rm -f "$link_dir" || return 1
  elif [ -d "$link_dir" ]; then
    find "$link_dir" -mindepth 1 -maxdepth 1 -type l -exec rm -f {} \; 2>/dev/null
    rmdir "$link_dir" 2>/dev/null || return 1
  elif [ -e "$link_dir" ]; then
    return 1
  fi
  ln -s "$source_dir" "$link_dir" || return 1
  [ -f "$link_path" ] || {
    rm -f "$link_dir"
    return 1
  }
  printf '%s\n' "$link_path"
}

cleanup_arcade_launch_rom() {
  link_path="$1"
  [ -n "$link_path" ] || return 0
  case "$link_path" in
    "$ARCADE_ROM_LINK_ROOT"/*)
      link_dir="${link_path%/*}"
      [ -L "$link_dir" ] && rm -f "$link_dir"
      ;;
  esac
}

ra_config_folder() {
  platform="$1"
  fallback="$2"
  rom="$3"
  case "$platform" in
    fbneo)
      collection="$(fbneo_collection_folder "$rom" || true)"
      if [ -n "$collection" ]; then
        if is_fbneo_vertical_collection "$collection"; then
          printf 'FBNEO V\n'
        else
          printf 'FBNEO\n'
        fi
        return 0
      fi
      ;;
  esac
  printf '%s\n' "$fallback"
}

forced_varc_mode() {
  platform="$1"
  rom="$2"
  orientation="$(arcade_orientation "$platform" "$rom" || true)"
  case "$orientation" in
    horizontal) printf '1\n' ;;
    vertical)
      case "$platform" in
        varcade) printf '3\n' ;;
        hbmame) printf '4\n' ;;
        *) printf '2\n' ;;
      esac
      ;;
    *) return 1 ;;
  esac
}

forced_video_rotation() {
  platform="$1"
  rom="$2"
  if [ "$(arcade_orientation "$platform" "$rom" || true)" = "vertical" ]; then
    printf '3\n'
  else
    printf '0\n'
  fi
}

ensure_retroarch_config() {
  [ -r "$RETROARCH_CONFIG" ] && return 0
  [ -r "$RETROARCH_BASE_CONFIG" ] || return 1
  mkdir -p "${RETROARCH_CONFIG%/*}" || return 1
  cp "$RETROARCH_BASE_CONFIG" "$RETROARCH_CONFIG"
}

prepare_retroarch_append_config() {
  rotation="$1"
  mkdir -p "${RETROARCH_APPEND_CONFIG%/*}" || return 1
  temporary="$RETROARCH_APPEND_CONFIG.tmp.$$"
  printf 'video_rotation = "%s"\n' "$rotation" >"$temporary" || {
    rm -f "$temporary"
    return 1
  }
  mv -f "$temporary" "$RETROARCH_APPEND_CONFIG"
  printf '%s\n' "$RETROARCH_APPEND_CONFIG"
}

launch_nds() {
  rom_path="$1"
  game_id="$2"
  [ -x "$NDS_LAUNCHER" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=nds path=$NDS_LAUNCHER"
    return 15
  }
  [ -d "$NDS_WORKDIR" ] || {
    log_line "[launch] rejected standalone_workdir_missing platform=nds path=$NDS_WORKDIR"
    return 15
  }
  [ -w "$NDS_CONTROL_STATE_PATH" ] || {
    log_line "[launch] rejected nds_control_state_unwritable platform=nds path=$NDS_CONTROL_STATE_PATH"
    return 15
  }

  log_line "[launch] launching platform=nds launcher=h700-standalone-nds game=$game_id rom=$rom_path runner=$NDS_LAUNCHER"
  release_game_audio
  export LD_LIBRARY_PATH=/usr/lib32:/usr/lib:/mnt/vendor/lib
  unset LD_PRELOAD
  "$NDS_LAUNCHER" savedir "$rom_path" >>"$LOG_FILE" 2>&1
  savedir_rc=$?
  if [ "$savedir_rc" -ne 0 ]; then
    log_line "[launch] failed rc=$savedir_rc platform=nds game=$game_id stage=savedir"
    return "$savedir_rc"
  fi
  # The stock dmenu enables this transient kernel-driver state before it hands
  # off to setNDS.sh. ndsCtrl.dge exits immediately unless nds_esckey reads 1.
  if ! printf '1\n' >"$NDS_CONTROL_STATE_PATH"; then
    log_line "[launch] failed rc=15 platform=nds game=$game_id stage=control-enable"
    return 15
  fi
  log_line "[launch] enabled nds control state path=$NDS_CONTROL_STATE_PATH"

  # Match the stock dmenu /tmp/.next context. Do not leak the frontend's SDL
  # environment into the 32-bit stock helper or emulator.
  (
    cd "$NDS_WORKDIR" || exit 15
    unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
    "$NDS_LAUNCHER" run "$rom_path"
  ) >>"$LOG_FILE" 2>&1
  rc=$?
  if printf '0\n' >"$NDS_CONTROL_STATE_PATH"; then
    log_line "[launch] disabled nds control state path=$NDS_CONTROL_STATE_PATH"
  else
    log_line "[launch] warning nds_control_state_reset_failed path=$NDS_CONTROL_STATE_PATH"
  fi
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=nds game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=nds game=$game_id stage=run"
  fi
  return "$rc"
}

psp_needs_sdl_preload() {
  [ -f "$PSP_BOARD_INI" ] || return 1
  [ "$(sed -n '1p' "$PSP_BOARD_INI" 2>/dev/null)" = "RG28xx" ] || return 1
  [ -f "$PSP_SDL_PRELOAD" ] || return 1
}

launch_psp() {
  rom_path="$1"
  game_id="$2"
  [ -x "$PSP_LAUNCHER" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=psp path=$PSP_LAUNCHER"
    return 15
  }
  psp_launcher_dir="${PSP_LAUNCHER%/*}"
  psp_launcher_name="${PSP_LAUNCHER##*/}"
  [ -d "$psp_launcher_dir" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=psp dir=$psp_launcher_dir"
    return 15
  }

  log_line "[launch] launching platform=psp launcher=h700-standalone-psp game=$game_id rom=$rom_path runner=$PSP_LAUNCHER"
  # Stock PPSSPP keeps the hardware path at full output and implements the
  # openbor_volume scale in software. Applying the frontend lineout scale here
  # attenuates the audio a second time and can make the game effectively mute.
  if set_full_game_hardware_volume; then
    log_line "[launch] prepared psp game audio hardware=lineout31 software=ppsspp"
  else
    log_line "[launch] warning psp_game_audio_prepare_failed"
  fi
  unset LD_PRELOAD
  if psp_needs_sdl_preload; then
    export LD_PRELOAD="$PSP_SDL_PRELOAD"
    log_line "[launch] psp preload=$PSP_SDL_PRELOAD"
  fi
  (
    cd "$psp_launcher_dir" || exit 15
    # Match the stock dmenu /tmp/.next environment. PPSSPP owns its SDL input,
    # audio and video setup; frontend-specific SDL variables break that path.
    unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
    export LD_LIBRARY_PATH="$PSP_LD_LIBRARY_PATH"
    "./$psp_launcher_name" "$rom_path"
  ) >>"$LOG_FILE" 2>&1
  rc=$?
  unset LD_PRELOAD
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=psp game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=psp game=$game_id"
  fi
  return "$rc"
}

launch_openbor() {
  rom_path="$1"
  game_id="$2"
  [ -x "$OPENBOR_SETUP" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=openbor path=$OPENBOR_SETUP"
    return 15
  }
  [ -x "$OPENBOR_LAUNCHER" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=openbor path=$OPENBOR_LAUNCHER"
    return 15
  }
  [ -d "$OPENBOR_WORKDIR" ] || {
    log_line "[launch] rejected standalone_workdir_missing platform=openbor path=$OPENBOR_WORKDIR"
    return 15
  }

  log_line "[launch] launching platform=openbor launcher=h700-standalone-openbor game=$game_id rom=$rom_path setup=$OPENBOR_SETUP runner=$OPENBOR_LAUNCHER"
  # Stock OpenBOR keeps the hardware path at full output and applies the
  # openbor_volume scale internally, just like PPSSPP. Frontend hardware
  # attenuation here would reduce the same logical volume a second time.
  if set_full_game_hardware_volume; then
    log_line "[launch] prepared openbor game audio hardware=lineout31 software=openbor"
  else
    log_line "[launch] warning openbor_game_audio_prepare_failed"
  fi
  unset LD_PRELOAD
  "$OPENBOR_SETUP" "$rom_path" >>"$LOG_FILE" 2>&1
  setup_rc=$?
  if [ "$setup_rc" -ne 0 ]; then
    log_line "[launch] failed rc=$setup_rc platform=openbor game=$game_id stage=setup"
    return "$setup_rc"
  fi
  (
    cd "$OPENBOR_WORKDIR" || exit 15
    unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
    export LD_LIBRARY_PATH="$OPENBOR_LD_LIBRARY_PATH"
    "$OPENBOR_LAUNCHER" "$rom_path"
  ) >>"$LOG_FILE" 2>&1
  rc=$?
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=openbor game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=openbor game=$game_id stage=run"
  fi
  return "$rc"
}

ports_needs_joy_helper() {
  [ -f "$PSP_BOARD_INI" ] || return 1
  case "$(sed -n '1p' "$PSP_BOARD_INI" 2>/dev/null)" in
    RG28xx|RG35xxSP|RG35xxP|RG35xx+_P|RG34xx) return 0 ;;
    *) return 1 ;;
  esac
}

launch_ports() {
  rom_path="$1"
  game_id="$2"
  [ -x "$PORTS_SHELL" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=ports path=$PORTS_SHELL"
    return 15
  }
  case "$rom_path" in
    *.sh|*.SH) ;;
    *)
      log_line "[launch] rejected unsupported_extension platform=ports rom=$rom_path"
      return 16
      ;;
  esac
  [ -d "$PORTS_WORKDIR" ] || {
    log_line "[launch] rejected standalone_workdir_missing platform=ports path=$PORTS_WORKDIR"
    return 15
  }
  [ -w "$PORTS_CONTROL_STATE_PATH" ] || {
    log_line "[launch] rejected ports_control_state_unwritable platform=ports path=$PORTS_CONTROL_STATE_PATH"
    return 15
  }

  log_line "[launch] launching platform=ports launcher=h700-standalone-ports game=$game_id rom=$rom_path runner=$PORTS_SHELL"
  prepare_ports_controlled_game_audio || log_line "[launch] warning ports_game_audio_prepare_failed"
  if ! printf '1\n' >"$PORTS_CONTROL_STATE_PATH"; then
    log_line "[launch] failed rc=15 platform=ports game=$game_id stage=control-enable"
    return 15
  fi
  log_line "[launch] enabled ports control state path=$PORTS_CONTROL_STATE_PATH"
  unset LD_PRELOAD
  if ports_needs_joy_helper; then
    if [ -x "$PORTS_JOY_HELPER" ]; then
      (
        cd "$PORTS_WORKDIR" || exit 15
        unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
        export LD_LIBRARY_PATH="$PORTS_LD_LIBRARY_PATH"
        "$PORTS_JOY_HELPER"
      ) >>"$LOG_FILE" 2>&1 &
      log_line "[launch] ports joy_helper=$PORTS_JOY_HELPER"
    else
      log_line "[launch] ports joy_helper_missing path=$PORTS_JOY_HELPER"
    fi
  fi
  (
    cd "$PORTS_WORKDIR" || exit 15
    unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
    export LD_LIBRARY_PATH="$PORTS_LD_LIBRARY_PATH"
    "$PORTS_SHELL" "$rom_path"
  ) >>"$LOG_FILE" 2>&1
  rc=$?
  if printf '0\n' >"$PORTS_CONTROL_STATE_PATH"; then
    log_line "[launch] disabled ports control state path=$PORTS_CONTROL_STATE_PATH"
  else
    log_line "[launch] warning ports_control_state_reset_failed path=$PORTS_CONTROL_STATE_PATH"
  fi
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=ports game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=ports game=$game_id"
  fi
  return "$rc"
}

launch_java() {
  rom_path="$1"
  game_id="$2"
  [ -x "$JAVA_LAUNCHER" ] || {
    log_line "[launch] rejected standalone_launcher_missing platform=java path=$JAVA_LAUNCHER"
    return 15
  }
  case "$rom_path" in
    *.jar|*.JAR) ;;
    *)
      log_line "[launch] rejected unsupported_extension platform=java rom=$rom_path"
      return 16
      ;;
  esac
  [ -d "$JAVA_WORKDIR" ] || {
    log_line "[launch] rejected standalone_workdir_missing platform=java path=$JAVA_WORKDIR"
    return 15
  }
  [ -w "$JAVA_CONTROL_STATE_PATH" ] || {
    log_line "[launch] rejected java_control_state_unwritable platform=java path=$JAVA_CONTROL_STATE_PATH"
    return 15
  }

  log_line "[launch] launching platform=java launcher=h700-standalone-java game=$game_id rom=$rom_path runner=$JAVA_LAUNCHER"
  prepare_ports_controlled_game_audio || log_line "[launch] warning java_game_audio_prepare_failed"
  if ! printf '1\n' >"$JAVA_CONTROL_STATE_PATH"; then
    log_line "[launch] failed rc=15 platform=java game=$game_id stage=control-enable"
    return 15
  fi
  log_line "[launch] enabled java control state path=$JAVA_CONTROL_STATE_PATH"
  unset LD_PRELOAD
  (
    cd "$JAVA_WORKDIR" || exit 15
    # Match the stock dmenu context. launch.sh starts portsCtrl.dge after it
    # enters the JDK directory; frontend SDL variables break that helper path.
    unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
    export LD_LIBRARY_PATH="$JAVA_LD_LIBRARY_PATH"
    "$JAVA_LAUNCHER" "$rom_path"
  ) >>"$LOG_FILE" 2>&1
  rc=$?
  if printf '0\n' >"$JAVA_CONTROL_STATE_PATH"; then
    log_line "[launch] disabled java control state path=$JAVA_CONTROL_STATE_PATH"
  else
    log_line "[launch] warning java_control_state_reset_failed path=$JAVA_CONTROL_STATE_PATH"
  fi
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=java game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=java game=$game_id"
  fi
  return "$rc"
}

launch_saturn() {
  rom_path="$1"
  game_id="$2"
  [ "$SATURN_ENABLED" = "1" ] || {
    log_line "[launch] rejected platform_disabled platform=saturn"
    return 17
  }
  case "$rom_path" in
    *.bin|*.BIN|*.cue|*.CUE|*.iso|*.ISO|*.mds|*.MDS|*.ccd|*.CCD|*.chd|*.CHD|*.rar|*.RAR|*.m3u|*.M3U) ;;
    *)
      log_line "[launch] rejected unsupported_extension platform=saturn rom=$rom_path"
      return 16
      ;;
  esac
  case "$SATURN_MODE" in
    HLE|BIOS) ;;
    *) SATURN_MODE=BIOS ;;
  esac
  [ -d "$SATURN_WORKDIR" ] || {
    log_line "[launch] rejected standalone_workdir_missing platform=saturn path=$SATURN_WORKDIR"
    return 15
  }
  [ -w "$SATURN_CONTROL_STATE_PATH" ] || {
    log_line "[launch] rejected saturn_control_state_unwritable platform=saturn path=$SATURN_CONTROL_STATE_PATH"
    return 15
  }

  if [ "$SATURN_USE_SET_SCRIPT" = "1" ]; then
    [ -x "$SATURN_LAUNCHER" ] || {
      log_line "[launch] rejected standalone_launcher_missing platform=saturn path=$SATURN_LAUNCHER"
      return 15
    }
    log_line "[launch] launching platform=saturn launcher=h700-standalone-saturn game=$game_id rom=$rom_path runner=$SATURN_LAUNCHER mode=$SATURN_MODE"
    prepare_ports_controlled_game_audio || log_line "[launch] warning saturn_game_audio_prepare_failed"
    if ! printf '1\n' >"$SATURN_CONTROL_STATE_PATH"; then
      log_line "[launch] failed rc=15 platform=saturn game=$game_id stage=control-enable"
      return 15
    fi
    log_line "[launch] enabled saturn control state path=$SATURN_CONTROL_STATE_PATH"
    unset LD_PRELOAD
    (
      cd "$SATURN_WORKDIR" || exit 15
      unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
      export LD_LIBRARY_PATH="$SATURN_LD_LIBRARY_PATH"
      "$SATURN_LAUNCHER" "$SATURN_MODE" "$rom_path"
    ) >>"$LOG_FILE" 2>&1
    rc=$?
  else
    [ -x "$SATURN_EMULATOR" ] || {
      log_line "[launch] rejected standalone_launcher_missing platform=saturn path=$SATURN_EMULATOR"
      return 15
    }
    if [ "$SATURN_MODE" = "BIOS" ] && [ ! -f "$SATURN_BIOS" ]; then
      log_line "[launch] rejected standalone_launcher_missing platform=saturn path=$SATURN_BIOS"
      return 15
    fi
    home_dir="$MMC_ROOT"
    case "$rom_path" in
      "$SDCARD_ROOT"/*) home_dir="$SDCARD_ROOT" ;;
    esac
    case "$SATURN_FULLSCREEN" in
      1|true|TRUE|yes|YES) SATURN_FULLSCREEN=1 ;;
      *) SATURN_FULLSCREEN=0 ;;
    esac
    fullscreen_arg=
    [ "$SATURN_FULLSCREEN" = "1" ] && fullscreen_arg="-f"
    log_line "[launch] launching platform=saturn launcher=h700-standalone-saturn game=$game_id rom=$rom_path runner=$SATURN_EMULATOR mode=$SATURN_MODE autostart=1 fullscreen=$SATURN_FULLSCREEN"
    prepare_ports_controlled_game_audio || log_line "[launch] warning saturn_game_audio_prepare_failed"
    if ! printf '1\n' >"$SATURN_CONTROL_STATE_PATH"; then
      log_line "[launch] failed rc=15 platform=saturn game=$game_id stage=control-enable"
      return 15
    fi
    log_line "[launch] enabled saturn control state path=$SATURN_CONTROL_STATE_PATH"
    if ! ps -A | grep "portsCtrl.dge" >/dev/null 2>&1; then
      if [ -x "$MMC_ROOT/portsCtrl.dge" ]; then
        "$MMC_ROOT/portsCtrl.dge" >>"$LOG_FILE" 2>&1 &
      elif [ -x /mnt/vendor/bin/portsCtrl.dge ]; then
        /mnt/vendor/bin/portsCtrl.dge >>"$LOG_FILE" 2>&1 &
      fi
    fi
    unset LD_PRELOAD
    unset SDL_VIDEODRIVER SDL_AUDIODRIVER SDL_NOMOUSE
    export LD_LIBRARY_PATH="$SATURN_LD_LIBRARY_PATH"
    display_id="$(cat /sys/class/power_supply/axp2202-battery/display_id 2>/dev/null || true)"
    audio_prefix=
    [ "$display_id" = "1" ] && audio_prefix="AUDIODEV=hw:2,0"
    if [ "$SATURN_MODE" = "BIOS" ] && [ -n "$audio_prefix" ]; then
      HOME="$home_dir" AUDIODEV=hw:2,0 "$SATURN_EMULATOR" -r 3 -i "$rom_path" -b "$SATURN_BIOS" -a $fullscreen_arg >>"$LOG_FILE" 2>&1
    elif [ "$SATURN_MODE" = "BIOS" ]; then
      HOME="$home_dir" "$SATURN_EMULATOR" -r 3 -i "$rom_path" -b "$SATURN_BIOS" -a $fullscreen_arg >>"$LOG_FILE" 2>&1
    elif [ -n "$audio_prefix" ]; then
      HOME="$home_dir" AUDIODEV=hw:2,0 "$SATURN_EMULATOR" -r 3 -i "$rom_path" -a $fullscreen_arg >>"$LOG_FILE" 2>&1
    else
      HOME="$home_dir" "$SATURN_EMULATOR" -r 3 -i "$rom_path" -a $fullscreen_arg >>"$LOG_FILE" 2>&1
    fi
    rc=$?
  fi
  if printf '0\n' >"$SATURN_CONTROL_STATE_PATH"; then
    log_line "[launch] disabled saturn control state path=$SATURN_CONTROL_STATE_PATH"
  else
    log_line "[launch] warning saturn_control_state_reset_failed path=$SATURN_CONTROL_STATE_PATH"
  fi
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=saturn game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=saturn game=$game_id"
  fi
  return "$rc"
}

prepare_private_ra_launcher() {
  [ -x "$PREPARE_RA_LAUNCHER" ] || return 1
  source_hash="$(hash_file "$MOD_RA_LAUNCHER")" || return 1
  recorded_source_hash="$(read_first_line "$MPL_RA_LAUNCHER.source.sha256")"
  if [ -x "$MPL_RA_LAUNCHER" ] &&
     [ "$source_hash" = "$recorded_source_hash" ] &&
     [ ! "$PREPARE_RA_LAUNCHER" -nt "$MPL_RA_LAUNCHER" ]; then
    return 0
  fi
  MPL_H700_RA_LAUNCHER_SOURCE="$MOD_RA_LAUNCHER" \
    MPL_H700_MPL_RA_LAUNCHER="$MPL_RA_LAUNCHER" \
    "$PREPARE_RA_LAUNCHER" || return 1
  recorded_source_hash="$(read_first_line "$MPL_RA_LAUNCHER.source.sha256")"
  [ -x "$MPL_RA_LAUNCHER" ] && [ "$source_hash" = "$recorded_source_hash" ]
}

consume_request() {
  [ -f "$REQUEST_PATH" ] || {
    log_line "[launch] no request: $REQUEST_PATH"
    return 1
  }

  request_version="$(read_value request_version "$REQUEST_PATH")"
  platform_id="$(read_value platform_id "$REQUEST_PATH")"
  game_id="$(read_value game_id "$REQUEST_PATH")"
  rom_path="$(read_value rom_path "$REQUEST_PATH")"
  launcher_id="$(read_value launcher_id "$REQUEST_PATH")"
  core_hint="$(read_value core_hint "$REQUEST_PATH")"

  [ "$request_version" = "1" ] || {
    log_line "[launch] rejected unsupported_request_version value=$request_version"
    return 10
  }

  folder="$(platform_folder "$platform_id")" || {
    log_line "[launch] rejected unsupported_platform platform=$platform_id"
    return 11
  }

  expected="$(expected_launcher_id "$platform_id")" || return 11
  [ "$launcher_id" = "$expected" ] || {
    log_line "[launch] rejected launcher_mismatch platform=$platform_id launcher=$launcher_id expected=$expected"
    return 12
  }

  is_trusted_rom_path "$rom_path" || {
    log_line "[launch] rejected rom_outside_roots platform=$platform_id rom=$rom_path"
    return 13
  }

  [ -f "$rom_path" ] || {
    log_line "[launch] rejected rom_missing platform=$platform_id rom=$rom_path"
    return 14
  }

  if [ "$platform_id" = "nds" ]; then
    launch_nds "$rom_path" "$game_id"
    return "$?"
  fi
  if [ "$platform_id" = "psp" ]; then
    launch_psp "$rom_path" "$game_id"
    return "$?"
  fi
  if [ "$platform_id" = "openbor" ]; then
    launch_openbor "$rom_path" "$game_id"
    return "$?"
  fi
  if [ "$platform_id" = "ports" ]; then
    launch_ports "$rom_path" "$game_id"
    return "$?"
  fi
  if [ "$platform_id" = "java" ]; then
    launch_java "$rom_path" "$game_id"
    return "$?"
  fi
  if [ "$platform_id" = "saturn" ]; then
    launch_saturn "$rom_path" "$game_id"
    return "$?"
  fi

  launch_route=stock
  if [ -x "$MOD_RA_LAUNCHER" ]; then
    launch_route=mod
  elif [ ! -x "$RETROARCH_BIN" ]; then
    log_line "[launch] rejected launcher_missing mod=$MOD_RA_LAUNCHER stock=$RETROARCH_BIN"
    return 15
  fi

  core="$(platform_core "$platform_id" "$core_hint")" || {
    log_line "[launch] rejected core_unconfigured platform=$platform_id core_hint=$core_hint"
    return 16
  }
  config_folder="$(ra_config_folder "$platform_id" "$folder" "$rom_path")"
  arcade_orientation_value="$(arcade_orientation "$platform_id" "$rom_path" || true)"
  control_folder="$(arcade_control_folder "$config_folder" "$arcade_orientation_value")"
  varc_mode="$(forced_varc_mode "$platform_id" "$rom_path" || true)"
  video_rotation="$(forced_video_rotation "$platform_id" "$rom_path" || true)"

  [ -f "$CORE_DIR/$core" ] || {
    log_line "[launch] rejected core_missing platform=$platform_id core=$core path=$CORE_DIR/$core"
    return 17
  }

  launch_rom_path="$rom_path"
  arcade_rom_link=""
  append_config=""
  runner="$RETROARCH_BIN"
  if [ "$launch_route" = "mod" ]; then
    prepare_private_ra_launcher || {
      log_line "[launch] rejected private_launcher_unavailable source=$MOD_RA_LAUNCHER path=$MPL_RA_LAUNCHER prepare=$PREPARE_RA_LAUNCHER"
      return 18
    }
    runner="$MPL_RA_LAUNCHER"
  else
    ensure_retroarch_config || {
      log_line "[launch] rejected retroarch_config_unavailable config=$RETROARCH_CONFIG base=$RETROARCH_BASE_CONFIG"
      return 18
    }

    append_config="$(prepare_retroarch_append_config "$video_rotation" || true)"
    [ -n "$append_config" ] || {
      log_line "[launch] rejected retroarch_append_config_unavailable path=$RETROARCH_APPEND_CONFIG"
      return 18
    }

  fi

  if [ -n "$arcade_orientation_value" ]; then
    arcade_rom_link="$(prepare_arcade_launch_rom "$rom_path" "$control_folder" || true)"
    [ -n "$arcade_rom_link" ] || {
      log_line "[launch] rejected arcade_rom_link_failed platform=$platform_id control_folder=$control_folder rom=$rom_path"
      return 19
    }
    launch_rom_path="$arcade_rom_link"
  fi

  log_line "[launch] launching route=$launch_route platform=$platform_id folder=$folder config_folder=$config_folder control_folder=$control_folder orientation=$arcade_orientation_value varc=$varc_mode rotation=$video_rotation launcher=$launcher_id core=$core game=$game_id rom=$rom_path launch_rom=$launch_rom_path runner=$runner config=$RETROARCH_CONFIG appendconfig=$append_config"
  if prepare_retroarch_game_audio; then
    :
  else
    log_line "[launch] retroarch game audio preparation failed; using frontend hardware volume"
    release_game_audio
  fi
  export LD_LIBRARY_PATH=/usr/lib32:/usr/lib:/mnt/vendor/lib
  unset LD_PRELOAD
  if [ "$launch_route" = "mod" ]; then
    export MPL_FORCE_EMU="$folder"
    export MPL_FORCE_EMU_DIR="$control_folder"
    export MPL_FORCE_RA_CONFIG_EMU="$control_folder"
    export MPL_FORCE_VARC="$varc_mode"
    if [ "$arcade_orientation_value" = "vertical" ]; then
      unset MPL_FORCE_VIDEO_ROTATION
    else
      export MPL_FORCE_VIDEO_ROTATION="$video_rotation"
    fi
    "$MPL_RA_LAUNCHER" "$core" "$launch_rom_path" >>"$LOG_FILE" 2>&1
  else
    unset MPL_FORCE_EMU MPL_FORCE_EMU_DIR MPL_FORCE_RA_CONFIG_EMU \
      MPL_FORCE_VARC MPL_FORCE_VIDEO_ROTATION
    (
      cd "$RETROARCH_WORKDIR" || exit 126
      "$RETROARCH_BIN" -c "$RETROARCH_CONFIG" \
        -L "$CORE_DIR/$core" \
        --appendconfig "$append_config" \
        "$launch_rom_path"
    ) >>"$LOG_FILE" 2>&1
  fi
  rc=$?
  cleanup_arcade_launch_rom "$arcade_rom_link"
  release_game_audio || log_line "[launch] failed to restore frontend hardware volume"
  if [ "$rc" -eq 0 ]; then
    rm -f "$REQUEST_PATH"
    log_line "[launch] completed rc=0 platform=$platform_id game=$game_id"
  else
    log_line "[launch] failed rc=$rc platform=$platform_id game=$game_id"
  fi
  return "$rc"
}

consume_request "$@"

#!/bin/sh
set -eu

SCRIPT="H700/launcher/launch_request.sh"
default_core_function="$(sed -n '/^default_core_for_platform()/,/^}/p' "$SCRIPT")"
[ -n "$default_core_function" ]
eval "$default_core_function"

while IFS=' ' read -r platform expected; do
  [ -n "$platform" ] || continue
  actual="$(default_core_for_platform "$platform")"
  if [ "$actual" != "$expected" ]; then
    printf 'dmenu default mismatch platform=%s expected=%s actual=%s\n' \
      "$platform" "$expected" "$actual" >&2
    exit 1
  fi
done <<'DEFAULTS'
a2600 stella2014_libretro.so
a5200 a5200_libretro.so
a7800 prosystem_libretro.so
a800 atari800_libretro.so
amiga puae_libretro.so
atarist hatari_libretro.so
atomiswave flycast_libretro.so
c64 vice_x64_libretro.so
cps1 fbalpha_libretro.so
cps2 fbalpha_libretro.so
cps3 fbalpha_libretro.so
dos dosbox_pure_libretro.so
dreamcast flycast_libretro.so
easyrpg easyrpg_libretro.so
fbneo fbneo_libretro.so
fc fceumm_libretro.so
fds nestopia_libretro.so
gb gambatte_libretro.so
gba mgba_libretro.so
gbc gambatte_libretro.so
gg genesis_plus_gx_libretro.so
gw gw_libretro.so
hbmame nebularm_legacy_libretro.so
lynx handy_libretro.so
mame mame2003_plus_libretro.so
md genesis_plus_gx_libretro.so
mdcd genesis_plus_gx_libretro.so
msx bluemsx_libretro.so
n64 parallel_n64_libretro.so
naomi flycast_libretro.so
neocd neocd_libretro.so
neogeo fbalpha2012_neogeo_libretro.so
ngp mednafen_ngp_libretro.so
ons onscripter_libretro.so
pce mednafen_pce_fast_libretro.so
pcecd mednafen_pce_fast_libretro.so
pgm2 mame2022xtreme_libretro.so
pico fake08_libretro.so
poke pokemini_libretro.so
ps pcsx_rearmed_libretro.so
scummvm scummvm_libretro.so
sega32x picodrive_libretro.so
sfc snes9x_libretro.so
sms genesis_plus_gx_libretro.so
varcade fbneo_libretro.so
vb mednafen_vb_libretro.so
vic20 vice_xvic_libretro.so
ws mednafen_wswan_libretro.so
DEFAULTS

# RGFrontend

[English](README.md) | [简体中文](README.zh-CN.md)

RGFrontend is a lightweight game library and launcher for H700 stock Linux
handhelds. It scans, organizes, displays, and launches games already present on
the device. It is not an emulator and does not distribute ROMs, BIOS files,
emulator cores, firmware files, or commercial media.

## Features

- H700 stock Linux APPS launcher package.
- Handheld UI layouts for 720x480, 640x480, and 720x720, with platform
  navigation, cover grid, details panel, favorites, recent games, and settings.
- Dual-card scanning for `/mnt/mmc/Roms` and `/mnt/sdcard/Roms`.
- Plain ROM directory scanning.
- Pegasus `metadata.pegasus.txt` support, including arbitrary package directory names and
  per-game launch/core hints.
- EmulationStation `gamelist.xml` support.
- Anbernic-style `Imgs/` media lookup.
- Cover, logo, and video preview support, with safe fallback when media is
  missing.
- Launches games through the device's existing RetroArch and standalone
  emulator chain.
- Single-game core selection, with priority:
  user override > Pegasus metadata hint > system default.
- One-click cache clearing and rescan.
- Long game title scrolling in the details panel.

## System Safety

RGFrontend stays out of system files:

- It uses the target firmware's existing RetroArch, standalone emulators, cores,
  and launch chain.
- It does not install, replace, rename, or delete system cores.
- It does not modify RetroArch main configuration, platform configuration, core
  configuration, remaps, shaders, cheats, vendor launch scripts, or system
  startup files.
- Pegasus `launch` fields are used only for trusted platform and core hints.
- Favorites, recents, cache, UI state, logs, and diagnostics are saved only in
  the app's own data folder.

## Installation

See the H700 installation guide:

- [docs/H700_INSTALL.md](docs/H700_INSTALL.md)
- [中文安装说明](docs/H700_INSTALL.zh-CN.md)

The release zip is designed to be extracted at the TF card root. Its internal
layout starts at `Roms/`:

```text
Roms/APPS/RGFrontend.sh
Roms/APPS/RGFrontend/
Roms/APPS/Imgs/RGFrontend.png
```

## Resolution Compatibility

RGFrontend includes UI layouts for these screen sizes:

| Resolution | Status |
| --- | --- |
| 720x480 | Included and used by the stock H700 release |
| 640x480 | Included as a compact 4:3 layout |
| 720x720 | Included as a square-screen layout |

The stock H700 release uses 720x480. The 640x480 and 720x720 layouts are
included, but should be checked on matching devices before being marked as fully
verified.

## H700 Controls

- `D-pad`: move selection.
- `A`: confirm, launch a game, or save a choice.
- `B`: go back or cancel.
- `L1 / R1`: switch platform or top-level category.
- `L2 / R2`: move one page backward or forward.
- `X`: favorite or unfavorite.
- `Y`: open the current game's core selection.
- `Select`: search the current game list with the built-in on-screen keyboard.
- `Start / Menu`: open settings.
- `Volume - / +`: adjust system volume.
- `Power`: suspend.

## H700 Vendor Platform Mapping and RGFrontend Test Status

The 54 platforms below map directly to platform definitions found in the vendor
`dmenu.bin` of the inspected H700 stock firmware.

A vendor dmenu mapping means that the menu contains platform recognition such
as a name, icon, file extensions, or launcher mapping. It does not guarantee
that every H700 firmware variant contains every optional executable or core,
or that every game has been validated. `YES` means that an RGFrontend launch
path has been exercised; `TBD` means that complete validation is still pending.

| Platform | RGFrontend status |
| --- |---------|
| A2600 | YES     |
| A5200 | YES     |
| A7800 | YES     |
| A800 | TBD     |
| AMIGA | TBD     |
| ATARIST | TBD     |
| ATOMISWAVE | YES     |
| C64 | TBD     |
| CPS1 | YES     |
| CPS2 | YES     |
| CPS3 | YES     |
| DOS | YES     |
| DREAMCAST | YES     |
| EASYRPG | YES     |
| FBNEO | YES     |
| FC | YES     |
| FDS | YES     |
| GB | YES     |
| GBC | YES     |
| GBA | YES     |
| GG | YES     |
| GW | YES     |
| HBMAME | YES     |
| JAVA | YES     |
| LYNX | YES     |
| MAME | YES     |
| MD | YES     |
| MDCD | YES     |
| MSX | YES     |
| N64 | YES     |
| NAOMI | YES     |
| NDS | YES     |
| NEOCD | YES     |
| NEOGEO | YES     |
| NGP | YES     |
| ONS | YES     |
| OPENBOR | YES     |
| PCE | YES     |
| PCECD | YES     |
| PGM2 | TBD     |
| PICO | YES     |
| POKE | YES     |
| PORTS | YES     |
| PS | YES     |
| PSP | YES     |
| SATURN | YES     |
| SCUMMVM | TBD     |
| SEGA32X | YES     |
| SFC | YES     |
| SMS | YES     |
| VARCADE | YES     |
| VB | YES     |
| VIC20 | TBD     |
| WS | YES     |

## License and Attribution

RGFrontend is distributed under GPL-3.0-or-later. It keeps the applicable
license, copyright, attribution, and modification notices for this project,
Pegasus Frontend, and PegasusG by ROC / Blood_roc.

See:

- [LICENSE.md](LICENSE.md)
- [NOTICE.md](NOTICE.md)
- [NOTICE.zh-CN.md](NOTICE.zh-CN.md)
- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

Release packages do not include ROMs, BIOS files, commercial media, emulator
cores, firmware images, or device dumps.

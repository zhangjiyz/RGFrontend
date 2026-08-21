# RGFrontend H700 Installation Guide

[English](H700_INSTALL.md) | [简体中文](H700_INSTALL.zh-CN.md)

This guide is for the H700 stock Linux release zip, for example:

```text
RGFrontend-H700-0.1.0-rc1.zip
```

RGFrontend is a game library and launcher. It does not include ROMs, BIOS
files, emulators, or cores. The package installs only the APPS entry and
RGFrontend runtime files. It does not modify RetroArch configuration, system
cores, vendor launcher scripts, or system startup files.

## Requirements

- An H700 stock Linux handheld.
- A ROM card that the stock firmware can read.
- The release zip, such as `RGFrontend-H700-0.1.0-rc1.zip`.

The zip layout starts at `Roms/`:

```text
Roms/APPS/RGFrontend.sh
Roms/APPS/RGFrontend/
Roms/APPS/Imgs/RGFrontend.png
```

Extract the zip at the TF card root. Do not extract it inside `Roms/APPS`.

## Fresh Install

1. Power off the handheld, remove the ROM card, and insert it into your computer.
2. Copy `RGFrontend-H700-0.1.0-rc1.zip` to the TF card root.

   Correct location example:

   ```text
   TF card root/RGFrontend-H700-0.1.0-rc1.zip
   TF card root/Roms/
   ```

3. Extract the zip at the TF card root.

   After extraction, these paths should exist:

   ```text
   TF card root/Roms/APPS/RGFrontend.sh
   TF card root/Roms/APPS/RGFrontend/
   TF card root/Roms/APPS/Imgs/RGFrontend.png
   ```

4. Safely eject the card, insert it back into the handheld, and boot the device.
5. Open the stock APPS list and launch `RGFrontend`.

## SSH Install

If SSH is enabled on the handheld, you can copy and extract the zip directly on
the device:

```sh
scp RGFrontend-H700-0.1.0-rc1.zip root@10.1.1.233:/mnt/mmc/
ssh root@10.1.1.233
cd /mnt/mmc
unzip -o RGFrontend-H700-0.1.0-rc1.zip
```

After installation, check:

```sh
ls -ld \
  /mnt/mmc/Roms/APPS/RGFrontend \
  /mnt/mmc/Roms/APPS/RGFrontend.sh \
  /mnt/mmc/Roms/APPS/Imgs/RGFrontend.png
```

## Resolution Compatibility

RGFrontend includes UI layouts for 720x480, 640x480, and 720x720. The stock H700
release uses 720x480.

The 640x480 and 720x720 layouts are included, but should be checked on matching
devices before being marked as fully verified.

## License

RGFrontend is distributed under GPL-3.0-or-later and keeps the applicable
copyright, license, attribution, and modification notices for this project,
Pegasus Frontend, and PegasusG by ROC / Blood_roc.

Installed license and notice files are stored under:

```text
Roms/APPS/RGFrontend/licenses/
```

## Upgrade or Reinstall

To upgrade, copy the new zip to the TF card root and extract it again:

```sh
cd /mnt/mmc
unzip -o RGFrontend-H700-0.1.0-rc1.zip
```

For a clean reinstall, delete only these APPS files before extracting the zip:

```text
Roms/APPS/RGFrontend
Roms/APPS/RGFrontend.sh
Roms/APPS/Imgs/RGFrontend.png
```

If an older build used the `RetroFrontend` name, these old entries may also be
removed:

```text
Roms/APPS/RetroFrontend
Roms/APPS/RetroFrontend.sh
Roms/APPS/Imgs/RetroFrontend.png
```

Do not delete ROMs, BIOS files, system cores, or system configuration
directories.

## Uninstall

Delete these files to remove the APPS entry and runtime files:

```text
Roms/APPS/RGFrontend
Roms/APPS/RGFrontend.sh
Roms/APPS/Imgs/RGFrontend.png
```

Favorites, recents, scan cache, and settings are stored in RGFrontend's private
state directory:

```text
/mnt/data/multiplatform-launcher
```

A normal uninstall can leave this directory in place so state can be restored
after reinstalling. Delete it only when you want to fully clear RGFrontend
settings and cache.

## Troubleshooting

### RGFrontend does not appear in APPS

The zip was usually extracted at the wrong level. This layout is wrong:

```text
TF card root/RGFrontend-H700-0.1.0-rc1/Roms/APPS/...
```

The correct layout is:

```text
TF card root/Roms/APPS/RGFrontend.sh
```

### The icon did not update

Check that the menu icon exists:

```text
Roms/APPS/Imgs/RGFrontend.png
```

If the stock APPS list has cached icons, reboot the handheld and check again.

### No games appear after launch

RGFrontend scans the stock ROM roots:

```text
/mnt/mmc/Roms
/mnt/sdcard/Roms
```

Only platforms supported by the device system and this launcher are shown.
Platforms without ROMs, unavailable launchers, or unsupported directories are
hidden from the normal game library.

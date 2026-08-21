# Third-Party Notices

This file records known provenance for third-party projects, formats, runtime
dependencies, and bundled or release-adjacent assets relevant to RGFrontend. It
is not a substitute for the full license text of each upstream project.

## Pegasus Frontend

- Project: https://github.com/mmatyas/pegasus-frontend
- Role: metadata ecosystem, compatibility behavior, and historical frontend
  reference.
- License: GPLv3 or later, with supplementary trademark terms.
- Local license text: `LICENSE.md`.

RGFrontend supports the Pegasus metadata format for compatibility. It is not
an official Pegasus Frontend release and does not use Pegasus marks as its
product title or logo.

## PegasusG by ROC

- Project/reference: `upstream/PegasusG-by-ROC` in local development
  workspaces, when present.
- Role: H700 SDL UI, input, launch, media preview, and packaging reference.
- License: GPLv3 or later with additional attribution terms retained in
  `LICENSE.md`, `NOTICE.md`, and `NOTICE.zh-CN.md`.

The local `upstream/` directory is ignored by this repository and should not
be committed as part of RGFrontend.

## RetroArch and Libretro

- Projects: https://www.retroarch.com/ and https://www.libretro.com/
- Role: target device runtime and emulator/core ecosystem.
- Distribution status: RGFrontend does not bundle RetroArch, Libretro cores,
  BIOS files, or emulator binaries in this repository.

RGFrontend only prepares structured launch requests for device-supported
platforms and relies on the target H700 firmware's existing emulator stack.

## SDL and Media Runtime Dependencies

The desktop and H700 builds may use these external libraries or programs:

- SDL2
- SDL2_ttf
- SDL2_image
- ALSA
- FFmpeg

Distribution status: these are provided by the build host or target firmware
unless a separate release package explicitly says otherwise. Each component
retains its own license and notices.

## H700 Firmware and Vendor Components

Paths such as system RetroArch launchers, emulator binaries, platform cores,
and firmware libraries belong to the target device firmware or its vendors.
RGFrontend does not claim ownership of those components and should not
redistribute them from this repository.

## RGFrontend Binary Release Packages

H700 release packages generated from this repository are intended to contain
only RGFrontend's frontend binary, application scripts, application icons, and
the required license/notice files. They should not contain ROMs, BIOS files,
commercial media, emulator cores, system images, local sysroots, device dumps,
or the local `upstream/` reference checkout.

When a binary package is distributed, provide the Corresponding Source under
GPLv3. If the object code and source are offered from a network location, keep
clear directions next to the binary package that identify where the matching
source can be obtained.

## Bundled Application Icons

The repository currently contains application icon files under:

```text
H700/assets/apps/
```

These icons are intended for RGFrontend application packaging. Before public
redistribution, keep only icons whose authorship and redistribution rights are
confirmed for the RGFrontend release. Do not include old product icons,
upstream project logos, commercial artwork, or unverified third-party marks as
release assets.

## Explicitly Not Distributed

The public source repository should not include:

- ROMs, BIOS files, saves from commercial games, or commercial game media.
- Emulator core binaries, firmware dumps, or device system images.
- Unauthorized music, fonts, themes, shaders, cheat collections, or logos.
- Local sysroots, build outputs, release zips, local logs/state, or the local
  `upstream/` reference checkout.

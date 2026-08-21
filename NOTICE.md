# Copyright and Attribution Notice

## RGFrontend

RGFrontend is a lightweight game library and launcher for H700 stock Linux
handheld devices.

```text
Copyright (c) 2026 zhangjiyz
Website: zhangjiyz.com
```

Redistributions of RGFrontend, in source or binary form, must preserve the
applicable license text, this notice, `NOTICE.zh-CN.md`, and a reasonable
visible credit for:

```text
RGFrontend / zhangjiyz
```

The credit may appear in an About, settings, credits, documentation, or other
comparable user-visible location. It may be restyled, translated, or moved to
fit another device, but must not be removed, hidden, or made effectively
invisible.

RGFrontend is not an emulator. It does not distribute ROMs, BIOS files,
commercial game media, emulator cores, system images, or device-vendor emulator
binaries. It uses the target device's existing emulator stack and launch chain.

## Downstream Status

RGFrontend was developed as an independent product tree while using PegasusG
by ROC and Pegasus Frontend behavior as references for metadata compatibility,
H700 interaction patterns, SDL UI behavior, media preview behavior, and launch
integration. The local `upstream/` reference checkout, when present in a
development workspace, is not part of the public RGFrontend source tree.

This repository is a downstream adaptation. It is not an official release of
Pegasus Frontend, PegasusG by ROC, RetroArch, Libretro, or any device vendor,
and no endorsement by those projects or authors is implied.

## Pegasus Frontend

Pegasus Frontend and its contributors retain their respective copyrights.
The upstream project is available at:

```text
https://github.com/mmatyas/pegasus-frontend
```

RGFrontend may accurately describe support for the Pegasus metadata format,
including `metadata.pegasus.txt`, but does not use Pegasus, Pegasus Frontend,
Pegasus Launcher, or confusingly similar marks as the title or logo of this
modified downstream work.

## PegasusG by ROC

The H700 SDL frontend and integration work used as a reference includes
PegasusG by ROC contributions:

```text
Copyright (c) 2026 Blood_roc
Contact: QQ 825826146
```

Redistributions of RGFrontend, in source or binary form, must preserve the
applicable license text, this notice, `NOTICE.zh-CN.md`, and a reasonable
visible credit for:

```text
PegasusG by ROC / Blood_roc
```

The credit may appear in an About, settings, credits, documentation, or other
comparable user-visible location. It may be restyled, translated, or moved to
fit another device, but must not be removed, hidden, or made effectively
invisible.

## Modification Summary

RGFrontend's current source tree uses independent product branding and a
general `Platform`, `Game`, and `Library` architecture. It separates catalog
scanning, UI, launch requests, H700 device integration, and user state. It
does not execute arbitrary metadata launch commands and does not install,
replace, or modify system emulator cores or RetroArch configuration.

Modified redistributions should clearly state that they are downstream
adaptations of RGFrontend and must not imply endorsement by Pegasus Frontend,
PegasusG by ROC, Blood_roc, RetroArch, Libretro, zhangjiyz, or any device
vendor.

## Binary Distribution

Binary packages should include `LICENSE.md`, `NOTICE.md`, `NOTICE.zh-CN.md`,
and `THIRD_PARTY_NOTICES.md`, and should provide the Corresponding Source as
required by GPLv3.

## Warranty

RGFrontend is provided without warranty, as described in `LICENSE.md`.

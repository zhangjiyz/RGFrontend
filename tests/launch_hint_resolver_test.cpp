#include "catalog/launch_hint_resolver.h"

#include <cassert>
#include <string>

using namespace mpl;

int main() {
  {
    const LaunchHint hint = ResolveLaunchHint(
        "/system/bin/am start --user 0 -n org.ppsspp.ppsspp/.PpssppActivity "
        "-a android.intent.action.VIEW -e path \"{file.path}\"");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.android_package == "org.ppsspp.ppsspp");
    assert(hint.android_activity == ".PpssppActivity");
    assert(hint.platform_hint == "psp");
    assert(hint.launcher_alias == "ppsspp");
    assert(LaunchHintKindName(hint.kind) == std::string("android-activity"));
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "am start -n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture "
        "-e LIBRETRO /data/data/com.retroarch.aarch64/cores/mgba_libretro_android.so "
        "-e ROM \"{file.uri}\"");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.android_package == "com.retroarch.aarch64");
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "gba");
    assert(hint.core_hint == "mgba_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "retroarch -L /home/deck/.config/retroarch/cores/gambatte_libretro.so "
        "\"{file.path}\"");
    assert(hint.kind == LaunchHintKind::LinuxCommand);
    assert(hint.command == "retroarch");
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "gb");
    assert(hint.core_hint == "gambatte_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "am start --user 0 "
        "-n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture "
        "-e LIBRETRO /data/data/com.retroarch.aarch64/cores/fbneo_plus_libretro.so");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "fbneo");
    assert(hint.core_hint == "fbneo_plus_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "am start --user 0 "
        "-n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture "
        "-e LIBRETRO /data/data/com.retroarch.aarch64/cores/mesen_libretro_android.so");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "fc_hd");
    assert(hint.core_hint == "mesen_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "am start --user 0 "
        "-n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture "
        "-e LIBRETRO /data/data/com.retroarch.aarch64/cores/picodrive_libretro_android.so");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "md");
    assert(hint.core_hint == "picodrive_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "am start --user 0 "
        "-n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture "
        "-e LIBRETRO /data/data/com.retroarch.aarch64/cores/mednafen_saturn_libretro_android.so");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "saturn");
    assert(hint.core_hint == "mednafen_saturn_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "am start --user 0 "
        "-n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture "
        "-e LIBRETRO /data/data/com.retroarch.aarch64/cores/mamearcade_libretro_android.so");
    assert(hint.kind == LaunchHintKind::AndroidActivity);
    assert(hint.launcher_alias == "retroarch");
    assert(hint.platform_hint == "mame");
    assert(hint.core_hint == "mame2022xtreme_libretro.so");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "/mnt/vendor/deep/openBOR/OpenBOR.dge \"{file.path}\"");
    assert(hint.kind == LaunchHintKind::LinuxCommand);
    assert(hint.command == "/mnt/vendor/deep/openBOR/OpenBOR.dge");
    assert(hint.launcher_alias == "openbor");
    assert(hint.platform_hint == "openbor");
  }

  {
    const LaunchHint hint = ResolveLaunchHint(
        "/mnt/vendor/deep/emuJava/launch.sh \"{file.path}\"");
    assert(hint.kind == LaunchHintKind::LinuxCommand);
    assert(hint.command == "/mnt/vendor/deep/emuJava/launch.sh");
    assert(hint.launcher_alias == "freej2me");
    assert(hint.platform_hint == "java");
  }

  {
    const LaunchHint hint = ResolveLaunchHint("/usr/bin/custom-launcher --rom \"{file.path}\"");
    assert(hint.kind == LaunchHintKind::Unknown);
    assert(hint.command == "/usr/bin/custom-launcher");
    assert(!hint.raw.empty());
    assert(!hint.diagnostic.empty());
  }

  return 0;
}

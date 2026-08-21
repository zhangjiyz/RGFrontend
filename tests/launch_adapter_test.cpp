#include "devices/h700/capabilities.h"
#include "devices/h700/launch_request_adapter.h"
#include "devices/h700/retroarch_adapter.h"
#include "launch/launch_request.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace mpl;

namespace {

const Platform *FindPlatformById(const std::vector<Platform> &platforms, const std::string &id) {
  const auto found = std::find_if(platforms.begin(), platforms.end(),
                                  [&](const Platform &platform) { return platform.id == id; });
  return found == platforms.end() ? nullptr : &*found;
}

Game MakeGame(const Platform &platform, const fs::path &rom_path) {
  Game game;
  game.id = "fixture-game";
  game.platform_id = platform.id;
  game.title = "Fixture";
  game.primary_target = LaunchTarget{rom_path.u8string(), "default"};
  return game;
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "multiplatform_launcher_launch_adapter_test";
  fs::remove_all(root);

  const fs::path card = root / "mnt" / "sdcard";
  const fs::path gbc_root = card / "Roms" / "GBC";
  const fs::path nds_root = card / "Roms" / "NDS";
  const fs::path psp_root = card / "Roms" / "PSP";
  const fs::path openbor_root = card / "Roms" / "OPENBOR";
  const fs::path ports_root = card / "Roms" / "PORTS";
  const fs::path java_root = card / "Roms" / "JAVA" / "240x320";
  const fs::path saturn_root = card / "Roms" / "SS";
  const fs::path system = root / "system";
  fs::create_directories(gbc_root);
  fs::create_directories(nds_root);
  fs::create_directories(psp_root);
  fs::create_directories(openbor_root);
  fs::create_directories(ports_root);
  fs::create_directories(java_root);
  fs::create_directories(saturn_root);
  fs::create_directories(system);
  const fs::path launcher = system / "RA_launch.sh";
  const fs::path nds_launcher = system / "setNDS.sh";
  const fs::path psp_launcher = system / "PPSSPPSDL";
  const fs::path openbor_setup = system / "openbor.sh";
  const fs::path openbor_launcher = system / "OpenBOR.dge";
  const fs::path ports_shell = system / "bash";
  const fs::path java_launcher = system / "launch_java.sh";
  const fs::path saturn_launcher = system / "setSaturn.sh";
  const fs::path saturn_emulator = system / "yabasanshiro";
  const fs::path saturn_bios = system / "saturn_bios.bin";
  const fs::path rom = gbc_root / "Oracle.gbc";
  const fs::path nds_rom = nds_root / "Ys.nds";
  const fs::path psp_rom = psp_root / "Ridge.iso";
  const fs::path openbor_rom = openbor_root / "Final Fight.pak";
  const fs::path ports_rom = ports_root / "Balatro.sh";
  const fs::path java_rom = java_root / "DoomRPG.jar";
  const fs::path saturn_rom = saturn_root / "Nights.chd";
  std::ofstream(launcher) << "#!/bin/sh\n";
  std::ofstream(nds_launcher) << "#!/bin/sh\n";
  std::ofstream(psp_launcher) << "#!/bin/sh\n";
  std::ofstream(openbor_setup) << "#!/bin/sh\n";
  std::ofstream(openbor_launcher) << "#!/bin/sh\n";
  std::ofstream(ports_shell) << "#!/bin/sh\n";
  std::ofstream(java_launcher) << "#!/bin/sh\n";
  std::ofstream(saturn_launcher) << "#!/bin/sh\n";
  std::ofstream(saturn_emulator) << "#!/bin/sh\n";
  std::ofstream(saturn_bios) << "bios";
  std::ofstream(rom) << "rom";
  std::ofstream(nds_rom) << "nds";
  std::ofstream(psp_rom) << "psp";
  std::ofstream(openbor_rom) << "openbor";
  std::ofstream(ports_rom) << "ports";
  std::ofstream(java_rom) << "java";
  std::ofstream(saturn_rom) << "saturn";

  H700RegistryOptions registry_options;
  registry_options.card_roots = {card.u8string()};
  registry_options.retroarch_launcher = launcher.u8string();
  registry_options.nds_launcher = nds_launcher.u8string();
  registry_options.psp_launcher = psp_launcher.u8string();
  registry_options.openbor_setup_script = openbor_setup.u8string();
  registry_options.openbor_launcher = openbor_launcher.u8string();
  registry_options.ports_shell = ports_shell.u8string();
  registry_options.java_launcher = java_launcher.u8string();
  registry_options.saturn_launcher = saturn_launcher.u8string();
  registry_options.saturn_emulator = saturn_emulator.u8string();
  registry_options.saturn_bios = saturn_bios.u8string();
  registry_options.enable_saturn = true;
  DeviceCapabilities capabilities = LoadH700Capabilities(registry_options);
  assert(capabilities.device_id == "h700-stock-linux");
  assert(capabilities.sdl_video_driver == "mali");
  assert(capabilities.sdl_audio_driver == "alsa");
  assert(capabilities.uses_vendor_frontend_lifecycle);
  assert(capabilities.content_roots.size() == 1);
  assert(capabilities.platforms.size() > 40);

  const Platform *gbc = FindPlatformById(capabilities.platforms, "gbc");
  const Platform *fc_hd = FindPlatformById(capabilities.platforms, "fc_hd");
  const Platform *gba = FindPlatformById(capabilities.platforms, "gba");
  const Platform *nds = FindPlatformById(capabilities.platforms, "nds");
  const Platform *ps = FindPlatformById(capabilities.platforms, "ps");
  const Platform *psp = FindPlatformById(capabilities.platforms, "psp");
  const Platform *openbor = FindPlatformById(capabilities.platforms, "openbor");
  const Platform *ports = FindPlatformById(capabilities.platforms, "ports");
  const Platform *java = FindPlatformById(capabilities.platforms, "java");
  const Platform *saturn = FindPlatformById(capabilities.platforms, "saturn");
  assert(gbc);
  assert(fc_hd);
  assert(gba);
  assert(nds);
  assert(ps);
  assert(psp);
  assert(openbor);
  assert(ports);
  assert(java);
  assert(saturn);
  assert(gbc->launchable);
  assert(nds->launchable);
  assert(psp->launchable);
  assert(openbor->launchable);
  assert(ports->launchable);
  assert(java->launchable);
  assert(saturn->launchable);
  assert(nds->launcher_id == "h700-standalone-nds");
  assert(nds->launcher_kind == LauncherKind::Standalone);
  assert(psp->launcher_id == "h700-standalone-psp");
  assert(psp->launcher_kind == LauncherKind::Standalone);
  assert(openbor->launcher_id == "h700-standalone-openbor");
  assert(openbor->launcher_kind == LauncherKind::Standalone);
  assert(ports->launcher_id == "h700-standalone-ports");
  assert(ports->launcher_kind == LauncherKind::Standalone);
  assert(java->launcher_id == "h700-standalone-java");
  assert(java->launcher_kind == LauncherKind::Standalone);
  assert(saturn->launcher_id == "h700-standalone-saturn");
  assert(saturn->launcher_kind == LauncherKind::Standalone);
  assert(fc_hd->launcher_id == "h700-retroarch-fc_hd");
  assert(gbc->launcher_id == "h700-retroarch-gbc");
  assert(gba->launcher_id == "h700-retroarch-gba");
  assert(ps->launcher_id == "h700-retroarch-ps");
  assert(ps->launcher_kind == LauncherKind::RetroArch);
  Game game = MakeGame(*gbc, rom);
  game.launch_hint.core_hint = "gambatte_libretro.so";
  game.user_core_hint = "sameboy_libretro.so";

  const fs::path request_path = root / "app-data" / "state" / "launch.request";
  H700RetroArchAdapter adapter({request_path.u8string(), launcher.u8string(), {}});
  LaunchCapability capability = adapter.Probe(*gbc);
  assert(capability.available);
  assert(capability.error == LaunchError::None);

  LaunchResult result = adapter.PrepareLaunch(*gbc, game, rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(!fs::exists(fs::u8path(request_path.u8string() + ".tmp")));
  LaunchRequest loaded;
  assert(LoadLaunchRequest(request_path.u8string(), &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "gbc");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == rom.u8string());
  assert(loaded.launcher_id == "h700-retroarch-gbc");
  assert(loaded.core_hint == "sameboy_libretro.so");

  H700LaunchRequestAdapter request_adapter({
      (root / "app-data" / "state" / "h700-launch.request").u8string(),
      launcher.u8string(),
      nds_launcher.u8string(),
      psp_launcher.u8string(),
      openbor_launcher.u8string(),
      openbor_setup.u8string(),
      ports_shell.u8string(),
      java_launcher.u8string(),
      saturn_launcher.u8string(),
      saturn_emulator.u8string(),
      saturn_bios.u8string(),
      {},
  });
  capability = request_adapter.Probe(*gbc);
  assert(capability.available);
  capability = request_adapter.Probe(*nds);
  assert(capability.available);
  assert(capability.kind == LauncherKind::Standalone);
  capability = request_adapter.Probe(*psp);
  assert(capability.available);
  assert(capability.kind == LauncherKind::Standalone);
  capability = request_adapter.Probe(*openbor);
  assert(capability.available);
  assert(capability.kind == LauncherKind::Standalone);
  capability = request_adapter.Probe(*ports);
  assert(capability.available);
  assert(capability.kind == LauncherKind::Standalone);
  capability = request_adapter.Probe(*java);
  assert(capability.available);
  assert(capability.kind == LauncherKind::Standalone);
  capability = request_adapter.Probe(*saturn);
  assert(capability.available);
  assert(capability.kind == LauncherKind::Standalone);
  Game nds_game = MakeGame(*nds, nds_rom);
  result = request_adapter.PrepareLaunch(*nds, nds_game, nds_rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(LoadLaunchRequest((root / "app-data" / "state" / "h700-launch.request").u8string(),
                           &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "nds");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == nds_rom.u8string());
  assert(loaded.launcher_id == "h700-standalone-nds");
  assert(loaded.core_hint.empty());

  Game psp_game = MakeGame(*psp, psp_rom);
  result = request_adapter.PrepareLaunch(*psp, psp_game, psp_rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(LoadLaunchRequest((root / "app-data" / "state" / "h700-launch.request").u8string(),
                           &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "psp");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == psp_rom.u8string());
  assert(loaded.launcher_id == "h700-standalone-psp");
  assert(loaded.core_hint.empty());

  Game openbor_game = MakeGame(*openbor, openbor_rom);
  result = request_adapter.PrepareLaunch(*openbor, openbor_game, openbor_rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(LoadLaunchRequest((root / "app-data" / "state" / "h700-launch.request").u8string(),
                           &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "openbor");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == openbor_rom.u8string());
  assert(loaded.launcher_id == "h700-standalone-openbor");
  assert(loaded.core_hint.empty());

  Game ports_game = MakeGame(*ports, ports_rom);
  result = request_adapter.PrepareLaunch(*ports, ports_game, ports_rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(LoadLaunchRequest((root / "app-data" / "state" / "h700-launch.request").u8string(),
                           &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "ports");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == ports_rom.u8string());
  assert(loaded.launcher_id == "h700-standalone-ports");
  assert(loaded.core_hint.empty());

  Game java_game = MakeGame(*java, java_rom);
  result = request_adapter.PrepareLaunch(*java, java_game, java_rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(LoadLaunchRequest((root / "app-data" / "state" / "h700-launch.request").u8string(),
                           &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "java");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == java_rom.u8string());
  assert(loaded.launcher_id == "h700-standalone-java");
  assert(loaded.core_hint.empty());

  Game saturn_game = MakeGame(*saturn, saturn_rom);
  saturn_game.launch_hint.core_hint = "mednafen_saturn_libretro.so";
  result = request_adapter.PrepareLaunch(*saturn, saturn_game, saturn_rom.u8string());
  assert(result.ok);
  assert(result.error == LaunchError::None);
  assert(LoadLaunchRequest((root / "app-data" / "state" / "h700-launch.request").u8string(),
                           &loaded));
  assert(loaded.request_version == 1);
  assert(loaded.platform_id == "saturn");
  assert(loaded.game_id == "fixture-game");
  assert(loaded.rom_path == saturn_rom.u8string());
  assert(loaded.launcher_id == "h700-standalone-saturn");
  assert(loaded.core_hint == "mednafen_saturn_libretro.so");

  result = adapter.PrepareLaunch(*gbc, game, (gbc_root / "missing.gbc").u8string());
  assert(!result.ok);
  assert(result.error == LaunchError::RomMissing);

  const fs::path outside = root / "outside.gbc";
  std::ofstream(outside) << "rom";
  result = adapter.PrepareLaunch(*gbc, game, outside.u8string());
  assert(!result.ok);
  assert(result.error == LaunchError::RomOutsideTrustedRoots);

  H700RetroArchAdapter missing_launcher_adapter({
      (root / "state" / "missing-launcher.request").u8string(),
      (system / "missing-launcher.sh").u8string(),
      {},
  });
  capability = missing_launcher_adapter.Probe(*gbc);
  assert(!capability.available);
  assert(capability.error == LaunchError::LauncherMissing);

  Platform unavailable = *gbc;
  unavailable.launchable = false;
  result = adapter.PrepareLaunch(unavailable, game, rom.u8string());
  assert(!result.ok);
  assert(result.error == LaunchError::PlatformUnavailable);

  Platform standalone = *gbc;
  standalone.launcher_kind = LauncherKind::Standalone;
  standalone.launcher_id = "fixture-standalone";
  capability = adapter.Probe(standalone);
  assert(!capability.available);
  assert(capability.error == LaunchError::WrongAdapterKind);

  fs::remove_all(root);
  return 0;
}

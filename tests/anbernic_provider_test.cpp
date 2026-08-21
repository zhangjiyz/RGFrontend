#include "catalog/library_builder.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace mpl;

int main() {
  const fs::path root = fs::temp_directory_path() / "multiplatform_launcher_anbernic_test";
  fs::remove_all(root);

  const fs::path fc = root / "Roms" / "FC";
  fs::create_directories(fc / "Imgs");
  std::ofstream(fc / "Contra.nes") << "nes-rom";
  std::ofstream(fc / "Contra.png") << "wrong-location";
  std::ofstream(fc / "Imgs" / "Contra.png") << "preview";
  std::ofstream(fc / "Imgs" / "Contra.nes.jpg") << "alternate-preview";

  Platform platform;
  platform.id = "fc";
  platform.display_name = "FC";
  platform.rom_directories = {fc.u8string()};
  platform.extensions = {".nes", ".zip"};
  platform.launcher_id = "fixture-retroarch-fc";
  platform.launchable = true;

  LibraryBuildReport report = LibraryBuilder().Build({platform});
  assert(report.rom_directory_games == 1);
  assert(report.anbernic_games == 1);
  assert(report.library.games.size() == 1);
  const Game &game = report.library.games[0];
  assert(game.title == "Contra");
  assert(game.source == "anbernic");
  assert(game.media.cover.find("Imgs") != std::string::npos);
  assert(game.media.cover.find("Contra.png") != std::string::npos);
  assert(game.media.logo.empty());
  assert(game.media.video.empty());
  assert(!game.fingerprint.sample_hash.empty());

  fs::remove_all(root);
  return 0;
}

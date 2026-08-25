#include "catalog/library_builder.h"
#include "devices/h700/platform_registry.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace mpl;

namespace {

const Platform *FindPlatformById(const std::vector<Platform> &platforms,
                                 const std::string &id) {
  const auto found = std::find_if(platforms.begin(), platforms.end(),
                                  [&](const Platform &platform) {
                                    return platform.id == id;
                                  });
  return found == platforms.end() ? nullptr : &*found;
}

const Game *FindByTitle(const std::vector<Game> &games, const std::string &title) {
  const auto found = std::find_if(games.begin(), games.end(), [&](const Game &game) {
    return game.title == title;
  });
  return found == games.end() ? nullptr : &*found;
}

void WriteArcadeNames(const fs::path &path, const std::string &naomi_title) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << "\xef\xbb\xbf" "DINO,\"恐龙新世纪\",\"Cadillacs and Dinosaurs\",\n";
  out << "crzytaxi,\"" << naomi_title << "\",\"Crazy Taxi\",\n";
  out << "homebrew,\"自制\"\"游戏\",\"Homebrew Game\",\n";
  out << "neogeo,\"NeoGeoMVS系统\",\"NeoGeo MVS System\",\n";
  out << "pgm,\"PGM\",\"PGM System BIOS\",\n";
  out << "ddtod,\"龙与地下城毁灭之塔\",\"Tower of Doom\",\n";
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "arcade_name_database_test";
  fs::remove_all(root);

  const fs::path card = root / "card";
  const fs::path cps1 = card / "Roms" / "CPS1";
  const fs::path cps2 = card / "Roms" / "CPS2";
  const fs::path naomi = card / "Roms" / "NAOMI";
  const fs::path fbneo = card / "Roms" / "FBNEO";
  const fs::path hbmame = card / "Roms" / "HBMAME";
  const fs::path neogeo = card / "Roms" / "NEOGEO";
  const fs::path gba = card / "Roms" / "GBA";
  const fs::path system = root / "system";
  fs::create_directories(cps1);
  fs::create_directories(cps2);
  fs::create_directories(naomi);
  fs::create_directories(fbneo);
  fs::create_directories(hbmame);
  fs::create_directories(neogeo);
  fs::create_directories(gba);
  fs::create_directories(system);

  std::ofstream(cps1 / "DINO.zip") << "cps1-rom";
  std::ofstream(cps2 / "ddtod.zip") << "cps2-rom";
  std::ofstream(cps2 / "empty-es.zip");
  std::ofstream(naomi / "crzytaxi.zip") << "naomi-rom";
  std::ofstream(fbneo / "bioship.zip") << "legitimate-game";
  std::ofstream(fbneo / "neogeo.zip") << "neogeo-bios";
  std::ofstream(fbneo / "pgm.zip") << "pgm-bios";
  std::ofstream(hbmame / "homebrew.zip") << "hbmame-rom";
  std::ofstream(hbmame / "empty-hack.zip");
  std::ofstream(hbmame / "neogeo.zip") << "neogeo-bios";
  std::ofstream(hbmame / "pgm.zip") << "pgm-bios";
  std::ofstream(neogeo / "kof98.zip") << "neogeo-game";
  std::ofstream(neogeo / "neogeo.zip") << "neogeo-bios";
  std::ofstream(gba / "DINO.gba") << "gba-rom";
  {
    std::ofstream gamelist(cps2 / "gamelist.xml");
    gamelist << "<gameList><game><path>./ddtod.zip</path>"
             << "<name>自定义标题</name></game>"
             << "<game><path>./empty-es.zip</path>"
             << "<name>空 ES 包</name></game></gameList>\n";
  }

  const fs::path database = system / "arcade-plus.csv";
  WriteArcadeNames(database, "疯狂,计程车");
  std::ofstream(system / "RA_launch.sh") << "#!/bin/sh\n";

  H700RegistryOptions options;
  options.card_roots = {card.u8string()};
  options.retroarch_launcher = (system / "RA_launch.sh").u8string();
  options.arcade_name_database = database.u8string();
  const std::vector<Platform> platforms = LoadH700Platforms(options);

  const Platform *cps1_platform = FindPlatformById(platforms, "cps1");
  const Platform *cps2_platform = FindPlatformById(platforms, "cps2");
  const Platform *naomi_platform = FindPlatformById(platforms, "naomi");
  const Platform *fbneo_platform = FindPlatformById(platforms, "fbneo");
  const Platform *hbmame_platform = FindPlatformById(platforms, "hbmame");
  const Platform *neogeo_platform = FindPlatformById(platforms, "neogeo");
  const Platform *gba_platform = FindPlatformById(platforms, "gba");
  assert(cps1_platform && cps2_platform && naomi_platform && fbneo_platform &&
         hbmame_platform && neogeo_platform && gba_platform);
  assert(hbmame_platform->display_name == "H.Brew");
  assert(cps1_platform->arcade_name_database_path == database.u8string());
  assert(naomi_platform->arcade_name_database_path == database.u8string());
  assert(hbmame_platform->arcade_name_database_path == database.u8string());
  assert(gba_platform->arcade_name_database_path.empty());

  int database_platforms = 0;
  for (const Platform &platform : platforms) {
    if (!platform.arcade_name_database_path.empty()) ++database_platforms;
  }
  assert(database_platforms == 11);

  const std::vector<Platform> selected = {
      *cps1_platform, *cps2_platform, *naomi_platform, *fbneo_platform,
      *hbmame_platform, *neogeo_platform, *gba_platform};
  const fs::path cache = root / "cache" / "scan.tsv";
  LibraryBuildReport first = LibraryBuilder().Build(selected, cache.u8string());
  assert(first.library.games.size() == 7);
  assert(FindByTitle(first.library.games, "恐龙新世纪"));
  assert(FindByTitle(first.library.games, "疯狂,计程车"));
  assert(FindByTitle(first.library.games, "自制\"游戏"));
  assert(FindByTitle(first.library.games, "bioship"));
  assert(FindByTitle(first.library.games, "kof98"));
  assert(!FindByTitle(first.library.games, "NeoGeoMVS系统"));
  assert(!FindByTitle(first.library.games, "PGM"));
  assert(!FindByTitle(first.library.games, "empty-hack"));
  assert(!FindByTitle(first.library.games, "空 ES 包"));
  assert(FindByTitle(first.library.games, "自定义标题"));
  const Game *gba_game = FindByTitle(first.library.games, "DINO");
  assert(gba_game && gba_game->platform_id == "gba");

  LibraryBuildReport cached = LibraryBuilder().Build(selected, cache.u8string());
  assert(cached.cached_games == 7);
  assert(FindByTitle(cached.library.games, "疯狂,计程车"));

  const fs::file_time_type old_database_time = fs::last_write_time(database);
  WriteArcadeNames(database, "疯狂出租车");
  fs::last_write_time(database, old_database_time + std::chrono::seconds(2));
  LibraryBuildReport refreshed = LibraryBuilder().Build(selected, cache.u8string());
  assert(FindByTitle(refreshed.library.games, "疯狂出租车"));
  assert(!FindByTitle(refreshed.library.games, "疯狂,计程车"));
  assert(refreshed.cached_games == 1);

  fs::remove_all(root);
  return 0;
}

#include "catalog/library_builder.h"
#include "devices/h700/platform_registry.h"
#include "launch/launch_request.h"
#include "services/state_store.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using namespace mpl;

namespace {

const Game *FindByTitle(const std::vector<Game> &games, const std::string &title) {
  const auto found = std::find_if(games.begin(), games.end(), [&](const Game &game) {
    return game.title == title;
  });
  return found == games.end() ? nullptr : &*found;
}

const Platform *FindPlatformById(const std::vector<Platform> &platforms, const std::string &id) {
  const auto found = std::find_if(platforms.begin(), platforms.end(),
                                  [&](const Platform &platform) { return platform.id == id; });
  return found == platforms.end() ? nullptr : &*found;
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "multiplatform_launcher_p0_test";
  fs::remove_all(root);

  const fs::path card1 = root / "mnt" / "mmc";
  const fs::path card2 = root / "mnt" / "sdcard";
  const fs::path gba1 = card1 / "Roms" / "GBA";
  const fs::path gba2 = card2 / "Roms" / "GBA";
  const fs::path system = root / "system";
  fs::create_directories(gba1 / "media" / "Meta");
  fs::create_directories(gba2);
  fs::create_directories(system);
  std::ofstream(system / "RA_launch.sh") << "#!/bin/sh\n";

  std::ofstream(gba1 / "Meta.gba") << "rom";
  std::ofstream(gba1 / "Loose.gba") << "rom";
  std::ofstream(gba2 / "SecondCard.gba") << "rom";
  std::ofstream(gba1 / "media" / "Meta" / "boxFront.jpg") << "jpg";
  {
    std::ofstream metadata(gba1 / "metadata.pegasus.txt");
    metadata << "collection: GBA\n\n";
    metadata << "launch: am start -n com.retroarch.aarch64/.RetroActivity\n";
    metadata << "  -e LIBRETRO mgba_libretro_android.so\n";
    metadata << "  -e ROM \"{file.uri}\"\n\n";
    metadata << "game: Meta Game\n";
    metadata << "file: Meta.gba\n";
    metadata << "developer: Example Studio\n";
    metadata << "publisher: Example Publisher\n";
    metadata << "genre: RPG\n";
    metadata << "release: 2001\n";
    metadata << "x-id: META001\n";
    metadata << "description: line1\\nline2\n";
    metadata << "sort-by: 001\n";
  }

  H700RegistryOptions registry_options;
  registry_options.card_roots = {card1.u8string(), card2.u8string()};
  registry_options.retroarch_launcher = (system / "RA_launch.sh").u8string();
  std::vector<Platform> platforms = LoadH700Platforms(registry_options);
  assert(platforms.size() > 40);
  const Platform *gba_platform = FindPlatformById(platforms, "gba");
  const Platform *gb_platform = FindPlatformById(platforms, "gb");
  const Platform *ps_platform = FindPlatformById(platforms, "ps");
  assert(gba_platform);
  assert(gb_platform);
  assert(ps_platform);
  assert(gba_platform->display_name == "GBA");
  assert(gba_platform->rom_directories.size() == 4);
  assert(gba_platform->launchable);
  assert(gba_platform->launcher_id == "h700-retroarch-gba");
  assert(gba_platform->launcher_kind == LauncherKind::RetroArch);
  assert(ps_platform->launcher_id == "h700-retroarch-ps");

  LibraryBuildReport report = LibraryBuilder().Build(platforms);
  assert(report.rom_directory_games == 1);
  assert(report.pegasus_games == 1);
  assert(report.merged_duplicates == 0);
  assert(report.library.games.size() == 2);

  const Game *meta = FindByTitle(report.library.games, "Meta Game");
  const Game *second_card = FindByTitle(report.library.games, "SecondCard");
  assert(meta);
  assert(second_card);
  assert(meta->source == "pegasus");
  assert(meta->collection_title == "GBA");
  assert(meta->collection_id == "collection:GBA");
  assert(meta->developer == "Example Studio");
  assert(meta->publisher == "Example Publisher");
  assert(meta->genre == "RPG");
  assert(meta->release == "2001");
  assert(meta->external_id == "META001");
  assert(meta->description == "line1\nline2");
  assert(!meta->media.cover.empty());
  assert(meta->non_executable_launch_hint.find("com.retroarch.aarch64") != std::string::npos);
  assert(meta->launch_hint.kind == LaunchHintKind::AndroidActivity);
  assert(meta->launch_hint.android_package == "com.retroarch.aarch64");
  assert(meta->launch_hint.launcher_alias == "retroarch");
  assert(meta->launch_hint.platform_hint == "gba");
  assert(meta->launch_hint.core_hint == "mgba_libretro.so");
  assert(!FindByTitle(report.library.games, "Loose"));
  assert(second_card->source == "rom-directory");

  std::vector<Game> state_games = report.library.games;
  auto writable_meta = std::find_if(state_games.begin(), state_games.end(), [](const Game &game) {
    return game.title == "Meta Game";
  });
  assert(writable_meta != state_games.end());
  writable_meta->favorite = true;
  writable_meta->recent_order = 12;
  writable_meta->user_core_hint = "gpsp_libretro.so";
  const fs::path state_dir = root / "app-data" / "state";
  GameStateStore game_state((state_dir / "games.tsv").u8string());
  assert(game_state.Save(state_games));

  for (Game &game : state_games) {
    game.favorite = false;
    game.recent_order = 0;
    game.user_core_hint.clear();
  }
  game_state.Load(&state_games);
  writable_meta = std::find_if(state_games.begin(), state_games.end(), [](const Game &game) {
    return game.title == "Meta Game";
  });
  assert(writable_meta != state_games.end());
  assert(writable_meta->favorite);
  assert(writable_meta->recent_order == 12);
  assert(writable_meta->user_core_hint == "gpsp_libretro.so");
  assert(game_state.NextRecentOrder(state_games) == 13);

  UiStateStore ui_state((state_dir / "ui.txt").u8string());
  UiState saved_ui;
  saved_ui.active_platform_id = "gba";
  saved_ui.selected_game_id = meta->id;
  saved_ui.scroll_offset = 4;
  saved_ui.focus = "grid";
  saved_ui.fullscreen_grid = true;
  assert(ui_state.Save(saved_ui));
  UiState loaded_ui;
  assert(ui_state.Load(&loaded_ui));
  assert(loaded_ui.active_platform_id == "gba");
  assert(loaded_ui.selected_game_id == meta->id);
  assert(loaded_ui.scroll_offset == 4);
  assert(loaded_ui.fullscreen_grid);

  LaunchRequest request = BuildLaunchRequest(*gba_platform, *meta, meta->primary_target.path);
  LaunchValidation validation =
      ValidateLaunchRequest(request, *gba_platform, gba_platform->rom_directories);
  assert(validation.ok);
  const fs::path request_path = state_dir / "launch.request";
  assert(SaveLaunchRequest(request, request_path.u8string()));
  LaunchRequest loaded_request;
  assert(LoadLaunchRequest(request_path.u8string(), &loaded_request));
  assert(loaded_request.request_version == 1);
  assert(loaded_request.platform_id == "gba");
  assert(loaded_request.game_id == meta->id);
  assert(loaded_request.rom_path == meta->primary_target.path);
  assert(loaded_request.launcher_id == "h700-retroarch-gba");

  std::ofstream(root / "outside.gba") << "rom";
  LaunchRequest outside = BuildLaunchRequest(*gba_platform, *meta, (root / "outside.gba").u8string());
  validation = ValidateLaunchRequest(outside, *gba_platform, gba_platform->rom_directories);
  assert(!validation.ok);
  assert(validation.message == "ROM outside trusted roots");

  H700RegistryOptions unavailable_options = registry_options;
  unavailable_options.retroarch_launcher = (system / "missing.sh").u8string();
  std::vector<Platform> unavailable = LoadH700Platforms(unavailable_options);
  const Platform *unavailable_gba = FindPlatformById(unavailable, "gba");
  assert(unavailable_gba);
  assert(!unavailable_gba->launchable);
  assert(!unavailable_gba->diagnostics.empty());

  fs::remove_all(root);
  return 0;
}

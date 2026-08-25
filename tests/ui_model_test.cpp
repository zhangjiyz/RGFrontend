#include "ui/ui_model.h"

#include <cassert>
#include <string>

using namespace mpl;

namespace {

Platform PlatformDef(std::string id, std::string name, int sort_order, bool launchable = true) {
  Platform platform;
  platform.id = std::move(id);
  platform.display_name = std::move(name);
  platform.sort_order = sort_order;
  platform.launchable = launchable;
  platform.launcher_id = "fixture-" + platform.id;
  return platform;
}

Game GameDef(std::string id, std::string platform_id, std::string title) {
  Game game;
  game.id = std::move(id);
  game.platform_id = std::move(platform_id);
  game.title = std::move(title);
  game.sort_key = game.title;
  game.primary_target.path = "/roms/" + game.platform_id + "/" + game.title;
  return game;
}

}  // namespace

int main() {
  Library library;
  library.platforms = {
      PlatformDef("gba", "GBA", 30),
      PlatformDef("gb", "GB", 10),
      PlatformDef("nds", "NDS", 40, false),
      PlatformDef("gbc", "GBC", 20),
  };
  library.games = {
      GameDef("gba-metroid", "gba", "Metroid"),
      GameDef("gb-tetris", "gb", "Tetris"),
      GameDef("gbc-zelda", "gbc", "Zelda"),
  };
  library.games[0].collection_id = "collection:动作合集";
  library.games[0].collection_title = "动作合集";
  library.games[2].collection_id = "collection:动作合集";
  library.games[2].collection_title = "动作合集";
  library.games[1].favorite = true;
  library.games[2].recent_order = 3;
  library.games[0].recent_order = 1;

  UiState restored;
  restored.active_platform_id = "collection:动作合集";
  restored.selected_game_id = "gbc-zelda";
  UiSession session = CreateUiSession(library, restored);

  assert(session.navigation.size() == 5);
  assert(session.navigation[0].id == "recent");
  assert(session.navigation[1].id == "favorites");
  assert(session.navigation[2].id == "all");
  assert(session.navigation[3].kind == UiNavKind::Collection);
  assert(session.navigation[3].id == "collection:动作合集");
  assert(session.navigation[3].title == "动作合集");
  assert(session.navigation[3].game_count == 2);
  assert(session.navigation[4].id == "gb");
  assert(session.navigation[4].game_count == 1);
  assert(SelectedGame(session));
  assert(SelectedGame(session)->id == "gbc-zelda");

  UiSession stale_platform_session =
      CreateUiSession(library, UiState{"gbc", "gbc-zelda", 0, "grid"});
  assert(stale_platform_session.navigation[stale_platform_session.active_nav_index].id ==
         "collection:动作合集");
  assert(SelectedGame(stale_platform_session));
  assert(SelectedGame(stale_platform_session)->id == "gbc-zelda");

  const UiLayout layout = ResolveUiLayout(720, 480);

  UiSession search_session = CreateUiSession(library, UiState{"all", "gb-tetris", 0, "grid"});
  ApplyUiAction(&search_session, UiAction::Search, layout);
  assert(search_session.search_active);
  assert(AppendSearchText(&search_session, "MET"));
  assert(search_session.search_query == "MET");
  assert(search_session.visible_game_indices.size() == 1);
  assert(SelectedGame(search_session));
  assert(SelectedGame(search_session)->id == "gba-metroid");
  assert(BackspaceSearchText(&search_session));
  assert(search_session.search_query == "ME");
  ApplyUiAction(&search_session, UiAction::Back, layout);
  assert(!search_session.search_active);
  assert(search_session.search_query == "ME");
  ApplyUiAction(&search_session, UiAction::Back, layout);
  assert(search_session.search_query.empty());
  assert(search_session.visible_game_indices.size() == 3);
  ApplyUiAction(&search_session, UiAction::Search, layout);
  assert(AppendSearchText(&search_session, "动作"));
  assert(BackspaceSearchText(&search_session));
  assert(search_session.search_query == "动");
  assert(BackspaceSearchText(&search_session));
  assert(search_session.search_query.empty());

  UiSession keyboard_session =
      CreateUiSession(library, UiState{"all", "gb-tetris", 0, "grid"});
  ApplyUiAction(&keyboard_session, UiAction::Search, layout);
  assert(keyboard_session.search_keyboard_index == 0);
  UiActionResult keyboard_input =
      ApplyUiAction(&keyboard_session, UiAction::Confirm, layout);
  assert(keyboard_input.intent == UiIntent::None);
  assert(keyboard_session.search_query == "1");
  ApplyUiAction(&keyboard_session, UiAction::Right, layout);
  assert(keyboard_session.search_keyboard_index == 1);
  ApplyUiAction(&keyboard_session, UiAction::Down, layout);
  assert(keyboard_session.search_keyboard_index == 11);
  ApplyUiAction(&keyboard_session, UiAction::Left, layout);
  assert(keyboard_session.search_keyboard_index == 10);
  ApplyUiAction(&keyboard_session, UiAction::Up, layout);
  assert(keyboard_session.search_keyboard_index == 0);
  ApplyUiAction(&keyboard_session, UiAction::ToggleFavorite, layout);
  assert(keyboard_session.search_query.empty());
  assert(!keyboard_session.library.games[0].favorite);
  assert(AppendSearchText(&keyboard_session, "动作"));
  ApplyUiAction(&keyboard_session, UiAction::ToggleFavorite, layout);
  assert(keyboard_session.search_query == "动");
  keyboard_session.search_keyboard_index = 29;
  ApplyUiAction(&keyboard_session, UiAction::Confirm, layout);
  assert(keyboard_session.search_query.empty());
  keyboard_session.search_keyboard_index = 39;
  ApplyUiAction(&keyboard_session, UiAction::Down, layout);
  assert(keyboard_session.search_keyboard_index == kSearchKeyboardOkIndex);
  ApplyUiAction(&keyboard_session, UiAction::Up, layout);
  assert(keyboard_session.search_keyboard_index == 35);
  ApplyUiAction(&keyboard_session, UiAction::OpenCoreSelect, layout);
  assert(!keyboard_session.search_active);
  assert(keyboard_session.view == UiView::Library);
  assert(SearchKeyboardKeyLabel(37) == "空格");
  assert(SearchKeyboardKeyLabel(kSearchKeyboardOkIndex) == "确定");

  UiActionResult launch = ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(launch.intent == UiIntent::LaunchGame);
  assert(launch.game_id == "gbc-zelda");

  ApplyUiAction(&session, UiAction::ToggleFavorite, layout);
  assert(SelectedGame(session)->favorite);
  assert(session.navigation[1].game_count == 2);
  assert(session.osd_text == "已加入收藏");
  assert(session.osd_frames_remaining == 100);

  UiState exported = ExportUiState(session);
  assert(exported.active_platform_id == "collection:动作合集");
  assert(exported.selected_game_id == "gbc-zelda");
  assert(exported.focus == "grid");

  UiState saved_from_settings;
  saved_from_settings.focus = "settings";
  saved_from_settings.active_platform_id = "all";
  UiSession restored_from_settings = CreateUiSession(library, saved_from_settings);
  assert(restored_from_settings.view == UiView::Library);

  ApplyUiAction(&restored_from_settings, UiAction::MenuPress, layout);
  ApplyUiAction(&restored_from_settings, UiAction::MenuRelease, layout);
  assert(restored_from_settings.view == UiView::Settings);
  ApplyUiAction(&restored_from_settings, UiAction::MenuPress, layout);
  ApplyUiAction(&restored_from_settings, UiAction::MenuRelease, layout);
  assert(restored_from_settings.view == UiView::Library);

  ApplyUiAction(&session, UiAction::Menu, layout);
  assert(session.view == UiView::Settings);
  assert(ExportUiState(session).focus == "grid");
  UiActionResult return_system = ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(return_system.intent == UiIntent::ReturnToSystem);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(session.view == UiView::Hotkeys);
  ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(session.view == UiView::Settings);
  ApplyUiAction(&session, UiAction::Down, layout);
  session.system_status.autostart_enabled = 0;
  UiActionResult autostart = ApplyUiAction(&session, UiAction::Right, layout);
  assert(autostart.intent == UiIntent::SetAutostart);
  assert(autostart.delta == 1);
  session.system_status.autostart_enabled = 1;
  autostart = ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(autostart.intent == UiIntent::SetAutostart);
  assert(autostart.delta == 0);
  ApplyUiAction(&session, UiAction::Down, layout);
  UiActionResult brightness = ApplyUiAction(&session, UiAction::Right, layout);
  assert(brightness.intent == UiIntent::AdjustBrightness);
  assert(brightness.delta == 1);
  ApplyUiAction(&session, UiAction::Down, layout);
  UiActionResult volume = ApplyUiAction(&session, UiAction::Left, layout);
  assert(volume.intent == UiIntent::AdjustVolume);
  assert(volume.delta == -1);
  UiActionResult volume_key_up = ApplyUiAction(&session, UiAction::AdjustVolumeUp, layout);
  assert(volume_key_up.intent == UiIntent::AdjustVolume);
  assert(volume_key_up.delta == 1);
  UiActionResult volume_key_down = ApplyUiAction(&session, UiAction::AdjustVolumeDown, layout);
  assert(volume_key_down.intent == UiIntent::AdjustVolume);
  assert(volume_key_down.delta == -1);
  UiActionResult suspend = ApplyUiAction(&session, UiAction::Power, layout);
  assert(suspend.intent == UiIntent::SuspendSystem);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Right, layout);
  assert(!session.preferences.preview_video_enabled);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Right, layout);
  assert(session.preferences.grid_size == UiGridSize::Small);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Right, layout);
  assert(session.preferences.fullscreen_grid);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Right, layout);
  assert(session.view == UiView::ThemeSelect);
  assert(session.theme_selected_index == 0);
  ApplyUiAction(&session, UiAction::Right, layout);
  assert(session.preferences.theme_color == 1);
  ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(session.view == UiView::Settings);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Right, layout);
  assert(!session.preferences.show_cover_titles);
  assert(ExportUiState(session).fullscreen_grid);
  assert(!ExportUiState(session).preview_video_enabled);
  for (int i = 0; i < 3; ++i) {
    ApplyUiAction(&session, UiAction::Down, layout);
  }
  UiActionResult rescan = ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(rescan.intent == UiIntent::ClearCacheAndRescan);
  for (int i = 0; i < 3; ++i) {
    ApplyUiAction(&session, UiAction::Down, layout);
  }
  ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(session.view == UiView::About);
  ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(session.view == UiView::Settings);
  ApplyUiAction(&session, UiAction::Back, layout);
  assert(session.view == UiView::Library);
  ApplyUiAction(&session, UiAction::DescriptionDown, layout);
  assert(session.description_scroll_line == 1);
  assert(ExportUiState(session).description_scroll_line == 0);
  ApplyUiAction(&session, UiAction::Left, layout);
  assert(SelectedGame(session)->id == "gba-metroid");
  assert(session.description_scroll_line == 0);
  ApplyUiAction(&session, UiAction::ToggleTitles, layout);
  assert(session.preferences.show_cover_titles);
  ApplyUiAction(&session, UiAction::ToggleFullscreenGrid, layout);
  assert(!session.preferences.fullscreen_grid);
  ApplyUiAction(&session, UiAction::OpenCoreSelect, layout);
  assert(session.view == UiView::CoreSelect);
  assert(CoreOptionsForGame(session).front().label == "自动: mGBA");
  ApplyUiAction(&session, UiAction::Back, layout);
  assert(session.view == UiView::Library);
  assert(SelectedGame(session)->user_core_hint.empty());
  ApplyUiAction(&session, UiAction::OpenCoreSelect, layout);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Down, layout);
  ApplyUiAction(&session, UiAction::Confirm, layout);
  assert(session.view == UiView::Library);
  assert(SelectedGame(session)->user_core_hint == "gpsp_libretro.so");
  assert(session.osd_text == "核心 gpSP");

  for (const std::string &platform_id : {"cps1", "cps2", "cps3"}) {
    Library cps_library;
    cps_library.platforms = {PlatformDef(platform_id, platform_id, 10)};
    cps_library.games = {GameDef(platform_id + "-game", platform_id, "Game")};
    UiSession cps_session = CreateUiSession(
        cps_library, UiState{"all", platform_id + "-game", 0, "grid"});
    const std::vector<UiCoreOption> cps_cores = CoreOptionsForGame(cps_session);
    assert(!cps_cores.empty());
    assert(cps_cores.front().label == "自动: FB Alpha");
  }
  assert(CoreDisplayName("fbalpha_libretro.so") == "FB Alpha");

  struct DmenuDefaultCase {
    const char *platform_id;
    const char *display_name;
  };
  const DmenuDefaultCase dmenu_defaults[] = {
      {"fds", "自动: Nestopia"},
      {"fbneo", "自动: FinalBurn Neo"},
      {"hbmame", "自动: Nebula ARM Legacy"},
      {"mame", "自动: MAME 2003-Plus"},
      {"n64", "自动: ParaLLEl N64"},
      {"neogeo", "自动: FB Alpha 2012 Neo Geo"},
      {"sfc", "自动: Snes9x"},
  };
  for (const DmenuDefaultCase &entry : dmenu_defaults) {
    Library core_library;
    core_library.platforms = {PlatformDef(entry.platform_id, entry.platform_id, 10)};
    core_library.games = {GameDef("core-game", entry.platform_id, "Game")};
    UiSession core_session =
        CreateUiSession(core_library, UiState{"all", "core-game", 0, "grid"});
    assert(CurrentCoreDisplayName(core_session) == entry.display_name);
  }

  ApplyUiAction(&session, UiAction::GridLarger, layout);
  assert(session.preferences.grid_size == UiGridSize::Medium);
  ApplyUiAction(&session, UiAction::QuickTheme, layout);
  assert(session.preferences.theme_color == 2);
  UiState last_theme_state;
  last_theme_state.theme_color = kUiThemeColorCount - 1;
  UiSession last_theme = CreateUiSession(library, last_theme_state);
  assert(last_theme.preferences.theme_color == kUiThemeColorCount - 1);
  ApplyUiAction(&last_theme, UiAction::QuickTheme, layout);
  assert(last_theme.preferences.theme_color == 0);
  UiState legacy_music_state;
  legacy_music_state.bgm_mode = 0;
  assert(CreateUiSession(library, legacy_music_state).preferences.bgm_mode ==
         UiBgmMode::GameAudio);
  session.notice = UiNotice{true, "启动失败", "ROM文件不存在或无法读取。"};
  ApplyUiAction(&session, UiAction::Back, layout);
  assert(!session.notice.visible);

  UiSession favorites = CreateUiSession(session.library, UiState{"favorites", "gb-tetris", 0,
                                                                 "grid"});
  assert(favorites.visible_game_indices.size() == 2);
  assert(SelectedGame(favorites)->id == "gb-tetris");

  UiSession include_empty = CreateUiSession(library, UiState{}, true);
  bool has_nds = false;
  for (const UiNavItem &item : include_empty.navigation) {
    if (item.id == "nds") has_nds = true;
  }
  assert(!has_nds);

  Library multi_library;
  multi_library.platforms = {PlatformDef("gba", "GBA", 10)};
  Game multi = GameDef("gba-multi", "gba", "Multi");
  multi.primary_target = LaunchTarget{"/roms/gba/multi-a.gba", "A"};
  multi.alternate_targets = {
      LaunchTarget{"/roms/gba/multi-b.gba", "B"},
      LaunchTarget{"/roms/gba/multi-c.gba", "C"},
  };
  multi_library.games = {multi};
  UiSession multi_session = CreateUiSession(multi_library, UiState{"all", "gba-multi", 0,
                                                                   "grid"});
  UiActionResult open_targets = ApplyUiAction(&multi_session, UiAction::Confirm, layout);
  assert(open_targets.intent == UiIntent::None);
  assert(multi_session.view == UiView::TargetSelect);
  ApplyUiAction(&multi_session, UiAction::Down, layout);
  UiActionResult launch_target = ApplyUiAction(&multi_session, UiAction::Confirm, layout);
  assert(launch_target.intent == UiIntent::LaunchGame);
  assert(launch_target.game_id == "gba-multi");
  assert(launch_target.target_index == 1);
  assert(launch_target.target_path == "/roms/gba/multi-b.gba");

  Library page_library;
  page_library.platforms = {PlatformDef("gba", "GBA", 10)};
  for (int index = 0; index < 30; ++index) {
    const std::string suffix = index < 10 ? "0" + std::to_string(index)
                                          : std::to_string(index);
    page_library.games.push_back(GameDef("gba-" + suffix, "gba", "Game " + suffix));
  }
  UiSession page_session = CreateUiSession(page_library, UiState{"all", "gba-00", 0, "grid"});
  ApplyUiAction(&page_session, UiAction::PageNext, layout);
  assert(page_session.selected_visible_index == 12);
  assert(SelectedGame(page_session)->id == "gba-12");
  ApplyUiAction(&page_session, UiAction::PagePrevious, layout);
  assert(page_session.selected_visible_index == 0);
  assert(SelectedGame(page_session)->id == "gba-00");

  UiSession scroll_session =
      CreateUiSession(page_library, UiState{"all", "gba-00", 0, "grid"});
  ApplyUiAction(&scroll_session, UiAction::Down, layout);
  ApplyUiAction(&scroll_session, UiAction::Down, layout);
  ApplyUiAction(&scroll_session, UiAction::Down, layout);
  assert(scroll_session.selected_visible_index == 12);
  assert(scroll_session.scroll_offset == 0);
  ApplyUiAction(&scroll_session, UiAction::Down, layout);
  assert(scroll_session.selected_visible_index == 16);
  assert(scroll_session.scroll_offset == 4);
  ApplyUiAction(&scroll_session, UiAction::Up, layout);
  assert(scroll_session.selected_visible_index == 12);
  assert(scroll_session.scroll_offset == 4);
  ApplyUiAction(&scroll_session, UiAction::Up, layout);
  assert(scroll_session.selected_visible_index == 8);
  assert(scroll_session.scroll_offset == 4);
  ApplyUiAction(&scroll_session, UiAction::Up, layout);
  assert(scroll_session.selected_visible_index == 4);
  assert(scroll_session.scroll_offset == 4);
  ApplyUiAction(&scroll_session, UiAction::Up, layout);
  assert(scroll_session.selected_visible_index == 0);
  assert(scroll_session.scroll_offset == 0);

  for (const auto &resolution : {std::pair{720, 480}, std::pair{640, 480},
                                 std::pair{720, 720}}) {
    Library pegasus_library = page_library;
    for (Game &game : pegasus_library.games) game.source = "pegasus";
    UiState pegasus_state{"all", "gba-00", 0, "grid"};
    pegasus_state.fullscreen_grid = true;
    UiSession pegasus_session = CreateUiSession(pegasus_library, pegasus_state);
    const UiLayout pegasus_layout = ResolveUiLayout(resolution.first, resolution.second);
    for (int step = 0; step < 6; ++step) {
      ApplyUiAction(&pegasus_session, UiAction::Down, pegasus_layout);
    }
    const int columns = pegasus_session.preferences.fullscreen_grid ? 5 : 4;
    const int bottom_row = pegasus_session.selected_visible_index / columns;
    const int scroll_row = pegasus_session.scroll_offset / columns;
    const int expected_visible_rows = resolution.second >= 700 ? 3 : 2;
    assert(bottom_row - scroll_row == expected_visible_rows);
    ApplyUiAction(&pegasus_session, UiAction::Up, pegasus_layout);
    assert(pegasus_session.selected_visible_index / columns == bottom_row - 1);
    assert(pegasus_session.scroll_offset / columns == scroll_row);
    assert(pegasus_session.selected_visible_index / columns - scroll_row ==
           expected_visible_rows - 1);
  }

  const UiLayout tiny = ResolveUiLayout(320, 240);
  assert(tiny.grid_columns >= 2);
  assert(tiny.card_width > 0);
  assert(tiny.grid_x + tiny.grid_width <= tiny.viewport_width);
  return 0;
}

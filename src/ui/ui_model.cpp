#include "ui/ui_model.h"

#include <algorithm>
#include <map>

namespace mpl {

namespace {

constexpr int kSettingsCount = 17;
constexpr int kVisibleSettingsRows = 6;
constexpr int kVisibleThemeRows = 7;
constexpr int kVisibleTargetRows = 5;
constexpr int kVisibleCoreRows = 5;
constexpr int kFontSizeLevelCount = 3;

int ClampIndex(int value, int minimum, int maximum) {
  return std::max(minimum, std::min(value, maximum));
}

int ClampCycle(int value, int count) {
  if (count <= 0) return 0;
  return (value % count + count) % count;
}

UiGridSize GridSizeFromInt(int value) {
  switch (value) {
    case 0:
      return UiGridSize::Large;
    case 2:
      return UiGridSize::Small;
    default:
      return UiGridSize::Medium;
  }
}

UiBgmMode BgmModeFromInt(int value) {
  switch (value) {
    case 0:
      return UiBgmMode::Music;
    case 2:
      return UiBgmMode::Muted;
    default:
      return UiBgmMode::GameAudio;
  }
}

int GridSizeToInt(UiGridSize value) {
  switch (value) {
    case UiGridSize::Large:
      return 0;
    case UiGridSize::Small:
      return 2;
    case UiGridSize::Medium:
      return 1;
  }
  return 1;
}

int BgmModeToInt(UiBgmMode value) {
  switch (value) {
    case UiBgmMode::Music:
      return 0;
    case UiBgmMode::Muted:
      return 2;
    case UiBgmMode::GameAudio:
      return 1;
  }
  return 1;
}

UiPreferences PreferencesFromState(const UiState &state) {
  UiPreferences preferences;
  preferences.grid_size = GridSizeFromInt(state.grid_size);
  preferences.bgm_mode = BgmModeFromInt(state.bgm_mode);
  preferences.theme_color = ClampCycle(state.theme_color, kUiThemeColorCount);
  preferences.startup_logo_style = ClampCycle(state.startup_logo_style,
                                              kUiStartupLogoStyleCount);
  preferences.cover_title_size_level = ClampCycle(state.cover_title_size_level,
                                                  kFontSizeLevelCount);
  preferences.description_size_level = ClampCycle(state.description_size_level,
                                                  kFontSizeLevelCount);
  preferences.show_cover_titles = state.show_cover_titles;
  preferences.fullscreen_grid = state.fullscreen_grid;
  preferences.preview_video_enabled = state.preview_video_enabled;
  preferences.preview_video_loop = state.preview_video_loop;
  return preferences;
}

int GridColumnsForPreferences(const UiPreferences &preferences) {
  int columns = 4;
  switch (preferences.grid_size) {
    case UiGridSize::Large:
      columns = 3;
      break;
    case UiGridSize::Small:
      columns = 5;
      break;
    case UiGridSize::Medium:
      columns = 4;
      break;
  }
  return columns + (preferences.fullscreen_grid ? 1 : 0);
}

int GridRowsForPreferences(const UiPreferences &preferences, const UiLayout &layout) {
  const int columns = GridColumnsForPreferences(preferences);
  const bool compact_four_three = layout.mode == LayoutMode::Compact &&
                                  layout.viewport_width <= 660 &&
                                  layout.viewport_height >= 470;
  const int grid_width = preferences.fullscreen_grid
                             ? (compact_four_three ? 640 : 720)
                             : (compact_four_three ? 400 : 480);
  int grid_height = 435;
  if (layout.mode == LayoutMode::Square && layout.viewport_height >= 700) {
    grid_height = 675;
  }
  const int inset = preferences.fullscreen_grid ? 18 : 16;
  const int card_size = std::max(1, (grid_width - inset * 2) / columns);
  return std::max(1, (grid_height - inset * 2) / card_size);
}

int GridPageSizeForPreferences(const UiPreferences &preferences, const UiLayout &layout) {
  return std::max(1, GridColumnsForPreferences(preferences) *
                         GridRowsForPreferences(preferences, layout));
}

int TargetCount(const Game *game) {
  if (!game) return 0;
  return 1 + static_cast<int>(game->alternate_targets.size());
}

const LaunchTarget *TargetAt(const Game *game, int index) {
  if (!game) return nullptr;
  if (index == 0) return &game->primary_target;
  const int alternate_index = index - 1;
  if (alternate_index < 0 ||
      alternate_index >= static_cast<int>(game->alternate_targets.size())) {
    return nullptr;
  }
  return &game->alternate_targets[alternate_index];
}

const Platform *SelectedPlatform(const UiSession &session) {
  const Game *game = SelectedGame(session);
  if (!game) return nullptr;
  for (const Platform &platform : session.library.platforms) {
    if (platform.id == game->platform_id) return &platform;
  }
  return nullptr;
}

void AddCore(std::vector<UiCoreOption> *options, const char *core, const char *label) {
  if (!options) return;
  options->push_back(UiCoreOption{core, label});
}

std::string DefaultCoreHintForPlatform(const Platform &platform) {
  const std::string &id = platform.id;
  if (id == "gba") return "mgba_libretro.so";
  if (id == "gb" || id == "gbc") return "gambatte_libretro.so";
  if (id == "fc" || id == "fds") return "fceumm_libretro.so";
  if (id == "sfc") return "snes9x2005_plus_libretro.so";
  if (id == "md" || id == "mdcd" || id == "sms" || id == "gg") {
    return "genesis_plus_gx_libretro.so";
  }
  if (id == "sega32x") return "picodrive_libretro.so";
  if (id == "ps") return "pcsx_rearmed_libretro.so";
  if (id == "n64") return "mupen64plus_next_libretro.so";
  if (id == "fbneo" || id == "cps1" || id == "cps2" ||
      id == "cps3" || id == "neogeo") {
    return "fbalpha2012_libretro.so";
  }
  if (id == "mame") return "mame2022xtreme_libretro.so";
  if (id == "varcade") return "fbneo_libretro.so";
  return "";
}

std::string AutomaticCoreHintForGame(const UiSession &session) {
  const Game *game = SelectedGame(session);
  if (!game) return "";
  if (!game->launch_hint.core_hint.empty()) return game->launch_hint.core_hint;
  const Platform *platform = SelectedPlatform(session);
  return platform ? DefaultCoreHintForPlatform(*platform) : "";
}

std::vector<UiCoreOption> PlatformCoreOptions(const Platform &platform) {
  std::vector<UiCoreOption> options;
  if (platform.launcher_kind != LauncherKind::RetroArch) return options;
  AddCore(&options, "", "自动");
  const std::string &id = platform.id;
  if (id == "gba") {
    AddCore(&options, "mgba_libretro.so", "mGBA");
    AddCore(&options, "gpsp_libretro.so", "gpSP");
    AddCore(&options, "vbam_libretro.so", "VBA-M");
    AddCore(&options, "vba_next_libretro.so", "VBA Next");
  } else if (id == "gb" || id == "gbc") {
    AddCore(&options, "gambatte_libretro.so", "Gambatte");
    AddCore(&options, "tgbdual_libretro.so", "TGB Dual");
    AddCore(&options, "gearboy_libretro.so", "Gearboy");
    AddCore(&options, "sameboy_libretro.so", "SameBoy");
  } else if (id == "fc" || id == "fds" || id == "fc_hd") {
    AddCore(&options, "fceumm_libretro.so", "FCEUmm");
    AddCore(&options, "nestopia_libretro.so", "Nestopia");
    AddCore(&options, "mesen_libretro.so", "Mesen");
    AddCore(&options, "quicknes_libretro.so", "QuickNES");
  } else if (id == "sfc") {
    AddCore(&options, "snes9x2005_plus_libretro.so", "Snes9x 2005 Plus");
    AddCore(&options, "snes9x_libretro.so", "Snes9x");
    AddCore(&options, "snes9x2002_libretro.so", "Snes9x 2002");
    AddCore(&options, "snes9x2005_libretro.so", "Snes9x 2005");
    AddCore(&options, "snes9x2010_libretro.so", "Snes9x 2010");
  } else if (id == "md" || id == "mdcd" || id == "sms" || id == "gg" ||
             id == "sega32x") {
    AddCore(&options, "genesis_plus_gx_libretro.so", "Genesis Plus GX");
    AddCore(&options, "picodrive_libretro.so", "PicoDrive");
  } else if (id == "ps") {
    AddCore(&options, "pcsx_rearmed_libretro.so", "PCSX-ReARMed");
    AddCore(&options, "pcsx_rearmed_peops_libretro.so", "PCSX-ReARMed PEOPS");
    AddCore(&options, "pcsx_rearmed_rumble_libretro.so", "PCSX-ReARMed Rumble");
    AddCore(&options, "swanstation_libretro.so", "SwanStation");
  } else if (id == "n64") {
    AddCore(&options, "mupen64plus_next_libretro.so", "Mupen64Plus-Next");
    AddCore(&options, "parallel_n64_libretro.so", "ParaLLEl N64");
  } else if (id == "fbneo" || id == "cps1" || id == "cps2" ||
             id == "cps3" || id == "neogeo") {
    AddCore(&options, "fbneo_libretro.so", "FinalBurn Neo");
    AddCore(&options, "fbalpha2012_libretro.so", "FB Alpha 2012");
    AddCore(&options, "mame2003_xtreme_libretro.so", "MAME 2003");
    AddCore(&options, "mame2003_plus_libretro.so", "MAME 2003-Plus");
    AddCore(&options, "mame2010_libretro.so", "MAME 2010");
    AddCore(&options, "mame2000_libretro.so", "MAME 2000");
  } else if (id == "mame" || id == "varcade") {
    AddCore(&options, "mame2022xtreme_libretro.so", "MAME 2022 Xtreme");
    AddCore(&options, "mame2003_xtreme_libretro.so", "MAME 2003");
    AddCore(&options, "mame2003_plus_libretro.so", "MAME 2003-Plus");
    AddCore(&options, "mame2010_libretro.so", "MAME 2010");
    AddCore(&options, "mame2000_libretro.so", "MAME 2000");
    AddCore(&options, "fbneo_libretro.so", "FinalBurn Neo");
    AddCore(&options, "fbalpha2012_libretro.so", "FB Alpha 2012");
  } else {
    return {};
  }
  return options;
}

void EnsureCoreVisible(UiSession *session) {
  if (!session) return;
  const int count = static_cast<int>(CoreOptionsForGame(*session).size());
  if (count <= 0) {
    session->core_selected_index = 0;
    session->core_scroll_offset = 0;
    return;
  }
  session->core_selected_index = ClampIndex(session->core_selected_index, 0, count - 1);
  if (session->core_selected_index < session->core_scroll_offset) {
    session->core_scroll_offset = session->core_selected_index;
  }
  if (session->core_selected_index >= session->core_scroll_offset + kVisibleCoreRows) {
    session->core_scroll_offset = session->core_selected_index - kVisibleCoreRows + 1;
  }
  session->core_scroll_offset = ClampIndex(session->core_scroll_offset, 0,
                                          std::max(0, count - kVisibleCoreRows));
}

int CountStandaloneGamesForPlatform(const Library &library, const std::string &platform_id) {
  return static_cast<int>(std::count_if(library.games.begin(), library.games.end(),
                                        [&](const Game &game) {
                                          return game.platform_id == platform_id &&
                                                 game.collection_id.empty();
                                        }));
}

int CountFavorites(const Library &library) {
  return static_cast<int>(std::count_if(library.games.begin(), library.games.end(),
                                        [](const Game &game) { return game.favorite; }));
}

int CountRecent(const Library &library) {
  return static_cast<int>(std::count_if(library.games.begin(), library.games.end(),
                                        [](const Game &game) {
                                          return game.recent_order != 0;
                                        }));
}

int CountCollection(const Library &library, const std::string &collection_id) {
  return static_cast<int>(std::count_if(library.games.begin(), library.games.end(),
                                        [&](const Game &game) {
                                          return game.collection_id == collection_id;
                                        }));
}

std::vector<UiNavItem> BuildNavigation(const Library &library, bool include_empty_platforms) {
  std::vector<UiNavItem> items;
  items.push_back(UiNavItem{UiNavKind::Recent, "recent", "最近游戏", CountRecent(library), true});
  items.push_back(UiNavItem{UiNavKind::Favorites, "favorites", "收藏", CountFavorites(library),
                            true});
  items.push_back(UiNavItem{UiNavKind::AllGames, "all", "全部游戏",
                            static_cast<int>(library.games.size()), true});

  std::map<std::string, std::string> collection_titles;
  for (const Game &game : library.games) {
    if (game.collection_id.empty() || game.collection_title.empty()) continue;
    collection_titles.emplace(game.collection_id, game.collection_title);
  }
  for (const auto &entry : collection_titles) {
    const int count = CountCollection(library, entry.first);
    if (count <= 0) continue;
    items.push_back(UiNavItem{UiNavKind::Collection, entry.first, entry.second, count, true});
  }

  std::vector<const Platform *> platforms;
  for (const Platform &platform : library.platforms) platforms.push_back(&platform);
  std::sort(platforms.begin(), platforms.end(), [](const Platform *left, const Platform *right) {
    if (left->sort_order != right->sort_order) return left->sort_order < right->sort_order;
    return left->display_name < right->display_name;
  });

  for (const Platform *platform : platforms) {
    const int count = CountStandaloneGamesForPlatform(library, platform->id);
    if (!platform->launchable) continue;
    if (!include_empty_platforms && count == 0) continue;
    items.push_back(UiNavItem{UiNavKind::Platform, platform->id, platform->display_name, count,
                              platform->launchable});
  }
  return items;
}

bool GameVisibleForNav(const Game &game, const UiNavItem &nav) {
  switch (nav.kind) {
    case UiNavKind::Recent:
      return game.recent_order != 0;
    case UiNavKind::Favorites:
      return game.favorite;
    case UiNavKind::AllGames:
      return true;
    case UiNavKind::Collection:
      return game.collection_id == nav.id;
    case UiNavKind::Platform:
      return game.platform_id == nav.id && game.collection_id.empty();
  }
  return true;
}

void RebuildVisibleGames(UiSession *session, const std::string &preferred_game_id = {}) {
  if (!session) return;
  session->visible_game_indices.clear();
  if (session->navigation.empty()) {
    session->selected_visible_index = 0;
    session->scroll_offset = 0;
    return;
  }

  const UiNavItem &nav = session->navigation[session->active_nav_index];
  for (std::size_t index = 0; index < session->library.games.size(); ++index) {
    if (GameVisibleForNav(session->library.games[index], nav)) {
      session->visible_game_indices.push_back(index);
    }
  }

  std::sort(session->visible_game_indices.begin(), session->visible_game_indices.end(),
            [&](std::size_t left_index, std::size_t right_index) {
              const Game &left = session->library.games[left_index];
              const Game &right = session->library.games[right_index];
              if (nav.kind == UiNavKind::Recent && left.recent_order != right.recent_order) {
                return left.recent_order > right.recent_order;
              }
              if (left.sort_key != right.sort_key) return left.sort_key < right.sort_key;
              if (left.title != right.title) return left.title < right.title;
              return left.id < right.id;
            });

  int preferred_index = 0;
  if (!preferred_game_id.empty()) {
    for (std::size_t index = 0; index < session->visible_game_indices.size(); ++index) {
      const Game &game = session->library.games[session->visible_game_indices[index]];
      if (game.id == preferred_game_id) {
        preferred_index = static_cast<int>(index);
        break;
      }
    }
  }

  const int max_index = static_cast<int>(session->visible_game_indices.size()) - 1;
  session->selected_visible_index = max_index >= 0 ? ClampIndex(preferred_index, 0, max_index) : 0;
  session->scroll_offset = std::max(0, std::min(session->scroll_offset,
      std::max(0, static_cast<int>(session->visible_game_indices.size()) - 1)));
}

void EnsureSelectionVisible(UiSession *session, const UiLayout &layout) {
  if (!session || session->visible_game_indices.empty()) return;
  const int columns = GridColumnsForPreferences(session->preferences);
  const int rows = GridRowsForPreferences(session->preferences, layout);
  const int page_size = std::max(1, columns * rows);
  if (session->selected_visible_index < session->scroll_offset) {
    session->scroll_offset = session->selected_visible_index;
  }
  if (session->selected_visible_index >= session->scroll_offset + page_size) {
    session->scroll_offset = session->selected_visible_index - page_size + 1;
  }
}

void EnsureSettingsVisible(UiSession *session) {
  if (!session) return;
  session->settings_selected_index = ClampIndex(session->settings_selected_index, 0,
                                               kSettingsCount - 1);
  if (session->settings_selected_index < session->settings_scroll_offset) {
    session->settings_scroll_offset = session->settings_selected_index;
  }
  if (session->settings_selected_index >= session->settings_scroll_offset +
                                              kVisibleSettingsRows) {
    session->settings_scroll_offset =
        session->settings_selected_index - kVisibleSettingsRows + 1;
  }
  session->settings_scroll_offset = ClampIndex(session->settings_scroll_offset, 0,
                                              std::max(0, kSettingsCount -
                                                            kVisibleSettingsRows));
}

void EnsureThemeVisible(UiSession *session) {
  if (!session) return;
  session->theme_selected_index = ClampIndex(session->theme_selected_index, 0,
                                            kUiThemeColorCount - 1);
  if (session->theme_selected_index < session->theme_scroll_offset) {
    session->theme_scroll_offset = session->theme_selected_index;
  }
  if (session->theme_selected_index >= session->theme_scroll_offset + kVisibleThemeRows) {
    session->theme_scroll_offset = session->theme_selected_index - kVisibleThemeRows + 1;
  }
  session->theme_scroll_offset = ClampIndex(session->theme_scroll_offset, 0,
                                           std::max(0, kUiThemeColorCount -
                                                         kVisibleThemeRows));
}

void EnsureTargetVisible(UiSession *session) {
  if (!session) return;
  const int count = TargetCount(SelectedGame(*session));
  if (count <= 0) {
    session->target_selected_index = 0;
    session->target_scroll_offset = 0;
    return;
  }
  session->target_selected_index = ClampIndex(session->target_selected_index, 0, count - 1);
  if (session->target_selected_index < session->target_scroll_offset) {
    session->target_scroll_offset = session->target_selected_index;
  }
  if (session->target_selected_index >= session->target_scroll_offset + kVisibleTargetRows) {
    session->target_scroll_offset = session->target_selected_index - kVisibleTargetRows + 1;
  }
  session->target_scroll_offset = ClampIndex(session->target_scroll_offset, 0,
                                            std::max(0, count - kVisibleTargetRows));
}

std::string SelectedGameId(const UiSession &session) {
  if (session.visible_game_indices.empty()) return {};
  const int index = ClampIndex(session.selected_visible_index, 0,
                              static_cast<int>(session.visible_game_indices.size()) - 1);
  return session.library.games[session.visible_game_indices[index]].id;
}

void MoveNav(UiSession *session, int delta) {
  if (!session || session->navigation.empty()) return;
  const int size = static_cast<int>(session->navigation.size());
  session->active_nav_index = (session->active_nav_index + delta + size) % size;
  session->scroll_offset = 0;
  RebuildVisibleGames(session);
}

}  // namespace

UiSession CreateUiSession(Library library, const UiState &state, bool include_empty_platforms) {
  UiSession session;
  session.library = std::move(library);
  session.include_empty_platforms = include_empty_platforms;
  session.preferences = PreferencesFromState(state);
  session.view = UiView::Library;
  session.settings_selected_index = ClampIndex(state.settings_selected_index, 0,
                                              kSettingsCount - 1);
  session.settings_scroll_offset = std::max(0, state.settings_scroll_offset);
  session.description_scroll_line = 0;
  session.navigation = BuildNavigation(session.library, include_empty_platforms);

  bool restored_nav = false;
  for (std::size_t index = 0; index < session.navigation.size(); ++index) {
    const UiNavItem &item = session.navigation[index];
    if (item.id == state.active_platform_id) {
      session.active_nav_index = static_cast<int>(index);
      restored_nav = true;
      break;
    }
  }
  if (!restored_nav && !state.selected_game_id.empty()) {
    std::string fallback_nav_id;
    for (const Game &game : session.library.games) {
      if (game.id != state.selected_game_id) continue;
      fallback_nav_id = game.collection_id.empty() ? game.platform_id : game.collection_id;
      break;
    }
    for (std::size_t index = 0; index < session.navigation.size(); ++index) {
      if (session.navigation[index].id == fallback_nav_id) {
        session.active_nav_index = static_cast<int>(index);
        break;
      }
    }
  }
  session.scroll_offset = std::max(0, state.scroll_offset);
  RebuildVisibleGames(&session, state.selected_game_id);
  EnsureSettingsVisible(&session);
  return session;
}

UiState ExportUiState(const UiSession &session) {
  UiState state;
  if (!session.navigation.empty()) {
    state.active_platform_id = session.navigation[session.active_nav_index].id;
  }
  if (const Game *game = SelectedGame(session)) {
    state.selected_game_id = game->id;
  }
  state.scroll_offset = std::max(0, session.scroll_offset);
  state.focus = "grid";
  state.settings_selected_index = std::max(0, session.settings_selected_index);
  state.settings_scroll_offset = std::max(0, session.settings_scroll_offset);
  state.description_scroll_line = 0;
  state.grid_size = GridSizeToInt(session.preferences.grid_size);
  state.bgm_mode = BgmModeToInt(session.preferences.bgm_mode);
  state.theme_color = ClampCycle(session.preferences.theme_color, kUiThemeColorCount);
  state.startup_logo_style = ClampCycle(session.preferences.startup_logo_style,
                                        kUiStartupLogoStyleCount);
  state.cover_title_size_level = ClampCycle(session.preferences.cover_title_size_level,
                                           kFontSizeLevelCount);
  state.description_size_level = ClampCycle(session.preferences.description_size_level,
                                           kFontSizeLevelCount);
  state.show_cover_titles = session.preferences.show_cover_titles;
  state.fullscreen_grid = session.preferences.fullscreen_grid;
  state.preview_video_enabled = session.preferences.preview_video_enabled;
  state.preview_video_loop = session.preferences.preview_video_loop;
  return state;
}

UiActionResult ApplyUiAction(UiSession *session, UiAction action, const UiLayout &layout) {
  UiActionResult result;
  if (!session) return result;

  if (session->notice.visible &&
      (action == UiAction::Confirm || action == UiAction::Back || action == UiAction::Menu)) {
    session->notice = UiNotice{};
    return result;
  }

  if (action == UiAction::MenuPress) {
    session->menu_button_held = true;
    session->menu_chord_used = false;
    return result;
  }
  if (action == UiAction::MenuRelease) {
    const bool open_menu = session->menu_button_held && !session->menu_chord_used;
    session->menu_button_held = false;
    session->menu_chord_used = false;
    if (open_menu) {
      if (session->view == UiView::Library) {
        session->view = UiView::Settings;
        session->settings_selected_index = 0;
        session->settings_scroll_offset = 0;
      } else if (session->view == UiView::Settings) {
        session->view = UiView::Library;
      } else if (session->view == UiView::About ||
                 session->view == UiView::Hotkeys) {
        session->view = UiView::Settings;
      }
    }
    return result;
  }

  if (action == UiAction::AdjustVolumeDown || action == UiAction::AdjustVolumeUp) {
    result.intent = UiIntent::AdjustVolume;
    result.delta = action == UiAction::AdjustVolumeDown ? -1 : 1;
    return result;
  }

  if (action == UiAction::Power) {
    result.intent = UiIntent::SuspendSystem;
    return result;
  }

  if (session->menu_button_held && session->view == UiView::Library &&
      (action == UiAction::Up || action == UiAction::Down ||
       action == UiAction::Left || action == UiAction::Right)) {
    session->menu_chord_used = true;
    if (action == UiAction::Up) action = UiAction::JumpPrevious;
    else if (action == UiAction::Down) action = UiAction::JumpNext;
    else if (action == UiAction::Left) action = UiAction::GridSmaller;
    else action = UiAction::GridLarger;
  }

  if (session->view == UiView::Settings) {
    if (action == UiAction::Back || action == UiAction::Menu) {
      session->view = UiView::Library;
      return result;
    }
    if (action == UiAction::Up) {
      session->settings_selected_index =
          (session->settings_selected_index + kSettingsCount - 1) % kSettingsCount;
      EnsureSettingsVisible(session);
      return result;
    }
    if (action == UiAction::Down) {
      session->settings_selected_index =
          (session->settings_selected_index + 1) % kSettingsCount;
      EnsureSettingsVisible(session);
      return result;
    }

    const bool adjust = action == UiAction::Left || action == UiAction::Right ||
                        action == UiAction::Confirm;
    const int direction = action == UiAction::Left ? -1 : 1;
    if (adjust) {
      switch (session->settings_selected_index) {
        case 0:
          result.intent = UiIntent::ReturnToSystem;
          break;
        case 1:
          session->view = UiView::Hotkeys;
          break;
        case 2:
          if (session->system_status.autostart_enabled >= 0) {
            result.intent = UiIntent::SetAutostart;
            result.delta = session->system_status.autostart_enabled > 0 ? 0 : 1;
          }
          break;
        case 3:
          result.intent = UiIntent::AdjustBrightness;
          result.delta = direction;
          break;
        case 4:
          result.intent = UiIntent::AdjustVolume;
          result.delta = direction;
          break;
        case 5:
          if (action == UiAction::Left) {
            if (!session->preferences.preview_video_enabled) {
              session->preferences.preview_video_enabled = true;
              session->preferences.preview_video_loop = true;
            } else if (session->preferences.preview_video_loop) {
              session->preferences.preview_video_loop = false;
            } else {
              session->preferences.preview_video_enabled = false;
            }
          } else {
            if (!session->preferences.preview_video_enabled) {
              session->preferences.preview_video_enabled = true;
              session->preferences.preview_video_loop = false;
            } else if (!session->preferences.preview_video_loop) {
              session->preferences.preview_video_loop = true;
            } else {
              session->preferences.preview_video_enabled = false;
            }
          }
          break;
        case 6:
          session->preferences.grid_size = GridSizeFromInt(
              ClampCycle(GridSizeToInt(session->preferences.grid_size) + direction, 3));
          EnsureSelectionVisible(session, layout);
          break;
        case 7:
          session->preferences.fullscreen_grid = !session->preferences.fullscreen_grid;
          EnsureSelectionVisible(session, layout);
          break;
        case 8:
          session->view = UiView::ThemeSelect;
          session->theme_selected_index =
              ClampCycle(session->preferences.theme_color, kUiThemeColorCount);
          session->theme_scroll_offset = 0;
          EnsureThemeVisible(session);
          break;
        case 9:
          session->preferences.startup_logo_style = ClampCycle(
              session->preferences.startup_logo_style + direction,
              kUiStartupLogoStyleCount);
          break;
        case 10:
          session->preferences.show_cover_titles = !session->preferences.show_cover_titles;
          break;
        case 11:
          session->preferences.cover_title_size_level = ClampCycle(
              session->preferences.cover_title_size_level + direction, kFontSizeLevelCount);
          break;
        case 12:
          session->preferences.description_size_level = ClampCycle(
              session->preferences.description_size_level + direction, kFontSizeLevelCount);
          session->description_scroll_line = 0;
          break;
        case 13:
          result.intent = UiIntent::ClearCacheAndRescan;
          break;
        case 14:
          result.intent = UiIntent::RestartSystem;
          break;
        case 15:
          result.intent = UiIntent::ShutdownSystem;
          break;
        case 16:
          session->view = UiView::About;
          break;
      }
    }
    return result;
  }

  if (session->view == UiView::About) {
    if (action == UiAction::Back || action == UiAction::Menu ||
        action == UiAction::Confirm) {
      session->view = UiView::Settings;
      EnsureSettingsVisible(session);
    }
    return result;
  }

  if (session->view == UiView::Hotkeys) {
    if (action == UiAction::Back || action == UiAction::Menu ||
        action == UiAction::Confirm) {
      session->view = UiView::Settings;
      EnsureSettingsVisible(session);
    }
    return result;
  }

  if (session->view == UiView::ThemeSelect) {
    if (action == UiAction::Back || action == UiAction::Menu ||
        action == UiAction::Confirm) {
      session->view = UiView::Settings;
      EnsureSettingsVisible(session);
      return result;
    }
    if (action == UiAction::Up || action == UiAction::Left) {
      session->theme_selected_index =
          (session->theme_selected_index + kUiThemeColorCount - 1) % kUiThemeColorCount;
      session->preferences.theme_color = session->theme_selected_index;
      EnsureThemeVisible(session);
      return result;
    }
    if (action == UiAction::Down || action == UiAction::Right) {
      session->theme_selected_index =
          (session->theme_selected_index + 1) % kUiThemeColorCount;
      session->preferences.theme_color = session->theme_selected_index;
      EnsureThemeVisible(session);
      return result;
    }
    return result;
  }

  if (session->view == UiView::TargetSelect) {
    Game *game = SelectedGame(session);
    const int count = TargetCount(game);
    if (!game || count <= 1 || action == UiAction::Back || action == UiAction::Menu) {
      session->view = UiView::Library;
      return result;
    }
    if (action == UiAction::Up) {
      session->target_selected_index = (session->target_selected_index + count - 1) % count;
      EnsureTargetVisible(session);
      return result;
    }
    if (action == UiAction::Down) {
      session->target_selected_index = (session->target_selected_index + 1) % count;
      EnsureTargetVisible(session);
      return result;
    }
    if (action == UiAction::Confirm) {
      const LaunchTarget *target = TargetAt(game, session->target_selected_index);
      if (target) {
        result.intent = UiIntent::LaunchGame;
        result.game_id = game->id;
        result.target_index = session->target_selected_index;
        result.target_path = target->path;
      }
      return result;
    }
    return result;
  }

  if (session->view == UiView::CoreSelect) {
    Game *game = SelectedGame(session);
    const std::vector<UiCoreOption> options = CoreOptionsForGame(*session);
    const int count = static_cast<int>(options.size());
    if (!game || count <= 0 || action == UiAction::Back || action == UiAction::Menu ||
        action == UiAction::OpenCoreSelect) {
      session->view = UiView::Library;
      return result;
    }
    if (action == UiAction::Up) {
      session->core_selected_index = (session->core_selected_index + count - 1) % count;
      EnsureCoreVisible(session);
      return result;
    }
    if (action == UiAction::Down) {
      session->core_selected_index = (session->core_selected_index + 1) % count;
      EnsureCoreVisible(session);
      return result;
    }
    if (action == UiAction::Confirm) {
      const int index = ClampIndex(session->core_selected_index, 0, count - 1);
      game->user_core_hint = options[index].core;
      session->view = UiView::Library;
      session->osd_text = "核心 " + options[index].label;
      session->osd_frames_remaining = 100;
      return result;
    }
    return result;
  }

  const std::string previous_game_id = SelectedGameId(*session);
  const int max_index = static_cast<int>(session->visible_game_indices.size()) - 1;
  switch (action) {
    case UiAction::TabPrevious:
      MoveNav(session, -1);
      break;
    case UiAction::TabNext:
      MoveNav(session, 1);
      break;
    case UiAction::Up:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index -
                           GridColumnsForPreferences(session->preferences), 0, max_index);
      }
      break;
    case UiAction::Down:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index +
                           GridColumnsForPreferences(session->preferences), 0, max_index);
      }
      break;
    case UiAction::Left:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index - 1, 0, max_index);
      }
      break;
    case UiAction::Right:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index + 1, 0, max_index);
      }
      break;
    case UiAction::ToggleFavorite:
      if (Game *game = SelectedGame(session)) {
        const std::string selected_id = game->id;
        game->favorite = !game->favorite;
        session->navigation = BuildNavigation(session->library, session->include_empty_platforms);
        session->active_nav_index = ClampIndex(session->active_nav_index, 0,
            static_cast<int>(session->navigation.size()) - 1);
        RebuildVisibleGames(session, selected_id);
      }
      break;
    case UiAction::ToggleTitles:
      session->preferences.show_cover_titles = !session->preferences.show_cover_titles;
      break;
    case UiAction::ToggleFullscreenGrid:
      session->preferences.fullscreen_grid = !session->preferences.fullscreen_grid;
      EnsureSelectionVisible(session, layout);
      break;
    case UiAction::OpenCoreSelect: {
      const std::vector<UiCoreOption> options = CoreOptionsForGame(*session);
      Game *game = SelectedGame(session);
      if (!game || options.empty()) {
        session->notice = UiNotice{true, "核心选择不可用", "当前平台不使用可切换的RetroArch核心。"};
        break;
      }
      session->view = UiView::CoreSelect;
      session->core_selected_index = 0;
      const std::string current = game->user_core_hint;
      for (std::size_t index = 0; index < options.size(); ++index) {
        if (options[index].core == current) {
          session->core_selected_index = static_cast<int>(index);
          break;
        }
      }
      session->core_scroll_offset = 0;
      EnsureCoreVisible(session);
      break;
    }
    case UiAction::GridSmaller:
      session->preferences.grid_size = GridSizeFromInt(
          ClampCycle(GridSizeToInt(session->preferences.grid_size) + 1, 3));
      EnsureSelectionVisible(session, layout);
      break;
    case UiAction::GridLarger:
      session->preferences.grid_size = GridSizeFromInt(
          ClampCycle(GridSizeToInt(session->preferences.grid_size) - 1, 3));
      EnsureSelectionVisible(session, layout);
      break;
    case UiAction::QuickTheme:
      session->preferences.theme_color =
          ClampCycle(session->preferences.theme_color + 1, kUiThemeColorCount);
      break;
    case UiAction::JumpPrevious:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index - 100, 0, max_index);
      }
      break;
    case UiAction::JumpNext:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index + 100, 0, max_index);
      }
      break;
    case UiAction::PagePrevious:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index -
                           GridPageSizeForPreferences(session->preferences, layout),
                       0, max_index);
      }
      break;
    case UiAction::PageNext:
      if (max_index >= 0) {
        session->selected_visible_index =
            ClampIndex(session->selected_visible_index +
                           GridPageSizeForPreferences(session->preferences, layout),
                       0, max_index);
      }
      break;
    case UiAction::NextBgm:
      session->preferences.bgm_mode = UiBgmMode::Music;
      ++session->bgm_track_revision;
      break;
    case UiAction::AdjustVolumeDown:
    case UiAction::AdjustVolumeUp:
    case UiAction::Power:
      break;
    case UiAction::Confirm:
      if (const Game *game = SelectedGame(*session)) {
        if (TargetCount(game) > 1) {
          session->view = UiView::TargetSelect;
          session->target_selected_index = 0;
          session->target_scroll_offset = 0;
        } else {
          result.intent = UiIntent::LaunchGame;
          result.game_id = game->id;
          result.target_index = 0;
          result.target_path = game->primary_target.path;
        }
      }
      break;
    case UiAction::Back:
      result.intent = UiIntent::Back;
      break;
    case UiAction::Menu:
      session->view = UiView::Settings;
      session->settings_selected_index = 0;
      session->settings_scroll_offset = 0;
      break;
    case UiAction::DescriptionUp:
      session->description_scroll_line = std::max(0, session->description_scroll_line - 1);
      break;
    case UiAction::DescriptionDown:
      session->description_scroll_line = std::min(9999, session->description_scroll_line + 1);
      break;
    case UiAction::MenuPress:
    case UiAction::MenuRelease:
      break;
  }

  EnsureSelectionVisible(session, layout);
  if (SelectedGameId(*session) != previous_game_id) {
    session->description_scroll_line = 0;
  }
  return result;
}

const Game *SelectedGame(const UiSession &session) {
  if (session.visible_game_indices.empty()) return nullptr;
  const int index = ClampIndex(session.selected_visible_index, 0,
                              static_cast<int>(session.visible_game_indices.size()) - 1);
  return &session.library.games[session.visible_game_indices[index]];
}

Game *SelectedGame(UiSession *session) {
  if (!session || session->visible_game_indices.empty()) return nullptr;
  const int index = ClampIndex(session->selected_visible_index, 0,
                              static_cast<int>(session->visible_game_indices.size()) - 1);
  return &session->library.games[session->visible_game_indices[index]];
}

std::vector<const Game *> VisibleGames(const UiSession &session) {
  std::vector<const Game *> games;
  games.reserve(session.visible_game_indices.size());
  for (const std::size_t index : session.visible_game_indices) {
    games.push_back(&session.library.games[index]);
  }
  return games;
}

std::vector<UiCoreOption> CoreOptionsForGame(const UiSession &session) {
  const Platform *platform = SelectedPlatform(session);
  std::vector<UiCoreOption> options =
      platform ? PlatformCoreOptions(*platform) : std::vector<UiCoreOption>{};
  if (!options.empty() && options.front().core.empty()) {
    const std::string automatic_core = AutomaticCoreHintForGame(session);
    options.front().label = automatic_core.empty()
                                ? "自动: 系统默认"
                                : "自动: " + CoreDisplayName(automatic_core);
  }
  return options;
}

std::string CoreDisplayName(const std::string &core) {
  if (core.empty()) return "自动";
  struct NameMap {
    const char *core;
    const char *label;
  };
  static const NameMap kNames[] = {
      {"mgba_libretro.so", "mGBA"},
      {"gpsp_libretro.so", "gpSP"},
      {"vbam_libretro.so", "VBA-M"},
      {"vba_next_libretro.so", "VBA Next"},
      {"gambatte_libretro.so", "Gambatte"},
      {"tgbdual_libretro.so", "TGB Dual"},
      {"gearboy_libretro.so", "Gearboy"},
      {"sameboy_libretro.so", "SameBoy"},
      {"fceumm_libretro.so", "FCEUmm"},
      {"nestopia_libretro.so", "Nestopia"},
      {"mesen_libretro.so", "Mesen"},
      {"quicknes_libretro.so", "QuickNES"},
      {"snes9x2005_plus_libretro.so", "Snes9x 2005 Plus"},
      {"snes9x_libretro.so", "Snes9x"},
      {"snes9x2002_libretro.so", "Snes9x 2002"},
      {"snes9x2005_libretro.so", "Snes9x 2005"},
      {"snes9x2010_libretro.so", "Snes9x 2010"},
      {"genesis_plus_gx_libretro.so", "Genesis Plus GX"},
      {"picodrive_libretro.so", "PicoDrive"},
      {"pcsx_rearmed_libretro.so", "PCSX-ReARMed"},
      {"pcsx_rearmed_peops_libretro.so", "PCSX-ReARMed PEOPS"},
      {"pcsx_rearmed_rumble_libretro.so", "PCSX-ReARMed Rumble"},
      {"swanstation_libretro.so", "SwanStation"},
      {"mupen64plus_next_libretro.so", "Mupen64Plus-Next"},
      {"parallel_n64_libretro.so", "ParaLLEl N64"},
      {"fbneo_libretro.so", "FinalBurn Neo"},
      {"fbalpha2012_libretro.so", "FB Alpha 2012"},
      {"mame2022xtreme_libretro.so", "MAME 2022 Xtreme"},
      {"mame2003_xtreme_libretro.so", "MAME 2003"},
      {"mame2003_plus_libretro.so", "MAME 2003-Plus"},
      {"mame2010_libretro.so", "MAME 2010"},
      {"mame2000_libretro.so", "MAME 2000"},
  };
  for (const NameMap &entry : kNames) {
    if (core == entry.core) return entry.label;
  }
  return core;
}

std::string CurrentCoreDisplayName(const UiSession &session) {
  const Game *game = SelectedGame(session);
  if (!game) return "自动";
  if (!game->user_core_hint.empty()) return CoreDisplayName(game->user_core_hint);
  const std::string automatic_core = AutomaticCoreHintForGame(session);
  return automatic_core.empty() ? "自动: 系统默认"
                                : "自动: " + CoreDisplayName(automatic_core);
}

std::string EffectiveCoreHint(const Game &game) {
  return game.user_core_hint.empty() ? game.launch_hint.core_hint : game.user_core_hint;
}

}  // namespace mpl

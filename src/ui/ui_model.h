#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "domain/game.h"
#include "domain/library.h"
#include "services/state_store.h"
#include "ui/layout.h"

namespace mpl {

enum class UiAction {
  Up,
  Down,
  Left,
  Right,
  Confirm,
  Back,
  ToggleFavorite,
  ToggleTitles,
  ToggleFullscreenGrid,
  OpenCoreSelect,
  GridSmaller,
  GridLarger,
  QuickTheme,
  JumpPrevious,
  JumpNext,
  NextBgm,
  AdjustVolumeDown,
  AdjustVolumeUp,
  TabPrevious,
  TabNext,
  PagePrevious,
  PageNext,
  Menu,
  MenuPress,
  MenuRelease,
  Power,
  DescriptionUp,
  DescriptionDown,
};

enum class UiNavKind {
  Recent,
  Favorites,
  AllGames,
  Collection,
  Platform,
};

enum class UiIntent {
  None,
  LaunchGame,
  OpenMenu,
  Back,
  ExitFrontend,
  ReturnToSystem,
  SuspendSystem,
  RestartSystem,
  ShutdownSystem,
  AdjustBrightness,
  AdjustVolume,
  SetAutostart,
  ClearCacheAndRescan,
};

enum class UiView {
  Library,
  Settings,
  About,
  Hotkeys,
  ThemeSelect,
  TargetSelect,
  CoreSelect,
};

enum class UiGridSize {
  Large,
  Medium,
  Small,
};

enum class UiBgmMode {
  Music,
  GameAudio,
  Muted,
};

constexpr int kUiThemeColorCount = 25;
constexpr int kUiStartupLogoStyleCount = 2;

struct UiPreferences {
  UiGridSize grid_size = UiGridSize::Medium;
  UiBgmMode bgm_mode = UiBgmMode::GameAudio;
  int theme_color = 0;
  int startup_logo_style = 0;
  int cover_title_size_level = 1;
  int description_size_level = 1;
  bool show_cover_titles = true;
  bool fullscreen_grid = false;
  bool preview_video_enabled = true;
  bool preview_video_loop = true;
};

struct UiNotice {
  bool visible = false;
  std::string title;
  std::string message;
};

struct UiSystemStatus {
  int battery_percent = -1;
  bool charging = false;
  int brightness = -1;
  int volume = -1;
  int autostart_enabled = -1;
};

struct UiNavItem {
  UiNavKind kind = UiNavKind::AllGames;
  std::string id;
  std::string title;
  int game_count = 0;
  bool launchable = true;
};

struct UiCoreOption {
  std::string core;
  std::string label;
};

struct UiActionResult {
  UiIntent intent = UiIntent::None;
  std::string game_id;
  int target_index = 0;
  std::string target_path;
  int delta = 0;
};

struct UiSession {
  Library library;
  std::vector<UiNavItem> navigation;
  std::vector<std::size_t> visible_game_indices;
  int active_nav_index = 0;
  int selected_visible_index = 0;
  int scroll_offset = 0;
  UiView view = UiView::Library;
  int settings_selected_index = 0;
  int settings_scroll_offset = 0;
  int theme_selected_index = 0;
  int theme_scroll_offset = 0;
  int target_selected_index = 0;
  int target_scroll_offset = 0;
  int core_selected_index = 0;
  int core_scroll_offset = 0;
  int description_scroll_line = 0;
  UiPreferences preferences;
  UiNotice notice;
  UiSystemStatus system_status;
  std::string osd_text;
  int osd_frames_remaining = 0;
  bool include_empty_platforms = false;
  bool menu_button_held = false;
  bool menu_chord_used = false;
  int bgm_track_revision = 0;
};

UiSession CreateUiSession(Library library, const UiState &state,
                          bool include_empty_platforms = false);
UiState ExportUiState(const UiSession &session);
UiActionResult ApplyUiAction(UiSession *session, UiAction action,
                             const UiLayout &layout = ResolveUiLayout(720, 480));
const Game *SelectedGame(const UiSession &session);
Game *SelectedGame(UiSession *session);
std::vector<const Game *> VisibleGames(const UiSession &session);
std::vector<UiCoreOption> CoreOptionsForGame(const UiSession &session);
std::string CoreDisplayName(const std::string &core);
std::string CurrentCoreDisplayName(const UiSession &session);
std::string EffectiveCoreHint(const Game &game);

}  // namespace mpl

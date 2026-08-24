#include "services/state_store.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_map>

namespace fs = std::filesystem;

namespace mpl {

namespace {

constexpr int kCurrentFontSizeScaleVersion = 2;
constexpr int kCurrentStartupLogoStyleVersion = 1;

int MigrateCoverTitleSizeLevel(int level) {
  switch (level) {
    case 0:
      return 0;
    case 1:
      return 3;
    case 2:
      return 5;
    default:
      return level;
  }
}

int MigrateDescriptionSizeLevel(int level) {
  switch (level) {
    case 0:
      return 0;
    case 1:
      return 4;
    case 2:
      return 5;
    default:
      return level;
  }
}

struct StoredGameState {
  bool favorite = false;
  std::uint64_t recent_order = 0;
  std::string user_core_hint;
};

bool AtomicWrite(const std::string &path_text, const std::string &content) {
  const fs::path path = fs::u8path(path_text);
  std::error_code error;
  fs::create_directories(path.parent_path(), error);
  const fs::path temporary = fs::u8path(path_text + ".tmp");
  {
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) return false;
    out << content;
    if (!out) return false;
  }
  fs::rename(temporary, path, error);
  if (!error) return true;
  fs::remove(path, error);
  error.clear();
  fs::rename(temporary, path, error);
  return !error;
}

bool ParseUint64(const std::string &text, std::uint64_t *value) {
  std::uint64_t parsed_value = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed_value);
  if (result.ec != std::errc() || result.ptr != text.data() + text.size()) return false;
  *value = parsed_value;
  return true;
}

}  // namespace

GameStateStore::GameStateStore(std::string path) : path_(std::move(path)) {}

void GameStateStore::Load(std::vector<Game> *games) const {
  if (!games) return;
  std::ifstream in(fs::u8path(path_));
  if (!in) return;

  std::unordered_map<std::string, StoredGameState> states;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line.front() == '#') continue;
    std::istringstream row(line);
    std::string id;
    std::string favorite;
    std::string recent;
    std::string user_core_hint;
    if (!std::getline(row, id, '\t') || !std::getline(row, favorite, '\t') ||
        !std::getline(row, recent, '\t')) {
      continue;
    }
    std::uint64_t recent_order = 0;
    if (!ParseUint64(recent, &recent_order)) continue;
    std::getline(row, user_core_hint, '\t');
    states[id] = StoredGameState{favorite == "1", recent_order, user_core_hint};
  }

  for (Game &game : *games) {
    const auto found = states.find(game.id);
    if (found == states.end()) continue;
    game.favorite = found->second.favorite;
    game.recent_order = found->second.recent_order;
    game.user_core_hint = found->second.user_core_hint;
  }
}

bool GameStateStore::Save(const std::vector<Game> &games) const {
  std::ostringstream out;
  out << "# id\tfavorite\trecent_order\tuser_core_hint\n";
  for (const Game &game : games) {
    if (!game.favorite && game.recent_order == 0 && game.user_core_hint.empty()) continue;
    out << game.id << '\t' << (game.favorite ? 1 : 0) << '\t'
        << game.recent_order << '\t' << game.user_core_hint << '\n';
  }
  return AtomicWrite(path_, out.str());
}

bool GameStateStore::LimitRecent(std::vector<Game> *games, std::size_t max_items) const {
  if (!games) return false;
  std::vector<Game *> recent;
  for (Game &game : *games) {
    if (game.recent_order != 0) recent.push_back(&game);
  }
  std::sort(recent.begin(), recent.end(), [](const Game *left, const Game *right) {
    return left->recent_order > right->recent_order;
  });
  bool changed = false;
  for (std::size_t index = max_items; index < recent.size(); ++index) {
    recent[index]->recent_order = 0;
    changed = true;
  }
  return changed;
}

std::uint64_t GameStateStore::NextRecentOrder(const std::vector<Game> &games) const {
  std::uint64_t max_order = 0;
  for (const Game &game : games) max_order = std::max(max_order, game.recent_order);
  return max_order + 1;
}

UiStateStore::UiStateStore(std::string path) : path_(std::move(path)) {}

bool UiStateStore::Load(UiState *state) const {
  if (!state) return false;
  std::ifstream in(fs::u8path(path_));
  if (!in) return false;
  UiState loaded;
  loaded.font_size_scale_version = 0;
  loaded.startup_logo_style_version = 0;
  std::string line;
  while (std::getline(in, line)) {
    const size_t equals = line.find('=');
    if (equals == std::string::npos) continue;
    const std::string key = line.substr(0, equals);
    const std::string value = line.substr(equals + 1);
    if (key == "active_platform_id") loaded.active_platform_id = value;
    else if (key == "selected_game_id") loaded.selected_game_id = value;
    else if (key == "scroll_offset") loaded.scroll_offset = std::max(0, std::atoi(value.c_str()));
    else if (key == "focus") loaded.focus = value;
    else if (key == "settings_selected_index") {
      loaded.settings_selected_index = std::max(0, std::atoi(value.c_str()));
    } else if (key == "settings_scroll_offset") {
      loaded.settings_scroll_offset = std::max(0, std::atoi(value.c_str()));
    } else if (key == "description_scroll_line") {
      loaded.description_scroll_line = std::max(0, std::atoi(value.c_str()));
    } else if (key == "grid_size") {
      loaded.grid_size = std::max(0, std::atoi(value.c_str()));
    } else if (key == "bgm_mode") {
      loaded.bgm_mode = std::max(0, std::atoi(value.c_str()));
    } else if (key == "theme_color") {
      loaded.theme_color = std::max(0, std::atoi(value.c_str()));
    } else if (key == "startup_logo_style") {
      loaded.startup_logo_style = std::max(0, std::atoi(value.c_str()));
    } else if (key == "cover_title_size_level") {
      loaded.cover_title_size_level = std::max(0, std::atoi(value.c_str()));
    } else if (key == "description_size_level") {
      loaded.description_size_level = std::max(0, std::atoi(value.c_str()));
    } else if (key == "font_size_scale_version") {
      loaded.font_size_scale_version = std::max(0, std::atoi(value.c_str()));
    } else if (key == "startup_logo_style_version") {
      loaded.startup_logo_style_version = std::max(0, std::atoi(value.c_str()));
    } else if (key == "show_cover_titles") {
      loaded.show_cover_titles = value != "0";
    } else if (key == "fullscreen_grid") {
      loaded.fullscreen_grid = value != "0";
    } else if (key == "preview_video_enabled") {
      loaded.preview_video_enabled = value != "0";
    } else if (key == "preview_video_loop") {
      loaded.preview_video_loop = value != "0";
    }
  }
  if (loaded.font_size_scale_version < kCurrentFontSizeScaleVersion) {
    loaded.cover_title_size_level =
        MigrateCoverTitleSizeLevel(loaded.cover_title_size_level);
    loaded.description_size_level =
        MigrateDescriptionSizeLevel(loaded.description_size_level);
    loaded.font_size_scale_version = kCurrentFontSizeScaleVersion;
  }
  if (loaded.startup_logo_style_version < kCurrentStartupLogoStyleVersion) {
    loaded.startup_logo_style = 1;
    loaded.startup_logo_style_version = kCurrentStartupLogoStyleVersion;
  }
  *state = std::move(loaded);
  return true;
}

bool UiStateStore::Save(const UiState &state) const {
  std::ostringstream out;
  out << "active_platform_id=" << state.active_platform_id << '\n';
  out << "selected_game_id=" << state.selected_game_id << '\n';
  out << "scroll_offset=" << std::max(0, state.scroll_offset) << '\n';
  out << "focus=" << state.focus << '\n';
  out << "settings_selected_index=" << std::max(0, state.settings_selected_index) << '\n';
  out << "settings_scroll_offset=" << std::max(0, state.settings_scroll_offset) << '\n';
  out << "description_scroll_line=" << std::max(0, state.description_scroll_line) << '\n';
  out << "grid_size=" << std::max(0, state.grid_size) << '\n';
  out << "bgm_mode=" << std::max(0, state.bgm_mode) << '\n';
  out << "theme_color=" << std::max(0, state.theme_color) << '\n';
  out << "startup_logo_style=" << std::max(0, state.startup_logo_style) << '\n';
  out << "cover_title_size_level=" << std::max(0, state.cover_title_size_level) << '\n';
  out << "description_size_level=" << std::max(0, state.description_size_level) << '\n';
  out << "font_size_scale_version=" << kCurrentFontSizeScaleVersion << '\n';
  out << "startup_logo_style_version=" << kCurrentStartupLogoStyleVersion << '\n';
  out << "show_cover_titles=" << (state.show_cover_titles ? 1 : 0) << '\n';
  out << "fullscreen_grid=" << (state.fullscreen_grid ? 1 : 0) << '\n';
  out << "preview_video_enabled=" << (state.preview_video_enabled ? 1 : 0) << '\n';
  out << "preview_video_loop=" << (state.preview_video_loop ? 1 : 0) << '\n';
  return AtomicWrite(path_, out.str());
}

}  // namespace mpl

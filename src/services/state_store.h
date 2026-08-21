#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "domain/game.h"

namespace mpl {

struct UiState {
  std::string active_platform_id;
  std::string selected_game_id;
  int scroll_offset = 0;
  std::string focus = "grid";
  int settings_selected_index = 0;
  int settings_scroll_offset = 0;
  int description_scroll_line = 0;
  int grid_size = 1;
  int bgm_mode = 1;
  int theme_color = 0;
  int startup_logo_style = 0;
  int cover_title_size_level = 1;
  int description_size_level = 1;
  bool show_cover_titles = true;
  bool fullscreen_grid = false;
  bool preview_video_enabled = true;
  bool preview_video_loop = true;
};

class GameStateStore {
 public:
  explicit GameStateStore(std::string path);

  void Load(std::vector<Game> *games) const;
  bool Save(const std::vector<Game> &games) const;
  bool LimitRecent(std::vector<Game> *games, std::size_t max_items) const;
  std::uint64_t NextRecentOrder(const std::vector<Game> &games) const;

 private:
  std::string path_;
};

class UiStateStore {
 public:
  explicit UiStateStore(std::string path);

  bool Load(UiState *state) const;
  bool Save(const UiState &state) const;

 private:
  std::string path_;
};

}  // namespace mpl

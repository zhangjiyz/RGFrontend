#pragma once

#include <algorithm>
#include <string>

namespace mpl {

class GridScrollTracker {
 public:
  int Resolve(const std::string &view_key, int raw_scroll_row,
              int selected_row, int visible_rows) {
    const int safe_raw_scroll_row = std::max(0, raw_scroll_row);
    const int safe_visible_rows = std::max(1, visible_rows);
    if (view_key != view_key_) {
      view_key_ = view_key;
      scroll_row_ = safe_raw_scroll_row;
      last_selected_row_ = selected_row;
      last_raw_scroll_row_ = safe_raw_scroll_row;
    } else if (selected_row == last_selected_row_ &&
               safe_raw_scroll_row != last_raw_scroll_row_) {
      scroll_row_ = safe_raw_scroll_row;
    }

    if (selected_row >= 0) {
      if (selected_row < scroll_row_) {
        scroll_row_ = selected_row;
      } else if (selected_row > scroll_row_ + safe_visible_rows) {
        scroll_row_ = selected_row - safe_visible_rows;
      }
    }

    scroll_row_ = std::max(0, scroll_row_);
    last_selected_row_ = selected_row;
    last_raw_scroll_row_ = safe_raw_scroll_row;
    return scroll_row_;
  }

  void Reset() {
    view_key_.clear();
    scroll_row_ = 0;
    last_selected_row_ = -1;
    last_raw_scroll_row_ = 0;
  }

 private:
  std::string view_key_;
  int scroll_row_ = 0;
  int last_selected_row_ = -1;
  int last_raw_scroll_row_ = 0;
};

}  // namespace mpl

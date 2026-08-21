#pragma once

namespace mpl {

enum class LayoutMode {
  Compact,
  Standard,
  Wide,
  Square,
  Tall,
};

struct Insets {
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
};

struct DisplayMetrics {
  int framebuffer_width = 0;
  int framebuffer_height = 0;
  int logical_width = 0;
  int logical_height = 720;
  double window_scale = 1.0;
  double density = 1.0;
};

struct UiLayout {
  int viewport_width = 0;
  int viewport_height = 0;
  DisplayMetrics display;
  LayoutMode mode = LayoutMode::Standard;
  Insets safe_area;

  int content_x = 0;
  int content_width = 0;
  bool expanded_sidebar = false;
  int nav_width = 0;
  int sidebar_width = 0;
  int sidebar_item_height = 0;
  int top_bar_height = 0;
  int filter_bar_height = 0;
  int section_header_height = 0;
  int bottom_bar_height = 0;
  int content_padding = 0;
  int grid_x = 0;
  int grid_y = 0;
  int grid_width = 0;
  int grid_height = 0;
  int grid_columns = 0;
  int grid_rows = 0;
  int card_width = 0;
  int card_cover_height = 0;
  int card_footer_height = 0;
  int card_height = 0;
  int card_gap = 0;
  int grid_gap = 0;
  int detail_x = 0;
  int detail_width = 0;
};

DisplayMetrics ResolveDisplayMetrics(int framebuffer_width, int framebuffer_height,
                                      double user_density = 0.0);
UiLayout ResolveUiLayout(int viewport_width, int viewport_height, Insets safe_area = {});
UiLayout ResolveUiLayoutForDisplay(int framebuffer_width, int framebuffer_height,
                                   Insets safe_area = {}, double user_density = 0.0);
const char *LayoutModeName(LayoutMode mode);

}  // namespace mpl

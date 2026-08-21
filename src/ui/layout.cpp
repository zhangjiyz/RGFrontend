#include "ui/layout.h"

#include <algorithm>
#include <cmath>

namespace mpl {

namespace {

int ClampInt(int value, int minimum, int maximum) {
  return std::max(minimum, std::min(value, maximum));
}

double ClampDouble(double value, double minimum, double maximum) {
  return std::max(minimum, std::min(value, maximum));
}

}  // namespace

DisplayMetrics ResolveDisplayMetrics(int framebuffer_width, int framebuffer_height,
                                      double user_density) {
  DisplayMetrics metrics;
  metrics.framebuffer_width = std::max(320, framebuffer_width);
  metrics.framebuffer_height = std::max(240, framebuffer_height);
  if (metrics.framebuffer_height < 720) {
    metrics.logical_width = metrics.framebuffer_width;
    metrics.logical_height = metrics.framebuffer_height;
    metrics.window_scale = 1.0;
  } else {
    metrics.logical_height = 720;
    metrics.window_scale = static_cast<double>(metrics.framebuffer_height) /
                           static_cast<double>(metrics.logical_height);
    metrics.logical_width = std::max(320, static_cast<int>(
        std::lround(metrics.framebuffer_width / metrics.window_scale)));
  }
  metrics.density = user_density > 0.0 ? ClampDouble(user_density, 0.75, 1.35) : 1.0;
  return metrics;
}

UiLayout ResolveUiLayout(int viewport_width, int viewport_height, Insets safe_area) {
  UiLayout result;
  result.viewport_width = std::max(320, viewport_width);
  result.viewport_height = std::max(240, viewport_height);
  result.safe_area = safe_area;
  result.display.framebuffer_width = result.viewport_width;
  result.display.framebuffer_height = result.viewport_height;
  result.display.logical_width = result.viewport_width;
  result.display.logical_height = result.viewport_height;
  result.display.window_scale = 1.0;
  result.display.density = 1.0;

  const int usable_width = result.viewport_width - safe_area.left - safe_area.right;
  const int usable_height = result.viewport_height - safe_area.top - safe_area.bottom;
  const double aspect = static_cast<double>(usable_width) / std::max(1, usable_height);

  if (aspect < 0.90) {
    result.mode = LayoutMode::Tall;
  } else if (aspect < 1.18) {
    result.mode = LayoutMode::Square;
  } else if (usable_width < 900 || usable_height <= 520) {
    result.mode = LayoutMode::Compact;
  } else if (usable_width >= 1450 && aspect >= 1.45) {
    result.mode = LayoutMode::Wide;
  } else {
    result.mode = LayoutMode::Standard;
  }

  const bool compact_height = usable_height <= 520;
  const bool roomy = usable_height >= 900;
  const double d = result.display.density;
  result.expanded_sidebar = false;
  result.top_bar_height = static_cast<int>(std::lround(
      (compact_height ? 52 : (roomy ? 72 : 62)) * d));
  result.bottom_bar_height = static_cast<int>(std::lround(
      (compact_height ? 38 : (roomy ? 54 : 46)) * d));
  result.sidebar_item_height = static_cast<int>(std::lround(
      (compact_height ? 39 : (roomy ? 58 : 48)) * d));
  result.filter_bar_height = static_cast<int>(std::lround(
      (compact_height ? 54 : (roomy ? 72 : 64)) * d));
  result.section_header_height = static_cast<int>(std::lround(
      (compact_height ? 30 : (roomy ? 44 : 36)) * d));
  result.content_padding = ClampInt(static_cast<int>(std::lround(usable_width * 0.04 * d)),
                                    18, 84);
  result.grid_gap = static_cast<int>(std::lround(
      (compact_height ? 12 : (roomy ? 36 : 28)) * d));
  result.card_gap = result.grid_gap;
  result.card_footer_height = static_cast<int>(std::lround(
      (compact_height ? 24 : (roomy ? 34 : 28)) * d));

  result.sidebar_width = ClampInt(static_cast<int>(std::lround(usable_width * 0.18 * d)),
                                  188, 320);
  result.nav_width = result.sidebar_width;

  result.content_x = safe_area.left;
  result.content_width = usable_width;
  result.grid_x = result.content_x + result.content_padding;
  result.grid_y = safe_area.top + result.top_bar_height + result.filter_bar_height +
                  result.section_header_height;
  result.grid_width = std::max(1, result.content_width - result.content_padding * 2);
  result.grid_height = std::max(1, usable_height - result.top_bar_height -
      result.filter_bar_height - result.section_header_height - result.bottom_bar_height);

  int minimum_card_width = 135;
  if (result.mode == LayoutMode::Standard) minimum_card_width = 148;
  if (result.mode == LayoutMode::Wide) minimum_card_width = 178;
  if (result.mode == LayoutMode::Square) minimum_card_width = compact_height ? 116 : 134;
  if (result.mode == LayoutMode::Tall) minimum_card_width = 132;
  minimum_card_width = static_cast<int>(std::lround(minimum_card_width * d));

  const int max_columns =
      (result.mode == LayoutMode::Square || result.mode == LayoutMode::Tall) ? 4 : 6;
  result.grid_columns = ClampInt(
      (result.grid_width + result.grid_gap) / (minimum_card_width + result.grid_gap),
      2, max_columns);
  result.card_width = std::max(72,
      (result.grid_width - result.grid_gap * (result.grid_columns - 1)) /
          result.grid_columns);
  result.card_cover_height = static_cast<int>(std::lround(result.card_width * 1.5));
  result.card_height = result.card_cover_height + result.card_footer_height;
  result.grid_rows = std::max(1, (result.grid_height + result.grid_gap) /
                                   (result.card_height + result.grid_gap));
  result.detail_width = 0;
  result.detail_x = 0;
  return result;
}

UiLayout ResolveUiLayoutForDisplay(int framebuffer_width, int framebuffer_height,
                                   Insets safe_area, double user_density) {
  const DisplayMetrics metrics =
      ResolveDisplayMetrics(framebuffer_width, framebuffer_height, user_density);
  UiLayout layout = ResolveUiLayout(metrics.logical_width, metrics.logical_height, safe_area);
  layout.display = metrics;
  return layout;
}

const char *LayoutModeName(LayoutMode mode) {
  switch (mode) {
    case LayoutMode::Compact: return "compact";
    case LayoutMode::Standard: return "standard";
    case LayoutMode::Wide: return "wide";
    case LayoutMode::Square: return "square";
    case LayoutMode::Tall: return "tall";
  }
  return "unknown";
}

}  // namespace mpl

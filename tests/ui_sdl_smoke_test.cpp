#include "ui/sdl_renderer.h"
#include "ui/grid_scroll_tracker.h"

#include <SDL.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <string>

using namespace mpl;

namespace {

std::uint32_t PixelAt(SDL_Surface *surface, int x, int y) {
  const auto *pixels = static_cast<const std::uint32_t *>(surface->pixels);
  return pixels[y * (surface->pitch / 4) + x];
}

bool IsMostlyRed(SDL_Surface *surface, int x, int y) {
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 b = 0;
  Uint8 a = 0;
  SDL_GetRGBA(PixelAt(surface, x, y), surface->format, &r, &g, &b, &a);
  return r > 180 && g < 80 && b < 80 && a > 200;
}

bool IsMostlyBlue(SDL_Surface *surface, int x, int y) {
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 b = 0;
  Uint8 a = 0;
  SDL_GetRGBA(PixelAt(surface, x, y), surface->format, &r, &g, &b, &a);
  return b > 170 && r < 90 && g < 120 && a > 200;
}

bool IsMostlyGreen(SDL_Surface *surface, int x, int y) {
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 b = 0;
  Uint8 a = 0;
  SDL_GetRGBA(PixelAt(surface, x, y), surface->format, &r, &g, &b, &a);
  return g > 170 && r < 120 && b < 150 && a > 200;
}

bool IsMostlyYellow(SDL_Surface *surface, int x, int y) {
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 b = 0;
  Uint8 a = 0;
  SDL_GetRGBA(PixelAt(surface, x, y), surface->format, &r, &g, &b, &a);
  return r > 180 && g > 150 && b < 120 && a > 200;
}

int CountMostlyWhite(SDL_Surface *surface, const SDL_Rect &rect) {
  int count = 0;
  for (int y = rect.y; y < rect.y + rect.h; ++y) {
    for (int x = rect.x; x < rect.x + rect.w; ++x) {
      Uint8 r = 0;
      Uint8 g = 0;
      Uint8 b = 0;
      Uint8 a = 0;
      SDL_GetRGBA(PixelAt(surface, x, y), surface->format, &r, &g, &b, &a);
      if (r > 230 && g > 230 && b > 230 && a > 230) ++count;
    }
  }
  return count;
}

bool RenderUntilMediaReady(SDL_Renderer *renderer, SDL_Surface *surface,
                           UiSession &session, const UiLayout &layout) {
  for (int attempt = 0; attempt < 30; ++attempt) {
    if (RenderLibraryView(renderer, session, layout) &&
        IsMostlyRed(surface, 300, 80) &&
        IsMostlyBlue(surface, 120, 85)) {
      return true;
    }
    SDL_Delay(10);
  }
  return false;
}

bool RenderUntilFullscreenCover(SDL_Renderer *renderer, SDL_Surface *surface,
                                UiSession &session, const UiLayout &layout) {
  session.preferences.fullscreen_grid = true;
  for (int attempt = 0; attempt < 45; ++attempt) {
    if (RenderLibraryView(renderer, session, layout) &&
        IsMostlyRed(surface, 30, 80)) {
      return true;
    }
    SDL_Delay(10);
  }
  return false;
}

void VerifyFullscreenLayout(const Library &library, int width, int height) {
  ClearRendererMediaCache();
  UiSession session = CreateUiSession(library, UiState{"gba", "gba-metroid", 0, "grid"});
  const UiLayout layout = ResolveUiLayout(width, height);
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
      0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
  assert(surface);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
  assert(renderer);
  assert(RenderLibraryView(renderer, session, layout));
  const std::uint32_t initial_top_bar = PixelAt(surface, 2, 2);
  assert(RenderUntilFullscreenCover(renderer, surface, session, layout));
  assert(PixelAt(surface, 2, 2) != initial_top_bar);
  ClearRendererMediaCache();
  SDL_DestroyRenderer(renderer);
  SDL_FreeSurface(surface);
}

void VerifySearchLayout(const Library &library, int width, int height) {
  ClearRendererMediaCache();
  UiSession session = CreateUiSession(library, UiState{"all", "gba-metroid", 0, "grid"});
  session.search_active = true;
  assert(AppendSearchText(&session, "Met"));
  const UiLayout layout = ResolveUiLayout(width, height);
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
      0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
  assert(surface);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
  assert(renderer);
  assert(RenderLibraryView(renderer, session, layout));
  const int logical_width = width <= 660 ? 640 : 720;
  const int logical_height = height >= 700 ? 720 : 480;
  const int dialog_width = std::min(560, std::max(420, logical_width - 32));
  const int key_height = logical_height >= 700 ? 42 : 32;
  const int input_height = logical_height >= 700 ? 44 : 38;
  const int ok_height = logical_height >= 700 ? 38 : 32;
  const int footer_height = logical_height >= 700 ? 30 : 26;
  const int dialog_height = 12 + input_height + 8 + key_height * 4 + 12 + 8 +
                            ok_height + footer_height + 12;
  const int keyboard_x = (logical_width - dialog_width) / 2 + 12;
  const int keyboard_y = (logical_height - dialog_height) / 2 + 12 +
                         input_height + 8;
  const int key_width = (dialog_width - 24 - 4 * 9) / 10;
  assert(PixelAt(surface, keyboard_x, keyboard_y) !=
         PixelAt(surface, keyboard_x + key_width + 4, keyboard_y));
  ClearRendererMediaCache();
  SDL_DestroyRenderer(renderer);
  SDL_FreeSurface(surface);
}

}  // namespace

int main() {
  GridScrollTracker scroll_tracker;
  assert(scroll_tracker.Resolve("pegasus", 0, 0, 2) == 0);
  assert(scroll_tracker.Resolve("pegasus", 0, 1, 2) == 0);
  assert(scroll_tracker.Resolve("pegasus", 0, 2, 2) == 0);
  assert(scroll_tracker.Resolve("pegasus", 0, 3, 2) == 1);
  assert(scroll_tracker.Resolve("pegasus", 0, 2, 2) == 1);
  assert(scroll_tracker.Resolve("pegasus", 0, 1, 2) == 1);
  assert(scroll_tracker.Resolve("pegasus", 0, 0, 2) == 0);

  Library library;
  Platform platform;
  platform.id = "gba";
  platform.display_name = "GBA";
  platform.launchable = true;
  library.platforms.push_back(platform);

  Game game;
  game.id = "gba-metroid";
  game.platform_id = "gba";
  game.title = "Metroid";
  game.sort_key = "Metroid";
  game.favorite = true;
  const std::string cover_path = "/tmp/mpl-ui-cover-smoke.bmp";
  const std::string logo_path = "/tmp/mpl-ui-logo-smoke.bmp";
  game.media.cover = cover_path;
  game.media.logo = logo_path;
  game.media.video = "/tmp/mpl-ui-missing-preview.mp4";
  library.games.push_back(game);

  UiSession session = CreateUiSession(library, UiState{"gba", "gba-metroid", 0, "grid"});
  const UiLayout layout = ResolveUiLayout(720, 480);

  assert(SDL_Init(0) == 0);
  SDL_Surface *cover = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32,
                                                     SDL_PIXELFORMAT_RGBA32);
  assert(cover);
  SDL_FillRect(cover, nullptr, SDL_MapRGBA(cover->format, 220, 20, 20, 255));
  assert(SDL_SaveBMP(cover, cover_path.c_str()) == 0);
  SDL_FreeSurface(cover);
  SDL_Surface *logo = SDL_CreateRGBSurfaceWithFormat(0, 32, 16, 32,
                                                    SDL_PIXELFORMAT_RGBA32);
  assert(logo);
  SDL_FillRect(logo, nullptr, SDL_MapRGBA(logo->format, 20, 80, 220, 255));
  assert(SDL_SaveBMP(logo, logo_path.c_str()) == 0);
  SDL_FreeSurface(logo);

  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 720, 480, 32, SDL_PIXELFORMAT_RGBA32);
  assert(surface);
  SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
  assert(renderer);
  assert(RenderLibraryView(renderer, session, layout));

  const std::uint32_t top_bar = PixelAt(surface, 2, 2);
  const std::uint32_t left_panel = PixelAt(surface, 20, 140);
  const std::uint32_t divider = PixelAt(surface, 239, 120);
  const std::uint32_t card = PixelAt(surface, 260, 70);
  assert(left_panel != top_bar);
  assert(divider != left_panel);
  assert(card != top_bar);
  assert(card != divider);
  assert(RenderUntilMediaReady(renderer, surface, session, layout));
  assert(IsMostlyRed(surface, 120, 200));

  session.system_status.battery_percent = 100;
  session.system_status.charging = false;
  assert(RenderLibraryView(renderer, session, layout));
  assert(IsMostlyGreen(surface, 695, 8));
  session.system_status.battery_percent = 15;
  assert(RenderLibraryView(renderer, session, layout));
  assert(IsMostlyYellow(surface, 695, 8));
  session.system_status.battery_percent = 10;
  assert(RenderLibraryView(renderer, session, layout));
  assert(IsMostlyRed(surface, 695, 8));
  session.system_status.battery_percent = -1;
  assert(RenderLibraryView(renderer, session, layout));
  assert(!IsMostlyGreen(surface, 695, 8));
  assert(!IsMostlyYellow(surface, 695, 8));
  assert(!IsMostlyRed(surface, 695, 8));

  session.system_status.battery_percent = 100;
  const SDL_Rect battery_cell{695, 7, 14, 6};
  const int idle_white_pixels = CountMostlyWhite(surface, battery_cell);
  session.system_status.charging = true;
  assert(RenderLibraryView(renderer, session, layout));
  assert(CountMostlyWhite(surface, battery_cell) > idle_white_pixels);

  assert(RenderUntilFullscreenCover(renderer, surface, session, layout));

  session.notice = UiNotice{true, "启动失败", "ROM文件不存在或无法读取。"};
  assert(RenderLibraryView(renderer, session, layout));
  const std::uint32_t notice_pixel = PixelAt(surface, 360, 170);
  assert(notice_pixel != top_bar);

  ClearRendererMediaCache();
  SDL_DestroyRenderer(renderer);
  SDL_FreeSurface(surface);
  VerifyFullscreenLayout(library, 640, 480);
  VerifyFullscreenLayout(library, 720, 720);
  VerifySearchLayout(library, 720, 480);
  VerifySearchLayout(library, 640, 480);
  VerifySearchLayout(library, 720, 720);
  std::remove(cover_path.c_str());
  std::remove(logo_path.c_str());
  SDL_Quit();
  return 0;
}

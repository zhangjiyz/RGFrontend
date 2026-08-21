#include "ui/sdl_renderer.h"

#include <SDL.h>

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

}  // namespace

int main() {
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

  session.preferences.fullscreen_grid = true;
  assert(RenderLibraryView(renderer, session, layout));
  assert(IsMostlyRed(surface, 30, 80));

  session.notice = UiNotice{true, "启动失败", "ROM文件不存在或无法读取。"};
  assert(RenderLibraryView(renderer, session, layout));
  const std::uint32_t notice_pixel = PixelAt(surface, 360, 170);
  assert(notice_pixel != top_bar);

  ClearRendererMediaCache();
  SDL_DestroyRenderer(renderer);
  SDL_FreeSurface(surface);
  std::remove(cover_path.c_str());
  std::remove(logo_path.c_str());
  SDL_Quit();
  return 0;
}

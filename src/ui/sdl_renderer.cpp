#include "ui/sdl_renderer.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ui/media_playback.h"

namespace mpl {

namespace fs = std::filesystem;

namespace {

constexpr int kLogicalWidth = 720;
constexpr int kCompactLogicalWidth = 640;
constexpr int kLogicalHeight = 480;
constexpr int kSquareLogicalHeight = 720;
constexpr int kGridX = 240;
constexpr int kGridY = 45;
constexpr int kGridInset = 16;
constexpr int kFullscreenGridX = 0;
constexpr int kFullscreenGridInset = 18;
constexpr int kTabTextFontSize = 14;
constexpr int kTabHorizontalPadding = 18;
constexpr int kCoverTitleBaseFontSize = 12;
constexpr int kDescriptionBaseFontSize = 14;
constexpr int kPreviewVideoWidth = 214;
constexpr int kPreviewVideoHeight = 120;
constexpr std::size_t kMaxCoverTextures = 64;
constexpr std::size_t kMaxTextTextures = 256;
constexpr int kCoverDecodeMaxDimension = 256;
constexpr std::size_t kMaxPendingCoverRequests = 64;
constexpr int kPrewarmCoverRadius = 12;
constexpr double kDefaultPegasusCoverAspect = 1.42;
constexpr Uint32 kCoverScaleDurationMs = 140;
constexpr Uint32 kTabTransitionDurationMs = 240;

constexpr SDL_Color kBackground{3, 4, 5, 255};
constexpr SDL_Color kPanel{8, 9, 10, 255};
constexpr SDL_Color kInk{242, 243, 245, 255};
constexpr SDL_Color kMuted{177, 181, 186, 255};
struct ThemeColors {
  SDL_Color bar;
  SDL_Color selected;
  SDL_Color accent;
};

// Theme indices are persisted in ui.state, so append new colors instead of reordering.
constexpr ThemeColors kThemes[] = {
    {{28, 48, 62, 255}, {55, 84, 102, 255}, {142, 178, 201, 255}},
    {{61, 38, 50, 255}, {94, 59, 76, 255}, {241, 132, 176, 255}},
    {{57, 60, 64, 255}, {88, 92, 97, 255}, {220, 225, 230, 255}},
    {{17, 18, 20, 255}, {47, 50, 54, 255}, {190, 195, 202, 255}},
    {{50, 44, 92, 255}, {78, 69, 132, 255}, {180, 164, 255, 255}},
    {{15, 70, 41, 255}, {10, 104, 60, 255}, {118, 232, 164, 255}},
    {{91, 21, 28, 255}, {133, 35, 43, 255}, {255, 122, 128, 255}},
    {{46, 61, 70, 255}, {72, 91, 103, 255}, {128, 214, 238, 255}},
    {{61, 62, 58, 255}, {91, 92, 86, 255}, {217, 202, 178, 255}},
    {{224, 223, 213, 255}, {178, 181, 177, 255}, {250, 249, 239, 255}},
    {{18, 20, 24, 255}, {54, 59, 66, 255}, {166, 172, 181, 255}},
    {{196, 204, 203, 255}, {141, 162, 166, 255}, {238, 244, 242, 255}},
    {{66, 42, 92, 255}, {105, 75, 142, 255}, {206, 166, 255, 255}},
    {{28, 64, 103, 255}, {48, 103, 154, 255}, {116, 200, 255, 255}},
    {{172, 174, 174, 255}, {114, 118, 122, 255}, {238, 238, 230, 255}},
    {{181, 166, 138, 255}, {137, 119, 91, 255}, {255, 232, 188, 255}},
    {{107, 49, 22, 255}, {165, 73, 30, 255}, {255, 146, 70, 255}},
    {{30, 68, 118, 255}, {47, 100, 170, 255}, {104, 179, 255, 255}},
    {{127, 98, 12, 255}, {188, 149, 24, 255}, {255, 221, 84, 255}},
    {{40, 68, 104, 255}, {67, 103, 148, 255}, {130, 190, 245, 255}},
    {{22, 86, 86, 255}, {42, 134, 132, 255}, {106, 228, 217, 255}},
    {{34, 38, 42, 255}, {66, 71, 77, 255}, {176, 181, 186, 255}},
    {{188, 184, 174, 255}, {137, 132, 122, 255}, {245, 238, 221, 255}},
    {{73, 96, 126, 255}, {108, 136, 170, 255}, {178, 218, 255, 255}},
    {{126, 75, 100, 255}, {178, 108, 139, 255}, {255, 188, 216, 255}},
};
static_assert(static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0])) == kUiThemeColorCount,
              "theme table must match ui model theme count");

std::string g_font_path;
std::unordered_map<int, TTF_Font *> g_fonts;

struct CachedTexture {
  SDL_Texture *texture = nullptr;
  int width = 0;
  int height = 0;
};

struct TextureCache {
  SDL_Renderer *renderer = nullptr;
  std::unordered_map<std::string, CachedTexture> textures;
  std::deque<std::string> order;
};

struct AsyncImageLoader {
  std::mutex mutex;
  std::condition_variable cv;
  std::thread worker;
  bool stop = false;
  bool started = false;
  std::deque<std::string> current_requests;
  std::deque<std::string> prewarm_requests;
  std::unordered_set<std::string> queued;
  std::unordered_set<std::string> failed;
  std::unordered_map<std::string, SDL_Surface *> ready;
};

TextureCache g_cover_cache;
TextureCache g_text_cache;
AsyncImageLoader g_cover_loader;
VideoPreviewDecoder g_video_preview;
AudioPreviewPlayer g_audio_preview;
std::string g_app_dir = ".";
std::string g_state_dir;
std::string g_pending_video_path;
Uint32 g_pending_video_since = 0;
std::string g_active_video_path;
SDL_Texture *g_video_texture = nullptr;
SDL_Renderer *g_video_renderer = nullptr;
std::uint64_t g_video_texture_version = 0;
std::string g_video_texture_path;
int g_seen_bgm_track_revision = 0;
std::vector<std::string> g_music_tracks;
std::size_t g_music_track_index = 0;
std::string g_selected_animation_id;
Uint32 g_selected_animation_started_at = 0;
std::string g_cover_request_view_key;
std::string g_pegasus_aspect_view_key;
double g_pegasus_aspect = kDefaultPegasusCoverAspect;
bool g_pegasus_aspect_locked = false;
int g_render_canvas_width = kLogicalWidth;
int g_render_canvas_height = kLogicalHeight;

struct TabTransitionState {
  int active_index = -1;
  int from_index = -1;
  int direction = 0;
  int nav_count = 0;
  Uint32 started_at = 0;
};

TabTransitionState g_tab_transition;

void SetColor(SDL_Renderer *renderer, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void Fill(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color) {
  SetColor(renderer, color);
  SDL_RenderFillRect(renderer, &rect);
}

void FillBlend(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color) {
  SDL_BlendMode previous = SDL_BLENDMODE_NONE;
  SDL_GetRenderDrawBlendMode(renderer, &previous);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  Fill(renderer, rect, color);
  SDL_SetRenderDrawBlendMode(renderer, previous);
}

void Stroke(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color) {
  SetColor(renderer, color);
  SDL_RenderDrawRect(renderer, &rect);
}

SDL_Color WithOpacity(SDL_Color color, float opacity) {
  const float clamped = std::max(0.0f, std::min(1.0f, opacity));
  color.a = static_cast<Uint8>(std::lround(color.a * clamped));
  return color;
}

void FillRightSlantTab(SDL_Renderer *renderer, int x, int y, int width, int height,
                       int skew, SDL_Color color) {
  SetColor(renderer, color);
  for (int row = 0; row < height; ++row) {
    const double t = height <= 1 ? 0.0 : row / static_cast<double>(height - 1);
    const int right_offset = static_cast<int>(std::lround(skew - 2.0 * skew * t));
    SDL_RenderDrawLine(renderer, x, y + row, x + width + right_offset, y + row);
  }
}

void ClearTextureCache(TextureCache *cache) {
  if (!cache) return;
  for (auto &entry : cache->textures) {
    if (entry.second.texture) SDL_DestroyTexture(entry.second.texture);
  }
  cache->textures.clear();
  cache->order.clear();
}

SDL_Surface *ScaleSurfaceToMaxDimension(SDL_Surface *source, int max_dimension) {
  if (!source) return nullptr;
  const int source_width = source->w;
  const int source_height = source->h;
  if (source_width <= 0 || source_height <= 0) {
    SDL_FreeSurface(source);
    return nullptr;
  }

  const double scale = max_dimension > 0
                           ? std::min(1.0, max_dimension /
                                                static_cast<double>(
                                                    std::max(source_width, source_height)))
                           : 1.0;
  const int target_width = std::max(1, static_cast<int>(std::lround(source_width * scale)));
  const int target_height = std::max(1, static_cast<int>(std::lround(source_height * scale)));

  SDL_Surface *converted = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(source);
  if (!converted) return nullptr;

  if (target_width == converted->w && target_height == converted->h) return converted;

  SDL_Surface *scaled = SDL_CreateRGBSurfaceWithFormat(0, target_width, target_height,
                                                       32, SDL_PIXELFORMAT_RGBA32);
  if (!scaled) return converted;
  SDL_SetSurfaceBlendMode(converted, SDL_BLENDMODE_NONE);
  if (SDL_BlitScaled(converted, nullptr, scaled, nullptr) != 0) {
    SDL_FreeSurface(scaled);
    return converted;
  }
  SDL_FreeSurface(converted);
  return scaled;
}

SDL_Surface *LoadScaledImageSurface(const std::string &path) {
  SDL_Surface *surface = IMG_Load(path.c_str());
  return ScaleSurfaceToMaxDimension(surface, kCoverDecodeMaxDimension);
}

void CoverImageWorker() {
  while (true) {
    std::string path;
    {
      std::unique_lock<std::mutex> lock(g_cover_loader.mutex);
      g_cover_loader.cv.wait(lock, [] {
        return g_cover_loader.stop || !g_cover_loader.current_requests.empty() ||
               !g_cover_loader.prewarm_requests.empty();
      });
      if (g_cover_loader.stop && g_cover_loader.current_requests.empty() &&
          g_cover_loader.prewarm_requests.empty()) {
        return;
      }
      if (!g_cover_loader.current_requests.empty()) {
        path = std::move(g_cover_loader.current_requests.front());
        g_cover_loader.current_requests.pop_front();
      } else {
        path = std::move(g_cover_loader.prewarm_requests.front());
        g_cover_loader.prewarm_requests.pop_front();
      }
    }

    SDL_Surface *surface = LoadScaledImageSurface(path);

    {
      std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
      g_cover_loader.queued.erase(path);
      if (surface) {
        auto existing = g_cover_loader.ready.find(path);
        if (existing != g_cover_loader.ready.end() && existing->second) {
          SDL_FreeSurface(existing->second);
        }
        g_cover_loader.ready[path] = surface;
      } else {
        g_cover_loader.failed.insert(path);
      }
    }
  }
}

void EnsureCoverImageWorker() {
  std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
  if (g_cover_loader.started) return;
  g_cover_loader.stop = false;
  g_cover_loader.started = true;
  g_cover_loader.worker = std::thread(CoverImageWorker);
}

void StopCoverImageWorker() {
  {
    std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
    if (!g_cover_loader.started) {
      for (auto &entry : g_cover_loader.ready) {
        if (entry.second) SDL_FreeSurface(entry.second);
      }
      g_cover_loader.ready.clear();
      g_cover_loader.current_requests.clear();
      g_cover_loader.prewarm_requests.clear();
      g_cover_loader.queued.clear();
      g_cover_loader.failed.clear();
      return;
    }
    g_cover_loader.stop = true;
    for (const std::string &path : g_cover_loader.current_requests) {
      g_cover_loader.queued.erase(path);
    }
    for (const std::string &path : g_cover_loader.prewarm_requests) {
      g_cover_loader.queued.erase(path);
    }
    g_cover_loader.current_requests.clear();
    g_cover_loader.prewarm_requests.clear();
  }
  g_cover_loader.cv.notify_all();
  if (g_cover_loader.worker.joinable()) g_cover_loader.worker.join();
  {
    std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
    g_cover_loader.started = false;
    g_cover_loader.stop = false;
    g_cover_loader.current_requests.clear();
    g_cover_loader.prewarm_requests.clear();
    g_cover_loader.queued.clear();
    g_cover_loader.failed.clear();
    for (auto &entry : g_cover_loader.ready) {
      if (entry.second) SDL_FreeSurface(entry.second);
    }
    g_cover_loader.ready.clear();
  }
}

void TouchTexture(TextureCache *cache, const std::string &path) {
  if (!cache) return;
  auto in_order = std::find(cache->order.begin(), cache->order.end(), path);
  if (in_order != cache->order.end()) {
    cache->order.erase(in_order);
    cache->order.push_back(path);
  }
}

void TrimCoverCache() {
  while (g_cover_cache.order.size() >= kMaxCoverTextures) {
    const std::string evicted = g_cover_cache.order.front();
    g_cover_cache.order.pop_front();
    auto found = g_cover_cache.textures.find(evicted);
    if (found == g_cover_cache.textures.end()) continue;
    if (found->second.texture) SDL_DestroyTexture(found->second.texture);
    g_cover_cache.textures.erase(found);
  }
}

void InsertCoverTexture(SDL_Renderer *renderer, const std::string &path,
                        SDL_Surface *surface) {
  if (!renderer || path.empty() || !surface) return;
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  const int width = surface->w;
  const int height = surface->h;
  if (!texture) return;
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

  auto existing = g_cover_cache.textures.find(path);
  if (existing != g_cover_cache.textures.end()) {
    if (existing->second.texture) SDL_DestroyTexture(existing->second.texture);
    existing->second = CachedTexture{texture, width, height};
    TouchTexture(&g_cover_cache, path);
    return;
  }

  TrimCoverCache();
  g_cover_cache.order.push_back(path);
  g_cover_cache.textures.emplace(path, CachedTexture{texture, width, height});
}

void DrainReadyCoverImages(SDL_Renderer *renderer) {
  if (!renderer) return;
  std::unordered_map<std::string, SDL_Surface *> ready;
  {
    std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
    ready.swap(g_cover_loader.ready);
  }
  for (auto &entry : ready) {
    InsertCoverTexture(renderer, entry.first, entry.second);
    if (entry.second) SDL_FreeSurface(entry.second);
  }
}

void ClearPendingCoverRequests() {
  std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
  for (const std::string &path : g_cover_loader.current_requests) {
    g_cover_loader.queued.erase(path);
  }
  for (const std::string &path : g_cover_loader.prewarm_requests) {
    g_cover_loader.queued.erase(path);
  }
  g_cover_loader.current_requests.clear();
  g_cover_loader.prewarm_requests.clear();
}

void RequestCoverImageDecode(const std::string &path, bool urgent) {
  if (path.empty()) return;
  EnsureCoverImageWorker();
  {
    std::lock_guard<std::mutex> lock(g_cover_loader.mutex);
    if (g_cover_loader.failed.find(path) != g_cover_loader.failed.end()) return;
    if (g_cover_loader.queued.find(path) != g_cover_loader.queued.end()) {
      if (urgent) {
        auto current = std::find(g_cover_loader.current_requests.begin(),
                                 g_cover_loader.current_requests.end(), path);
        if (current != g_cover_loader.current_requests.end()) return;
        auto prewarm = std::find(g_cover_loader.prewarm_requests.begin(),
                                 g_cover_loader.prewarm_requests.end(), path);
        if (prewarm != g_cover_loader.prewarm_requests.end()) {
          g_cover_loader.prewarm_requests.erase(prewarm);
          g_cover_loader.current_requests.push_back(path);
        }
      }
      return;
    }
    if (g_cover_loader.ready.find(path) != g_cover_loader.ready.end()) return;
    if (urgent) g_cover_loader.current_requests.push_back(path);
    else g_cover_loader.prewarm_requests.push_back(path);
    g_cover_loader.queued.insert(path);
    while (g_cover_loader.prewarm_requests.size() > kMaxPendingCoverRequests) {
      const std::string dropped = g_cover_loader.prewarm_requests.back();
      g_cover_loader.prewarm_requests.pop_back();
      g_cover_loader.queued.erase(dropped);
    }
  }
  g_cover_loader.cv.notify_one();
}

void ClearVideoTexture() {
  if (g_video_texture) SDL_DestroyTexture(g_video_texture);
  g_video_texture = nullptr;
  g_video_renderer = nullptr;
  g_video_texture_version = 0;
  g_video_texture_path.clear();
}

SDL_Rect FitInside(const SDL_Rect &bounds, int source_width, int source_height) {
  if (source_width <= 0 || source_height <= 0 || bounds.w <= 0 || bounds.h <= 0) {
    return bounds;
  }
  const double scale = std::min(bounds.w / static_cast<double>(source_width),
                                bounds.h / static_cast<double>(source_height));
  const int width = std::max(1, static_cast<int>(std::lround(source_width * scale)));
  const int height = std::max(1, static_cast<int>(std::lround(source_height * scale)));
  return SDL_Rect{bounds.x + (bounds.w - width) / 2,
                  bounds.y + (bounds.h - height) / 2, width, height};
}

SDL_Rect ClampRectInside(SDL_Rect rect, const SDL_Rect &bounds) {
  if (rect.w > bounds.w) {
    rect.w = bounds.w;
  }
  if (rect.h > bounds.h) {
    rect.h = bounds.h;
  }
  rect.x = std::max(bounds.x, std::min(rect.x, bounds.x + bounds.w - rect.w));
  rect.y = std::max(bounds.y, std::min(rect.y, bounds.y + bounds.h - rect.h));
  return rect;
}

bool EnsureImageRuntime() {
  static bool attempted = false;
  static bool available = false;
  if (!attempted) {
    attempted = true;
    const int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    available = (IMG_Init(flags) & (IMG_INIT_PNG | IMG_INIT_JPG)) != 0;
  }
  return available;
}

CachedTexture *LoadCoverTexture(SDL_Renderer *renderer, const std::string &path,
                                bool urgent = true) {
  if (!renderer || path.empty() || !EnsureImageRuntime()) return nullptr;
  if (g_cover_cache.renderer != renderer) {
    ClearTextureCache(&g_cover_cache);
    g_cover_cache.renderer = renderer;
  }
  DrainReadyCoverImages(renderer);
  auto existing = g_cover_cache.textures.find(path);
  if (existing != g_cover_cache.textures.end()) {
    TouchTexture(&g_cover_cache, path);
    return &existing->second;
  }

  RequestCoverImageDecode(path, urgent);
  return nullptr;
}

CachedTexture *FindCoverTexture(SDL_Renderer *renderer, const std::string &path) {
  if (!renderer || path.empty()) return nullptr;
  if (g_cover_cache.renderer != renderer) return nullptr;
  DrainReadyCoverImages(renderer);
  auto existing = g_cover_cache.textures.find(path);
  if (existing == g_cover_cache.textures.end()) return nullptr;
  return &existing->second;
}

CachedTexture *LoadTextTexture(SDL_Renderer *renderer, const std::string &key,
                               TTF_Font *font, const std::string &text,
                               SDL_Color color) {
  if (!renderer || !font || text.empty()) return nullptr;
  if (g_text_cache.renderer != renderer) {
    ClearTextureCache(&g_text_cache);
    g_text_cache.renderer = renderer;
  }
  auto existing = g_text_cache.textures.find(key);
  if (existing != g_text_cache.textures.end()) {
    auto in_order = std::find(g_text_cache.order.begin(), g_text_cache.order.end(), key);
    if (in_order != g_text_cache.order.end()) {
      g_text_cache.order.erase(in_order);
      g_text_cache.order.push_back(key);
    }
    return &existing->second;
  }
  SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
  if (!surface) return nullptr;
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  const int width = surface->w;
  const int height = surface->h;
  SDL_FreeSurface(surface);
  if (!texture) return nullptr;
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

  while (g_text_cache.order.size() >= kMaxTextTextures) {
    const std::string evicted = g_text_cache.order.front();
    g_text_cache.order.pop_front();
    auto found = g_text_cache.textures.find(evicted);
    if (found == g_text_cache.textures.end()) continue;
    if (found->second.texture) SDL_DestroyTexture(found->second.texture);
    g_text_cache.textures.erase(found);
  }
  g_text_cache.order.push_back(key);
  auto inserted = g_text_cache.textures.emplace(key, CachedTexture{texture, width, height});
  return &inserted.first->second;
}

bool DrawMediaImage(SDL_Renderer *renderer, const std::string &path, const SDL_Rect &bounds) {
  CachedTexture *cover = LoadCoverTexture(renderer, path);
  if (!cover || !cover->texture) return false;
  const SDL_Rect destination = FitInside(bounds, cover->width, cover->height);
  SDL_RenderCopy(renderer, cover->texture, nullptr, &destination);
  return true;
}

bool DrawCoverImage(SDL_Renderer *renderer, const Game &game, const SDL_Rect &bounds) {
  return DrawMediaImage(renderer, game.media.cover, bounds);
}

void PrewarmNearbyCoverImages(SDL_Renderer *renderer, const UiSession &session) {
  if (!renderer || session.visible_game_indices.empty()) return;
  const int selected = session.selected_visible_index;
  if (selected < 0 ||
      selected >= static_cast<int>(session.visible_game_indices.size())) {
    return;
  }

  for (int distance = 1; distance <= kPrewarmCoverRadius; ++distance) {
    const int next = selected + distance;
    const int previous = selected - distance;
    if (next < static_cast<int>(session.visible_game_indices.size())) {
      const Game &game = session.library.games[session.visible_game_indices[next]];
      LoadCoverTexture(renderer, game.media.cover, false);
    }
    if (previous >= 0) {
      const Game &game = session.library.games[session.visible_game_indices[previous]];
      LoadCoverTexture(renderer, game.media.cover, false);
    }
  }
}

std::string MediaViewKey(const UiSession &session, int start) {
  std::ostringstream key;
  key << session.active_nav_index << ':' << start << ':'
      << session.visible_game_indices.size();
  if (!session.navigation.empty() &&
      session.active_nav_index >= 0 &&
      session.active_nav_index < static_cast<int>(session.navigation.size())) {
    const UiNavItem &nav = session.navigation[session.active_nav_index];
    key << ':' << static_cast<int>(nav.kind) << ':' << nav.id << ':' << nav.title;
  }
  if (!session.visible_game_indices.empty()) {
    const std::size_t first_index = session.visible_game_indices.front();
    if (first_index < session.library.games.size()) {
      key << ':' << session.library.games[first_index].id;
    }
  }
  return key.str();
}

void QueueVisibleCoverImages(SDL_Renderer *renderer, const UiSession &session,
                             int start, int slots) {
  if (!renderer || slots <= 0) return;
  const std::string view_key = MediaViewKey(session, start);
  if (view_key != g_cover_request_view_key) {
    g_cover_request_view_key = view_key;
    ClearPendingCoverRequests();
  }
  for (int slot = 0; slot < slots; ++slot) {
    const int visible_index = start + slot;
    if (visible_index >= static_cast<int>(session.visible_game_indices.size())) break;
    const Game &game = session.library.games[session.visible_game_indices[visible_index]];
    LoadCoverTexture(renderer, game.media.cover, true);
  }
}

std::string PaddedNumber(int value, int width) {
  std::ostringstream stream;
  stream << std::setw(width) << std::setfill('0') << std::max(0, value);
  return stream.str();
}

int DigitCount(int value) {
  int digits = 1;
  for (value = std::max(0, value); value >= 10; value /= 10) ++digits;
  return digits;
}

std::string LowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool IsMusicFile(const fs::path &path) {
  const std::string extension = LowerAscii(path.extension().u8string());
  return extension == ".mp3" || extension == ".ogg" || extension == ".wav" ||
         extension == ".flac" || extension == ".m4a" || extension == ".aac";
}

std::vector<std::string> CollectMusicTracks() {
  std::vector<fs::path> directories;
  if (!g_app_dir.empty()) {
    directories.push_back(fs::u8path(g_app_dir) / "assets/music");
    directories.push_back(fs::u8path(g_app_dir) / "assets/music/builtin");
    directories.push_back(fs::u8path(g_app_dir) / "../assets/music/builtin");
  }
  if (!g_state_dir.empty()) directories.push_back(fs::u8path(g_state_dir) / "music");

  std::vector<std::string> tracks;
  for (const fs::path &directory : directories) {
    std::error_code error;
    for (fs::directory_iterator iterator(directory, error), end;
         !error && iterator != end; iterator.increment(error)) {
      if (!iterator->is_regular_file(error) || !IsMusicFile(iterator->path())) continue;
      tracks.push_back(iterator->path().u8string());
    }
  }
  std::sort(tracks.begin(), tracks.end());
  tracks.erase(std::unique(tracks.begin(), tracks.end()), tracks.end());
  return tracks;
}

std::string SelectedMusicTrack(const UiSession &session) {
  if (g_music_tracks.empty()) g_music_tracks = CollectMusicTracks();
  if (g_music_tracks.empty()) return {};
  if (session.bgm_track_revision != g_seen_bgm_track_revision) {
    g_seen_bgm_track_revision = session.bgm_track_revision;
    g_music_track_index = (g_music_track_index + 1) % g_music_tracks.size();
  }
  g_music_track_index %= g_music_tracks.size();
  return g_music_tracks[g_music_track_index];
}

bool PreviewGameAudioEnabled() {
  const char *value = std::getenv("MPL_DISABLE_PREVIEW_GAME_AUDIO");
  if (!value || value[0] == '\0') return true;
  std::string flag(value);
  std::transform(flag.begin(), flag.end(), flag.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return !(flag == "1" || flag == "true" || flag == "yes" || flag == "on");
}

void SyncVideoPreview(const UiSession &session, const Game *game) {
  const std::string desired_path =
      game && session.preferences.preview_video_enabled ? game->media.video : std::string{};
  if (desired_path.empty()) {
    g_video_preview.Stop();
    ClearVideoTexture();
    g_pending_video_path.clear();
    g_pending_video_since = 0;
    g_active_video_path.clear();
    return;
  }

  const Uint32 now = SDL_GetTicks();
  if (desired_path != g_pending_video_path) {
    g_video_preview.Stop();
    ClearVideoTexture();
    g_pending_video_path = desired_path;
    g_pending_video_since = now;
    g_active_video_path.clear();
    return;
  }
  if (!SDL_TICKS_PASSED(now, g_pending_video_since + 300)) return;
  g_video_preview.Start(desired_path, kPreviewVideoWidth, kPreviewVideoHeight,
                        session.preferences.preview_video_loop);
  g_active_video_path = desired_path;
}

void SyncAudioPreview(const UiSession &session, const Game *game) {
  std::string desired_path;
  bool loop = false;
  switch (session.preferences.bgm_mode) {
    case UiBgmMode::Music:
      desired_path = SelectedMusicTrack(session);
      loop = true;
      break;
    case UiBgmMode::GameAudio:
      if (PreviewGameAudioEnabled() && game && session.preferences.preview_video_enabled &&
          g_active_video_path == game->media.video) {
        desired_path = game->media.video;
        loop = session.preferences.preview_video_loop;
      }
      break;
    case UiBgmMode::Muted:
      break;
  }
  if (desired_path.empty()) {
    g_audio_preview.Stop();
  } else {
    g_audio_preview.Start(desired_path, loop);
  }
}

SDL_Texture *VideoTexture(SDL_Renderer *renderer) {
  if (!renderer) return nullptr;
  if (g_active_video_path.empty()) return nullptr;
  std::vector<std::uint8_t> pixels;
  std::uint64_t version = g_video_texture_version;
  if (!g_video_preview.CopyLatestFrame(&pixels, &version)) {
    return g_video_texture_path == g_active_video_path ? g_video_texture : nullptr;
  }
  if (g_video_texture_path != g_active_video_path) {
    ClearVideoTexture();
  }
  if (g_video_renderer != renderer || !g_video_texture) {
    ClearVideoTexture();
    g_video_renderer = renderer;
    g_video_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        kPreviewVideoWidth, kPreviewVideoHeight);
  }
  if (!g_video_texture) return nullptr;
  SDL_UpdateTexture(g_video_texture, nullptr, pixels.data(), kPreviewVideoWidth * 4);
  g_video_texture_version = version;
  g_video_texture_path = g_active_video_path;
  return g_video_texture;
}

std::string DefaultFontPath() {
  const std::vector<std::string> candidates = {
      "/System/Library/Fonts/Hiragino Sans GB.ttc",
      "/System/Library/Fonts/STHeiti Medium.ttc",
      "/System/Library/Fonts/AppleSDGothicNeo.ttc",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/dejavu/DejaVuSans.ttf",
  };
  for (const std::string &candidate : candidates) {
    SDL_RWops *rw = SDL_RWFromFile(candidate.c_str(), "rb");
    if (!rw) continue;
    SDL_RWclose(rw);
    return candidate;
  }
  return {};
}

TTF_Font *Font(int point_size) {
  if (TTF_WasInit() == 0 && TTF_Init() != 0) return nullptr;
  auto found = g_fonts.find(point_size);
  if (found != g_fonts.end()) return found->second;
  if (g_font_path.empty()) g_font_path = DefaultFontPath();
  if (g_font_path.empty()) return nullptr;
  TTF_Font *font = TTF_OpenFont(g_font_path.c_str(), point_size);
  if (font) TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
  g_fonts[point_size] = font;
  return font;
}

std::string Ellipsize(const std::string &text, TTF_Font *font, int max_width) {
  if (!font || max_width <= 0) return text;
  int width = 0;
  if (TTF_SizeUTF8(font, text.c_str(), &width, nullptr) == 0 && width <= max_width) {
    return text;
  }
  const std::string suffix = "...";
  std::vector<std::size_t> boundaries{0};
  for (std::size_t index = 0; index < text.size();) {
    const unsigned char byte = static_cast<unsigned char>(text[index]);
    std::size_t length = 1;
    if ((byte & 0xE0) == 0xC0) length = 2;
    else if ((byte & 0xF0) == 0xE0) length = 3;
    else if ((byte & 0xF8) == 0xF0) length = 4;
    index = std::min(text.size(), index + length);
    boundaries.push_back(index);
  }
  for (std::size_t count = boundaries.size(); count > 1; --count) {
    const std::string candidate = text.substr(0, boundaries[count - 2]) + suffix;
    if (TTF_SizeUTF8(font, candidate.c_str(), &width, nullptr) == 0 &&
        width <= max_width) {
      return candidate;
    }
  }
  return suffix;
}

void DrawText(SDL_Renderer *renderer, const std::string &text, int x, int y,
              int point_size, SDL_Color color, int max_width = 0,
              bool center_align = false) {
  if (text.empty()) return;
  TTF_Font *font = Font(point_size);
  if (!font) {
    const int width = max_width > 0 ? std::min(max_width, static_cast<int>(text.size()) * 7)
                                    : static_cast<int>(text.size()) * 7;
    Fill(renderer, SDL_Rect{center_align ? x - width / 2 : x, y + point_size / 3,
                            std::max(8, width), std::max(2, point_size / 3)}, color);
    return;
  }
  const std::string display = max_width > 0 ? Ellipsize(text, font, max_width) : text;
  const std::string key = display + "#" + std::to_string(point_size) + "#" +
                          std::to_string(color.r) + "," + std::to_string(color.g) +
                          "," + std::to_string(color.b) + "," + std::to_string(color.a);
  CachedTexture *cached = LoadTextTexture(renderer, key, font, display, color);
  if (!cached || !cached->texture) return;
  const SDL_Rect destination{center_align ? x - cached->width / 2 : x, y,
                             cached->width, cached->height};
  SDL_RenderCopy(renderer, cached->texture, nullptr, &destination);
}

void DrawTextRight(SDL_Renderer *renderer, const std::string &text, int right_x, int y,
                   int size, SDL_Color color) {
  TTF_Font *font = Font(size);
  if (!font || text.empty()) return;
  int width = 0;
  if (TTF_SizeUTF8(font, text.c_str(), &width, nullptr) != 0) return;
  DrawText(renderer, text, right_x - width, y, size, color);
}

void DrawScrollingText(SDL_Renderer *renderer, const std::string &text,
                       const SDL_Rect &bounds, int point_size, SDL_Color color) {
  if (text.empty() || bounds.w <= 0 || bounds.h <= 0) return;
  TTF_Font *font = Font(point_size);
  if (!font) {
    DrawText(renderer, text, bounds.x + bounds.w / 2, bounds.y, point_size, color,
             bounds.w, true);
    return;
  }
  int width = 0;
  if (TTF_SizeUTF8(font, text.c_str(), &width, nullptr) != 0 || width <= bounds.w) {
    DrawText(renderer, text, bounds.x + bounds.w / 2, bounds.y, point_size, color,
             bounds.w, true);
    return;
  }

  const std::string key = "scroll:" + text + "#" + std::to_string(point_size) + "#" +
                          std::to_string(color.r) + "," + std::to_string(color.g) +
                          "," + std::to_string(color.b) + "," + std::to_string(color.a);
  CachedTexture *cached = LoadTextTexture(renderer, key, font, text, color);
  if (!cached || !cached->texture) return;

  constexpr int kGap = 48;
  constexpr int kPauseMs = 900;
  constexpr int kPixelsPerSecond = 34;
  const int scroll_distance = cached->width + kGap;
  const int scroll_ms = std::max(1, (scroll_distance * 1000) / kPixelsPerSecond);
  const int cycle_ms = kPauseMs * 2 + scroll_ms;
  const int tick = static_cast<int>(SDL_GetTicks() % cycle_ms);
  int offset = 0;
  if (tick > kPauseMs) {
    offset = tick < kPauseMs + scroll_ms
                 ? ((tick - kPauseMs) * kPixelsPerSecond) / 1000
                 : scroll_distance;
  }

  const bool previous_clip_enabled = SDL_RenderIsClipEnabled(renderer) == SDL_TRUE;
  SDL_Rect previous_clip{};
  SDL_RenderGetClipRect(renderer, &previous_clip);
  SDL_RenderSetClipRect(renderer, &bounds);
  const int y = bounds.y + std::max(0, (bounds.h - cached->height) / 2);
  SDL_Rect destination{bounds.x - offset, y, cached->width, cached->height};
  SDL_RenderCopy(renderer, cached->texture, nullptr, &destination);
  if (destination.x + destination.w + kGap < bounds.x + bounds.w) {
    destination.x += cached->width + kGap;
    SDL_RenderCopy(renderer, cached->texture, nullptr, &destination);
  }
  SDL_RenderSetClipRect(renderer, previous_clip_enabled ? &previous_clip : nullptr);
}

std::vector<std::string> Utf8Characters(const std::string &text) {
  std::vector<std::string> chars;
  for (std::size_t index = 0; index < text.size();) {
    const unsigned char byte = static_cast<unsigned char>(text[index]);
    std::size_t length = 1;
    if ((byte & 0xE0) == 0xC0) length = 2;
    else if ((byte & 0xF0) == 0xE0) length = 3;
    else if ((byte & 0xF8) == 0xF0) length = 4;
    length = std::min(length, text.size() - index);
    chars.push_back(text.substr(index, length));
    index += length;
  }
  return chars;
}

std::vector<std::string> WrappedTextLines(const std::string &text, int width, int size) {
  std::vector<std::string> lines;
  TTF_Font *font = Font(size);
  if (!font) return lines;
  std::string line;
  int line_width = 0;
  for (const std::string &character : Utf8Characters(text)) {
    if (character == "\r") continue;
    if (character == "\n") {
      lines.push_back(line);
      line.clear();
      line_width = 0;
      continue;
    }
    int character_width = 0;
    if (TTF_SizeUTF8(font, character.c_str(), &character_width, nullptr) != 0) {
      character_width = 0;
    }
    if (line_width + character_width > width && !line.empty()) {
      lines.push_back(line);
      line = character;
      line_width = character_width;
    } else {
      line += character;
      line_width += character_width;
    }
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}

void DrawWrappedText(SDL_Renderer *renderer, const std::string &text, int x, int y,
                     int width, int line_height, int max_lines, int size,
                     SDL_Color color, int start_line = 0) {
  const std::vector<std::string> lines = WrappedTextLines(text, width, size);
  const int start = std::max(0, std::min(start_line,
      std::max(0, static_cast<int>(lines.size()) - max_lines)));
  for (int index = 0; index < max_lines && start + index < static_cast<int>(lines.size());
       ++index) {
    DrawText(renderer, lines[start + index], x, y + index * line_height, size, color, width);
  }
}

const Game *SelectedGameForRender(const UiSession &session) {
  return SelectedGame(session);
}

int WrappedNavIndex(int index, int count) {
  if (count <= 0) return 0;
  return (index % count + count) % count;
}

int NavDirection(int from, int to, int count) {
  if (count <= 1 || from == to) return 0;
  if (WrappedNavIndex(from + 1, count) == to) return 1;
  if (WrappedNavIndex(from - 1, count) == to) return -1;
  const int forward = WrappedNavIndex(to - from, count);
  const int backward = WrappedNavIndex(from - to, count);
  return forward <= backward ? 1 : -1;
}

int TabWidth(const UiNavItem &item) {
  TTF_Font *font = Font(kTabTextFontSize);
  int text_width = 0;
  if (font && TTF_SizeUTF8(font, item.title.c_str(), &text_width, nullptr) == 0) {
    return std::max(42, text_width + kTabHorizontalPadding);
  }
  return std::max(42, static_cast<int>(item.title.size()) * 7 + kTabHorizontalPadding);
}

const UiNavItem &TabItemAt(const UiSession &session, int index) {
  const int count = static_cast<int>(session.navigation.size());
  return session.navigation[WrappedNavIndex(index, count)];
}

std::string PlatformLabel(const Game &game) {
  std::string label = game.platform_id;
  std::transform(label.begin(), label.end(), label.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  return label.empty() ? "GAME" : label;
}

const ThemeColors &ThemeByIndex(int theme_color) {
  constexpr int count = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
  const int index = (theme_color % count + count) % count;
  return kThemes[index];
}

const ThemeColors &ThemeForSession(const UiSession &session) {
  return ThemeByIndex(session.preferences.theme_color);
}

bool IsFourThreeCompactLayout(const UiLayout &layout) {
  return layout.mode == LayoutMode::Compact && layout.viewport_width <= 660 &&
         layout.viewport_height >= 470;
}

int LogicalWidthForLayout(const UiLayout &layout) {
  return IsFourThreeCompactLayout(layout) ? kCompactLogicalWidth : kLogicalWidth;
}

int LogicalHeightForLayout(const UiLayout &layout) {
  if (layout.mode == LayoutMode::Square && layout.viewport_height >= 700) {
    return kSquareLogicalHeight;
  }
  return kLogicalHeight;
}

int CanvasWidth() {
  return g_render_canvas_width;
}

int CanvasHeight() {
  return g_render_canvas_height;
}

int CoverTitleFontSize(const UiSession &session) {
  switch (session.preferences.cover_title_size_level) {
    case 0:
      return 10;
    case 2:
      return 14;
    default:
      return kCoverTitleBaseFontSize;
  }
}

int DescriptionFontSize(const UiSession &session) {
  switch (session.preferences.description_size_level) {
    case 0:
      return 12;
    case 2:
      return 16;
    default:
      return kDescriptionBaseFontSize;
  }
}

int GridColumns(const UiSession &session) {
  int columns = 4;
  switch (session.preferences.grid_size) {
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
  return columns + (session.preferences.fullscreen_grid ? 1 : 0);
}

float SmoothStep(float value) {
  const float clamped = std::max(0.0f, std::min(1.0f, value));
  return clamped * clamped * (3.0f - 2.0f * clamped);
}

float SelectedScale(const Game &game) {
  const std::string key = game.id;
  const Uint32 now = SDL_GetTicks();
  if (key != g_selected_animation_id) {
    g_selected_animation_id = key;
    g_selected_animation_started_at = now;
  }
  const float progress = SmoothStep((now - g_selected_animation_started_at) /
                                    static_cast<float>(kCoverScaleDurationMs));
  return 1.0f + 0.15f * progress;
}

float ChromeHiddenProgress(const UiSession &session) {
  return session.preferences.fullscreen_grid ? 1.0f : 0.0f;
}

SDL_Rect GridBounds(const UiSession &session) {
  const float progress = ChromeHiddenProgress(session);
  const int x = static_cast<int>(std::lround(kGridX + (kFullscreenGridX - kGridX) * progress));
  const int compact_grid_width = std::max(1, CanvasWidth() - kGridX);
  const int fullscreen_grid_width = CanvasWidth();
  const int width = static_cast<int>(std::lround(
      compact_grid_width + (fullscreen_grid_width - compact_grid_width) * progress));
  return SDL_Rect{x, kGridY, width, std::max(1, CanvasHeight() - kGridY)};
}

int GridInset(const UiSession &session) {
  const float progress = ChromeHiddenProgress(session);
  return static_cast<int>(std::lround(kGridInset +
                                      (kFullscreenGridInset - kGridInset) * progress));
}

int GridCardSize(const UiSession &session) {
  const SDL_Rect bounds = GridBounds(session);
  const int inset = GridInset(session);
  return (bounds.w - inset * 2) / GridColumns(session);
}

int GridVisibleRows(const UiSession &session) {
  const SDL_Rect bounds = GridBounds(session);
  const int inset = GridInset(session);
  return std::max(1, (bounds.h - inset * 2) / GridCardSize(session));
}

bool IsPegasusGame(const Game &game) {
  return game.source == "pegasus" ||
         game.metadata_path.find("metadata.pegasus.txt") != std::string::npos;
}

bool UsePegasusGridLayout(const UiSession &session) {
  if (session.visible_game_indices.empty()) return false;
  int pegasus_count = 0;
  for (std::size_t index : session.visible_game_indices) {
    if (index >= session.library.games.size()) continue;
    if (IsPegasusGame(session.library.games[index])) ++pegasus_count;
  }
  if (pegasus_count <= 0) return false;
  if (!session.navigation.empty() &&
      session.navigation[session.active_nav_index].kind == UiNavKind::Collection) {
    return true;
  }
  const Game *selected = SelectedGameForRender(session);
  return selected && IsPegasusGame(*selected) && pegasus_count * 2 >=
      static_cast<int>(session.visible_game_indices.size());
}

struct PegasusAspectResult {
  double aspect = kDefaultPegasusCoverAspect;
  bool real = false;
};

PegasusAspectResult ComputePegasusCoverAspect(SDL_Renderer *renderer,
                                              const UiSession &session) {
  const auto aspect_for_game = [&](const Game *game) -> PegasusAspectResult {
    if (!game || !IsPegasusGame(*game)) return {};
    CachedTexture *texture = FindCoverTexture(renderer, game->media.cover);
    if (!texture || texture->width <= 0 || texture->height <= 0) return {};
    return {std::clamp(texture->height / static_cast<double>(texture->width), 0.55, 1.75),
            true};
  };

  PegasusAspectResult selected_aspect = aspect_for_game(SelectedGameForRender(session));
  if (selected_aspect.real) {
    return selected_aspect;
  }
  for (std::size_t index : session.visible_game_indices) {
    if (index >= session.library.games.size()) continue;
    PegasusAspectResult aspect = aspect_for_game(&session.library.games[index]);
    if (aspect.real) {
      return aspect;
    }
  }
  return {};
}

double PegasusCoverAspect(SDL_Renderer *renderer, const UiSession &session, int start) {
  const std::string view_key = MediaViewKey(session, start);
  if (view_key != g_pegasus_aspect_view_key) {
    g_pegasus_aspect_view_key = view_key;
    g_pegasus_aspect = kDefaultPegasusCoverAspect;
    g_pegasus_aspect_locked = false;
  }
  if (!g_pegasus_aspect_locked) {
    const PegasusAspectResult result = ComputePegasusCoverAspect(renderer, session);
    g_pegasus_aspect = result.aspect;
    g_pegasus_aspect_locked = result.real;
  }
  return g_pegasus_aspect;
}

void BeginLogicalCanvas(SDL_Renderer *renderer, const UiLayout &layout,
                        SDL_Rect *previous_viewport, float *previous_scale_x,
                        float *previous_scale_y) {
  SDL_RenderGetViewport(renderer, previous_viewport);
  SDL_RenderGetScale(renderer, previous_scale_x, previous_scale_y);
  const int logical_width = LogicalWidthForLayout(layout);
  const int logical_height = LogicalHeightForLayout(layout);
  const float scale = std::min(layout.viewport_width / static_cast<float>(logical_width),
                               layout.viewport_height / static_cast<float>(logical_height));
  const int width = static_cast<int>(std::lround(logical_width * scale));
  const int height = static_cast<int>(std::lround(logical_height * scale));
  const SDL_Rect viewport{(layout.viewport_width - width) / 2,
                          (layout.viewport_height - height) / 2, width, height};
  SDL_RenderSetViewport(renderer, &viewport);
  SDL_RenderSetScale(renderer, scale, scale);
}

void EndLogicalCanvas(SDL_Renderer *renderer, const SDL_Rect &previous_viewport,
                      float previous_scale_x, float previous_scale_y) {
  SDL_RenderSetScale(renderer, previous_scale_x, previous_scale_y);
  SDL_RenderSetViewport(renderer, &previous_viewport);
}

void RenderTopBar(SDL_Renderer *renderer, const UiSession &session) {
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  Fill(renderer, SDL_Rect{0, 0, canvas_width, 45}, theme.bar);

  struct TabVisual {
    int index = 0;
    float x = 0.0f;
    float opacity = 1.0f;
    bool selected = false;
  };

  const int nav_count = static_cast<int>(session.navigation.size());
  const int active_index = nav_count > 0
                               ? std::clamp(session.active_nav_index, 0, nav_count - 1)
                               : 0;
  const Uint32 now = SDL_GetTicks();
  if (nav_count <= 0) {
    g_tab_transition = TabTransitionState{};
  } else if (g_tab_transition.active_index < 0 ||
             g_tab_transition.nav_count != nav_count) {
    g_tab_transition.active_index = active_index;
    g_tab_transition.from_index = active_index;
    g_tab_transition.direction = 0;
    g_tab_transition.nav_count = nav_count;
    g_tab_transition.started_at = now;
  } else if (active_index != g_tab_transition.active_index) {
    g_tab_transition.from_index = g_tab_transition.active_index;
    g_tab_transition.direction =
        NavDirection(g_tab_transition.active_index, active_index, nav_count);
    g_tab_transition.active_index = active_index;
    g_tab_transition.started_at = now;
  }

  float progress = 1.0f;
  if (g_tab_transition.direction != 0) {
    progress = std::min(1.0f, (now - g_tab_transition.started_at) /
                                  static_cast<float>(kTabTransitionDurationMs));
    if (progress >= 1.0f) {
      g_tab_transition.direction = 0;
      g_tab_transition.from_index = g_tab_transition.active_index;
    }
  }

  std::vector<TabVisual> visuals;
  if (nav_count > 0) {
    const int visible_count = std::min(6, nav_count);
    if (g_tab_transition.direction == 0) {
      float x = 0.0f;
      for (int slot = 0; slot < visible_count; ++slot) {
        const int index = WrappedNavIndex(active_index + slot, nav_count);
        visuals.push_back({index, x, 1.0f, slot == 0});
        x += static_cast<float>(TabWidth(TabItemAt(session, index)));
      }
    } else {
      const float eased = SmoothStep(progress);
      std::vector<int> old_order;
      std::vector<float> old_positions;
      old_order.reserve(visible_count);
      old_positions.reserve(visible_count);
      float x = 0.0f;
      for (int slot = 0; slot < visible_count; ++slot) {
        const int index = WrappedNavIndex(g_tab_transition.from_index + slot, nav_count);
        old_order.push_back(index);
        old_positions.push_back(x);
        x += static_cast<float>(TabWidth(TabItemAt(session, index)));
      }
      const float end_x = x;

      if (g_tab_transition.direction > 0) {
        const int outgoing = old_order.front();
        const float distance = static_cast<float>(TabWidth(TabItemAt(session, outgoing)));
        visuals.push_back({outgoing, old_positions.front() - eased * distance,
                           1.0f - eased, false});
        for (int slot = 1; slot < visible_count; ++slot) {
          const int index = old_order[slot];
          visuals.push_back({index, old_positions[slot] - eased * distance,
                             1.0f, index == active_index});
        }
        visuals.push_back({outgoing, end_x - eased * distance, eased, false});
      } else {
        const int incoming = active_index;
        const float distance = static_cast<float>(TabWidth(TabItemAt(session, incoming)));
        visuals.push_back({incoming, -distance + eased * distance, eased, true});
        for (int slot = 0; slot < visible_count - 1; ++slot) {
          const int index = old_order[slot];
          visuals.push_back({index, old_positions[slot] + eased * distance,
                             1.0f, false});
        }
        visuals.push_back({incoming, old_positions.back() + eased * distance,
                           1.0f - eased, false});
      }
    }
  }

  const SDL_Rect tabs_clip{0, 0, std::max(1, canvas_width - 140), 45};
  SDL_RenderSetClipRect(renderer, &tabs_clip);
  SDL_BlendMode previous_blend = SDL_BLENDMODE_NONE;
  SDL_GetRenderDrawBlendMode(renderer, &previous_blend);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  for (const TabVisual &visual : visuals) {
    const UiNavItem &item = TabItemAt(session, visual.index);
    const int width = TabWidth(item);
    if (visual.selected) {
      FillRightSlantTab(renderer, static_cast<int>(std::lround(visual.x)), 1,
                        width, 42, 8, WithOpacity(theme.selected, visual.opacity));
    }
  }

  for (const TabVisual &visual : visuals) {
    const UiNavItem &item = TabItemAt(session, visual.index);
    const int tab_x = static_cast<int>(std::lround(visual.x));
    const int width = TabWidth(item);
    DrawText(renderer, item.title, tab_x + width / 2, 13,
             kTabTextFontSize,
             WithOpacity(visual.selected ? kInk : SDL_Color{160, 164, 170, 255},
                         visual.opacity),
             0, true);
    const SDL_Color divider = WithOpacity(SDL_Color{128, 132, 138, 255}, visual.opacity);
    SetColor(renderer, divider);
    SDL_RenderDrawLine(renderer, tab_x + width - 8, 42, tab_x + width + 8, 2);
  }
  SDL_SetRenderDrawBlendMode(renderer, previous_blend);
  SDL_RenderSetClipRect(renderer, nullptr);

  SetColor(renderer, SDL_Color{215, 217, 220, 255});
  SDL_RenderDrawLine(renderer, 0, 44, canvas_width - 1, 44);

  constexpr SDL_Color kStatusWhite{255, 255, 255, 255};
  if (!session.navigation.empty()) {
    const int nav_index = std::clamp(session.active_nav_index, 0,
                                     static_cast<int>(session.navigation.size()) - 1);
    const int game_total = static_cast<int>(session.visible_game_indices.size());
    const int game_index = game_total > 0
                               ? std::clamp(session.selected_visible_index + 1, 1, game_total)
                               : 0;
    const int game_index_width = DigitCount(game_total);
    const std::string game_index_text =
        PaddedNumber(game_index, game_index_width) + "/" +
        PaddedNumber(game_total, game_index_width);
    const std::string nav_index_text =
        "合集:" + PaddedNumber(nav_index + 1, 2) + "/" +
        PaddedNumber(static_cast<int>(session.navigation.size()), 2);
    DrawTextRight(renderer, game_index_text, canvas_width - 84, 26, 10,
                  SDL_Color{235, 237, 240, 220});
    DrawTextRight(renderer, nav_index_text, canvas_width - 5, 26, 10,
                  SDL_Color{235, 237, 240, 205});
  }

  const int battery_percent = session.system_status.battery_percent;
  const std::string battery_text =
      battery_percent >= 0 ? std::to_string(std::clamp(battery_percent, 0, 100)) + "%"
                           : "--%";
  constexpr int kBatteryOffsetY = 3;
  DrawTextRight(renderer, battery_text, canvas_width - 31, 0 + kBatteryOffsetY, 9,
                kStatusWhite);
  const SDL_Rect battery_body{canvas_width - 27, 2 + kBatteryOffsetY, 18, 10};
  Stroke(renderer, battery_body, kStatusWhite);
  Fill(renderer, SDL_Rect{canvas_width - 8, 5 + kBatteryOffsetY, 2, 4},
       kStatusWhite);
  const int fill_width = battery_percent >= 0
                             ? (battery_body.w - 4) * std::clamp(battery_percent, 0, 100) / 100
                             : 10;
  const SDL_Color battery_color = session.system_status.charging
                                      ? SDL_Color{92, 198, 255, 255}
                                      : SDL_Color{76, 219, 111, 255};
  Fill(renderer, SDL_Rect{battery_body.x + 2, battery_body.y + 2,
                          fill_width, battery_body.h - 4},
       battery_color);
}

void RenderGameInfo(SDL_Renderer *renderer, const UiSession &session) {
  const Game *game = SelectedGameForRender(session);
  const int canvas_height = CanvasHeight();
  Fill(renderer, SDL_Rect{0, 45, 240, std::max(1, canvas_height - 45)}, kBackground);
  SetColor(renderer, SDL_Color{50, 52, 55, 255});
  SDL_RenderDrawLine(renderer, 239, 45, 239, canvas_height - 1);

  if (!game) {
    DrawText(renderer, "此分类暂无游戏", 120, (canvas_height + 45) / 2 - 25,
             18, kMuted, 208, true);
    return;
  }

  const bool has_logo = !game->media.logo.empty();
  constexpr int kMediaTopOffset = 12;
  int media_y = 151 + kMediaTopOffset;
  if (has_logo) {
    const SDL_Rect logo_bounds{18, 52, 204, 67};
    DrawMediaImage(renderer, game->media.logo, logo_bounds);
    DrawScrollingText(renderer, game->title, SDL_Rect{13, 120, 214, 25}, 17, kInk);
  } else {
    DrawScrollingText(renderer, game->title, SDL_Rect{13, 51, 214, 25}, 17, kInk);
    media_y = 84 + kMediaTopOffset;
  }

  const SDL_Rect video_bounds{13, media_y, 214, 120};
  Fill(renderer, video_bounds, SDL_Color{0, 0, 0, 255});
  const SDL_Rect media_inner{video_bounds.x + 1, video_bounds.y + 1,
                             video_bounds.w - 2, video_bounds.h - 2};
  SDL_Texture *video_texture = VideoTexture(renderer);
  const bool expects_video =
      session.preferences.preview_video_enabled && !game->media.video.empty();
  if (video_texture) {
    SDL_RenderCopy(renderer, video_texture, nullptr, &media_inner);
  } else if (!expects_video) {
    const SDL_Color fallback = SDL_Color{24, 27, 31, 255};
    Fill(renderer, media_inner, fallback);
    if (!DrawCoverImage(renderer, *game, media_inner)) {
      DrawText(renderer, PlatformLabel(*game), video_bounds.x + video_bounds.w / 2,
               video_bounds.y + video_bounds.h / 2 - 14, 22, kMuted,
               video_bounds.w - 16, true);
    }
  }
  Stroke(renderer, video_bounds, SDL_Color{58, 61, 65, 255});

  if (!game->developer.empty()) {
    DrawText(renderer, game->developer, 13, media_y + 127, 12, kMuted, 214);
  }
  const std::string core_text = "核心 " + CurrentCoreDisplayName(session);
  DrawText(renderer, core_text, 13, media_y + 143, 12, kMuted, 214);
  if (!game->description.empty()) {
    const int description_size = DescriptionFontSize(session);
    const int line_height = description_size + 5;
    const int max_lines = std::max(1, (canvas_height - (media_y + 167) - 8) /
                                           line_height);
    DrawWrappedText(renderer, game->description, 13, media_y + 167, 214,
                    line_height, max_lines,
                    description_size, kInk, session.description_scroll_line);
  }
}

std::vector<LaunchTarget> TargetOptionsForRender(const Game *game) {
  std::vector<LaunchTarget> targets;
  if (!game) return targets;
  targets.push_back(game->primary_target);
  targets.insert(targets.end(), game->alternate_targets.begin(), game->alternate_targets.end());
  return targets;
}

std::string TargetLabel(const LaunchTarget &target) {
  if (!target.label.empty() && target.label != "default") return target.label;
  if (target.path.empty()) return "默认版本";
  const fs::path path = fs::u8path(target.path);
  const std::string stem = path.stem().u8string();
  return stem.empty() ? path.filename().u8string() : stem;
}

void DrawCoverTitle(SDL_Renderer *renderer, const Game &game, const SDL_Rect &bounds,
                    int size, SDL_Color color) {
  DrawText(renderer, game.title, bounds.x + bounds.w / 2, bounds.y, size, color,
           bounds.w, true);
}

void RenderGrid(SDL_Renderer *renderer, const UiSession &session) {
  const ThemeColors &theme = ThemeForSession(session);
  const SDL_Rect bounds = GridBounds(session);
  const int columns = GridColumns(session);
  const bool pegasus_grid = UsePegasusGridLayout(session);
  const int inset = pegasus_grid ? 12 : GridInset(session);
  const int gap = pegasus_grid ? 2 : 0;
  const int card_width = pegasus_grid
                             ? std::max(1, (bounds.w - inset * 2 - gap * (columns - 1)) / columns)
                             : GridCardSize(session);
  const int raw_scroll_row = std::max(0, session.scroll_offset / columns);
  const int raw_start = raw_scroll_row * columns;
  const int card_height = pegasus_grid
                              ? std::max(1, static_cast<int>(
                                                std::lround(card_width *
                                                            PegasusCoverAspect(renderer,
                                                                               session,
                                                                               raw_start))))
                              : card_width;
  const int step_x = card_width + gap;
  const int step_y = card_height + gap;
  const int content_width = pegasus_grid
                                ? card_width * columns + gap * (columns - 1)
                                : card_width * columns;
  const int base_x = bounds.x + (bounds.w - content_width) / 2;
  const int base_y = bounds.y + inset;
  const int visible_rows = pegasus_grid
                               ? std::max(1, (bounds.h - inset * 2) / step_y)
                               : GridVisibleRows(session);
  int scroll_row = raw_scroll_row;
  if (session.selected_visible_index >= 0) {
    const int selected_row = session.selected_visible_index / columns;
    if (selected_row < scroll_row) {
      scroll_row = selected_row;
    } else if (selected_row >= scroll_row + visible_rows) {
      scroll_row = std::max(0, selected_row - visible_rows + 1);
    }
  }
  const int start = scroll_row * columns;
  const int slots = (visible_rows + 1) * columns;
  const SDL_Rect grid_clip = bounds;

  if (session.visible_game_indices.empty()) {
    DrawText(renderer, "当前列表无游戏", bounds.x + bounds.w / 2,
             bounds.y + bounds.h / 2 - 10, 20, kMuted,
             std::max(120, bounds.w - inset * 2), true);
    return;
  }

  QueueVisibleCoverImages(renderer, session, start, slots);

  auto draw_card = [&](int visible_index, const SDL_Rect &cover, bool highlighted) {
    const Game &game = session.library.games[session.visible_game_indices[visible_index]];
    Fill(renderer, cover, kPanel);
    if (!DrawCoverImage(renderer, game, cover)) {
      DrawText(renderer, PlatformLabel(game), cover.x + cover.w / 2,
               cover.y + cover.h / 2 - 12, 18, kMuted, cover.w - 8, true);
    }

    if (session.preferences.show_cover_titles) {
      const int title_size = CoverTitleFontSize(session);
      const int title_height = std::clamp(std::max(cover.h / 5, title_size + 8),
                                          22, 34);
      const SDL_Rect title_bg{cover.x, cover.y + cover.h - title_height,
                              cover.w, title_height};
      SDL_Color title_background = theme.selected;
      title_background.a = 168;
      FillBlend(renderer, title_bg, title_background);
      DrawCoverTitle(renderer, game,
                     SDL_Rect{cover.x + 4, cover.y + cover.h - title_height + 4,
                              cover.w - 8, title_height - 4},
                     title_size, SDL_Color{255, 255, 255, 210});
    }
    if (game.favorite) {
      DrawText(renderer, "*", cover.x + cover.w - 11, cover.y + 3, 20, theme.accent, 16, true);
    }

    if (highlighted) {
      constexpr double kPi = 3.14159265358979323846;
      const double phase = (SDL_GetTicks() % 1400) / 1400.0 * 2.0 * kPi;
      const Uint8 alpha = static_cast<Uint8>(185 + 70 * (0.5 + 0.5 * std::sin(phase)));
      Stroke(renderer, cover, SDL_Color{255, 255, 255, alpha});
      const SDL_Rect inner{cover.x + 1, cover.y + 1, cover.w - 2, cover.h - 2};
      Stroke(renderer, inner, SDL_Color{255, 255, 255, static_cast<Uint8>(alpha / 2)});
    }
  };

  SDL_RenderSetClipRect(renderer, &grid_clip);
  struct ScaledCard {
    int visible_index = -1;
    SDL_Rect cover{};
    bool highlighted = false;
  };
  std::vector<ScaledCard> scaled_cards;
  for (int slot = 0; slot < slots; ++slot) {
    const int visible_index = start + slot;
    if (visible_index >= static_cast<int>(session.visible_game_indices.size())) break;
    const int column = slot % columns;
    const int row = slot / columns;
    const SDL_Rect cell{base_x + column * step_x, base_y + row * step_y,
                        card_width, card_height};
    const SDL_Rect cover = pegasus_grid
                               ? cell
                               : SDL_Rect{cell.x + 3, cell.y + 3,
                                          cell.w - 6, cell.h - 6};
    draw_card(visible_index, cover, false);
    if (visible_index == session.selected_visible_index) {
      const Game &game = session.library.games[session.visible_game_indices[visible_index]];
      const double scale = SelectedScale(game);
      const int expanded_width = static_cast<int>(std::lround(cover.w * scale));
      const int expanded_height = static_cast<int>(std::lround(cover.h * scale));
      const int center_x = cell.x + cell.w / 2;
      const int center_y = cell.y + cell.h / 2;
      scaled_cards.push_back({visible_index,
                              ClampRectInside(
                                  SDL_Rect{center_x - expanded_width / 2,
                                           center_y - expanded_height / 2,
                                           expanded_width, expanded_height},
                                  grid_clip),
                              true});
    }
  }
  for (const ScaledCard &card : scaled_cards) {
    draw_card(card.visible_index, card.cover, card.highlighted);
  }
  PrewarmNearbyCoverImages(renderer, session);
  SDL_RenderSetClipRect(renderer, nullptr);
}

std::string GridSizeText(UiGridSize size) {
  switch (size) {
    case UiGridSize::Large:
      return "大格子";
    case UiGridSize::Small:
      return "小格子";
    case UiGridSize::Medium:
      return "中格子";
  }
  return "中格子";
}

std::string PreviewVideoText(const UiSession &session) {
  if (!session.preferences.preview_video_enabled) return "< 关闭 >";
  return session.preferences.preview_video_loop ? "< 循环 >" : "< 单次 >";
}

std::string ThemeText(int index) {
  static const char *kThemeNames[] = {
      "蓝灰", "玫瑰粉", "金属银", "黑色", "靛蓝", "透明绿", "透明红", "冰川蓝",
      "复古灰", "白色", "透明黑", "透明白", "透明紫", "透明蓝", "银色", "米白",
      "熔岩橙", "蓝色", "黄色", "金属蓝", "透明青", "石墨黑", "铂金银",
      "珠光蓝", "珠光粉",
  };
  static_assert(static_cast<int>(sizeof(kThemeNames) / sizeof(kThemeNames[0])) ==
                    kUiThemeColorCount,
                "theme names must match ui model theme count");
  constexpr int count = static_cast<int>(sizeof(kThemeNames) / sizeof(kThemeNames[0]));
  return kThemeNames[(index % count + count) % count];
}

std::string StartupLogoText(int index) {
  static const char *kStartupLogoNames[] = {
      "RGFrontend",
      "经典Logo",
  };
  constexpr int count = static_cast<int>(sizeof(kStartupLogoNames) /
                                         sizeof(kStartupLogoNames[0]));
  static_assert(count == kUiStartupLogoStyleCount,
                "startup logo names must match ui model count");
  return kStartupLogoNames[(index % count + count) % count];
}

std::string SettingsValue(const UiSession &session, int index) {
  switch (index) {
    case 0:
    case 1:
      return "";
    case 2:
      if (session.system_status.autostart_enabled < 0) return "不可用";
      return session.system_status.autostart_enabled > 0 ? "< 开启 >" : "< 关闭 >";
    case 3:
      return session.system_status.brightness >= 0
                 ? "< " + std::to_string(session.system_status.brightness) + " >"
                 : "不可用";
    case 4:
      return session.system_status.volume >= 0
                 ? "< " + std::to_string(session.system_status.volume) + " >"
                 : "不可用";
    case 5:
      return PreviewVideoText(session);
    case 6:
      return "< " + GridSizeText(session.preferences.grid_size) + " >";
    case 7:
      return session.preferences.fullscreen_grid ? "< 开启 >" : "< 关闭 >";
    case 8:
      return ThemeText(session.preferences.theme_color) + " >";
    case 9:
      return "< " + StartupLogoText(session.preferences.startup_logo_style) + " >";
    case 10:
      return session.preferences.show_cover_titles ? "< 显示 >" : "< 隐藏 >";
    case 11:
      return "< " + std::to_string(CoverTitleFontSize(session)) + " >";
    case 12:
      return "< " + std::to_string(DescriptionFontSize(session)) + " >";
    default:
      return "";
  }
}

void RenderSettingsView(SDL_Renderer *renderer, const UiSession &session) {
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  const int right_edge = canvas_width - 37;
  const int row_width = std::max(1, canvas_width - 72);
  const int value_right = canvas_width - 56;
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height},
       SDL_Color{11, 12, 14, 255});
  DrawText(renderer, "设置", 36, 25, 28, kInk);
  DrawTextRight(renderer, "RGFrontend", right_edge, 30, 15, kMuted);
  SetColor(renderer, SDL_Color{72, 75, 80, 255});
  SDL_RenderDrawLine(renderer, 36, 66, right_edge, 66);

  static const char *kLabels[] = {
      "返回官方系统",
      "前端快捷键",
      "开机自动进入",
      "屏幕亮度",
      "系统音量",
      "预览视频",
      "封面大小",
      "全屏网格",
      "主题颜色",
      "加载Logo",
      "封面标题",
      "封面标题字号",
      "介绍文字字号",
      "清空缓存并扫描",
      "重启",
      "关机",
      "关于 RGFrontend",
  };
  constexpr int kSettingsCount = static_cast<int>(sizeof(kLabels) / sizeof(kLabels[0]));
  constexpr int kRowY = 78;
  constexpr int kRowHeight = 62;
  const int kVisibleRows = std::min(kSettingsCount,
                                    std::max(6, (canvas_height - 108) / kRowHeight));
  const int scroll = std::max(0, std::min(session.settings_scroll_offset,
                                          kSettingsCount - kVisibleRows));
  for (int slot = 0; slot < kVisibleRows; ++slot) {
    const int index = scroll + slot;
    if (index >= kSettingsCount) break;
    const SDL_Rect row{36, kRowY + slot * kRowHeight, row_width, kRowHeight};
    if (index == session.settings_selected_index) {
      Fill(renderer, row, SDL_Color{42, 45, 50, 255});
      Fill(renderer, SDL_Rect{36, row.y, 4, row.h}, theme.accent);
    }
    DrawText(renderer, kLabels[index], 56, row.y + 19, 19, kInk, 390);
    if (index == 7) {
      Fill(renderer, SDL_Rect{482, row.y + 18, 28, 28}, theme.selected);
      Stroke(renderer, SDL_Rect{482, row.y + 18, 28, 28}, SDL_Color{160, 164, 170, 255});
    }
    DrawTextRight(renderer, SettingsValue(session, index), value_right, row.y + 20, 17,
                  index >= 12 || ((index >= 1 && index <= 3) &&
                                  SettingsValue(session, index) == "不可用")
                      ? kMuted
                      : kInk);
  }

  const SDL_Rect track{canvas_width - 46, 78, 4, kVisibleRows * kRowHeight};
  Fill(renderer, track, SDL_Color{34, 36, 40, 255});
  const int thumb_height = std::max(48, track.h * kVisibleRows / kSettingsCount);
  const int max_scroll = std::max(1, kSettingsCount - kVisibleRows);
  const int thumb_y = track.y + (track.h - thumb_height) * scroll / max_scroll;
  Fill(renderer, SDL_Rect{track.x, thumb_y, track.w, thumb_height}, theme.accent);

  DrawText(renderer, "A确认  B返回  方向键调整", 36, canvas_height - 26, 14,
           kMuted, 420);
}

void RenderAboutView(SDL_Renderer *renderer, const UiSession &session) {
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height},
       SDL_Color{11, 12, 14, 255});

  constexpr int kTextX = 56;
  int y = 48;
  DrawText(renderer, "RGFrontend", kTextX, y, 30, kInk, canvas_width - 112);
  y += 42;
  DrawText(renderer, "Version 1.0.0", kTextX, y, 18, kMuted,
           canvas_width - 112);
  y += 34;
  DrawText(renderer, "Developed by zhangjiyz", kTextX, y, 18, kMuted,
           canvas_width - 112);
  y += 54;
  DrawText(renderer, "Copyright (c) 2026 zhangjiyz", kTextX, y, 17, kInk,
           canvas_width - 112);
  y += 34;
  DrawText(renderer, "Contact: QQ 604582868", kTextX, y, 17, kInk,
           canvas_width - 112);
  y += 32;
  DrawText(renderer, "Website: zhangjiyz.com", kTextX, y, 17, kInk,
           canvas_width - 112);
  y += 58;
  DrawText(renderer, "Thanks", kTextX, y, 17, theme.accent, canvas_width - 112);
  y += 36;
  DrawText(renderer, "PegasusG by ROC / Blood_roc", kTextX, y, 17, kInk,
           canvas_width - 112);
  y += 32;
  DrawText(renderer, "Pegasus Frontend", kTextX, y, 17, kInk,
           canvas_width - 112);
  y += 32;
  DrawText(renderer, "RetroArch / Libretro", kTextX, y, 17, kInk,
           canvas_width - 112);

  DrawText(renderer, "A/B返回", 36, canvas_height - 26, 14, kMuted, 420);
}

void RenderHotkeysView(SDL_Renderer *renderer, const UiSession &session) {
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height},
       SDL_Color{11, 12, 14, 255});

  constexpr int kTextX = 56;
  const int content_width = canvas_width - 112;
  int y = 34;
  DrawText(renderer, "前端快捷键", kTextX, y, 30, kInk, content_width);
  y += 52;
  DrawText(renderer, "手柄按键", kTextX, y, 17, theme.accent, content_width);
  y += 30;

  struct Row {
    const char *key;
    const char *action;
  };
  static const Row kDeviceRows[] = {
      {"方向键", "移动选择"},
      {"A", "确认 / 启动"},
      {"B", "返回 / 取消"},
      {"L1 / R1", "切换平台"},
      {"L2 / R2", "滚动介绍"},
      {"X", "显示/隐藏标题"},
      {"Y", "选择游戏核心"},
      {"Select", "收藏/取消"},
      {"Start/Menu", "打开设置"},
      {"音量 -/+", "调整音量"},
      {"Power", "休眠"},
  };
  constexpr int kKeyWidth = 132;
  constexpr int kRowHeight = 28;
  for (const Row &row : kDeviceRows) {
    DrawText(renderer, row.key, kTextX, y, 17, kInk, kKeyWidth);
    DrawText(renderer, row.action, kTextX + kKeyWidth, y, 17, kMuted,
             content_width - kKeyWidth);
    y += kRowHeight;
  }

  DrawText(renderer, "A/B返回", 36, canvas_height - 26, 14, kMuted, 420);
}

void RenderThemeSelectView(SDL_Renderer *renderer, const UiSession &session) {
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  const int right_edge = canvas_width - 37;
  const int row_width = std::max(1, canvas_width - 72);
  const int value_right = canvas_width - 56;
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height},
       SDL_Color{11, 12, 14, 255});
  DrawText(renderer, "主题颜色", 36, 25, 28, kInk);
  DrawTextRight(renderer, ThemeText(session.preferences.theme_color), right_edge, 30, 15,
                kMuted);
  SetColor(renderer, SDL_Color{72, 75, 80, 255});
  SDL_RenderDrawLine(renderer, 36, 66, right_edge, 66);

  constexpr int kRowY = 76;
  constexpr int kRowHeight = 49;
  const int kVisibleRows = std::min(kUiThemeColorCount,
                                    std::max(7, (canvas_height - 106) / kRowHeight));
  const int scroll = std::max(0, std::min(session.theme_scroll_offset,
                                          kUiThemeColorCount - kVisibleRows));
  for (int slot = 0; slot < kVisibleRows; ++slot) {
    const int index = scroll + slot;
    if (index >= kUiThemeColorCount) break;
    const SDL_Rect row{36, kRowY + slot * kRowHeight, row_width, kRowHeight};
    const ThemeColors &row_theme = ThemeByIndex(index);
    if (index == session.theme_selected_index) {
      Fill(renderer, row, SDL_Color{42, 45, 50, 255});
      Fill(renderer, SDL_Rect{36, row.y, 4, row.h}, theme.accent);
    }
    Fill(renderer, SDL_Rect{56, row.y + 12, 26, 26}, row_theme.bar);
    Stroke(renderer, SDL_Rect{56, row.y + 12, 26, 26}, SDL_Color{150, 154, 160, 255});
    Fill(renderer, SDL_Rect{88, row.y + 12, 26, 26}, row_theme.selected);
    Stroke(renderer, SDL_Rect{88, row.y + 12, 26, 26}, SDL_Color{150, 154, 160, 255});
    Fill(renderer, SDL_Rect{120, row.y + 12, 26, 26}, row_theme.accent);
    Stroke(renderer, SDL_Rect{120, row.y + 12, 26, 26}, SDL_Color{150, 154, 160, 255});
    DrawText(renderer, ThemeText(index), 166, row.y + 15, 18, kInk, 360);
    if (index == session.preferences.theme_color) {
      DrawTextRight(renderer, "当前", value_right, row.y + 16, 16, theme.accent);
    }
  }

  const SDL_Rect track{canvas_width - 46, 76, 4, kVisibleRows * kRowHeight};
  Fill(renderer, track, SDL_Color{34, 36, 40, 255});
  const int thumb_height = std::max(44, track.h * kVisibleRows / kUiThemeColorCount);
  const int max_scroll = std::max(1, kUiThemeColorCount - kVisibleRows);
  const int thumb_y = track.y + (track.h - thumb_height) * scroll / max_scroll;
  Fill(renderer, SDL_Rect{track.x, thumb_y, track.w, thumb_height}, theme.accent);

  DrawText(renderer, "方向键选择  A/B返回", 36, canvas_height - 26, 14, kMuted,
           420);
}

void RenderTargetSelectOverlay(SDL_Renderer *renderer, const UiSession &session) {
  const Game *game = SelectedGameForRender(session);
  const std::vector<LaunchTarget> targets = TargetOptionsForRender(game);
  if (!game || targets.size() <= 1) return;
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height}, SDL_Color{0, 0, 0, 120});
  const int max_visible_rows = std::max(5, std::min(8, (canvas_height - 160) / 45));
  const int visible_rows = std::min(max_visible_rows, static_cast<int>(targets.size()));
  const int dialog_height = 86 + visible_rows * 45;
  const int dialog_width = std::min(480, std::max(320, canvas_width - 120));
  const SDL_Rect dialog{(canvas_width - dialog_width) / 2,
                        (canvas_height - dialog_height) / 2,
                        dialog_width, dialog_height};
  Fill(renderer, dialog, SDL_Color{17, 18, 20, 250});
  Stroke(renderer, dialog, SDL_Color{112, 116, 122, 255});
  DrawText(renderer, "选择游戏版本", dialog.x + dialog.w / 2, dialog.y + 17, 20,
           kInk, dialog.w - 40, true);
  DrawText(renderer, game->title, dialog.x + dialog.w / 2, dialog.y + 42, 13,
           kMuted, dialog.w - 60, true);

  const int scroll = std::max(0, std::min(session.target_scroll_offset,
                                          static_cast<int>(targets.size()) - visible_rows));
  for (int slot = 0; slot < visible_rows; ++slot) {
    const int index = scroll + slot;
    if (index >= static_cast<int>(targets.size())) break;
    const SDL_Rect row{dialog.x + 20, dialog.y + 68 + slot * 45,
                       dialog.w - 40, 38};
    if (index == session.target_selected_index) {
      Fill(renderer, row, SDL_Color{48, 51, 56, 255});
      Fill(renderer, SDL_Rect{row.x, row.y, 4, row.h}, theme.accent);
    }
    const SDL_Color color = index == session.target_selected_index ? theme.accent : kInk;
    DrawText(renderer, TargetLabel(targets[index]), row.x + 14, row.y + 8, 17,
             color, row.w - 28);
  }
}

void RenderCoreSelectOverlay(SDL_Renderer *renderer, const UiSession &session) {
  const Game *game = SelectedGameForRender(session);
  const std::vector<UiCoreOption> options = CoreOptionsForGame(session);
  if (!game || options.empty()) return;
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height}, SDL_Color{0, 0, 0, 128});

  const int max_visible_rows = std::max(5, std::min(8, (canvas_height - 170) / 45));
  const int visible_rows = std::min(max_visible_rows, static_cast<int>(options.size()));
  const int header_height = 76;
  const int footer_height = 36;
  const int dialog_height = header_height + footer_height + visible_rows * 45;
  const int dialog_width = std::min(480, std::max(340, canvas_width - 120));
  const SDL_Rect dialog{(canvas_width - dialog_width) / 2,
                        (canvas_height - dialog_height) / 2,
                        dialog_width, dialog_height};
  Fill(renderer, dialog, SDL_Color{17, 18, 20, 250});
  Stroke(renderer, dialog, SDL_Color{112, 116, 122, 255});
  DrawText(renderer, "选择游戏核心", dialog.x + dialog.w / 2, dialog.y + 12, 19,
           kInk, dialog.w - 40, true);
  DrawText(renderer, game->title, dialog.x + dialog.w / 2, dialog.y + 43, 13,
           kMuted, dialog.w - 60, true);

  const int scroll = std::max(0, std::min(session.core_scroll_offset,
                                          static_cast<int>(options.size()) - visible_rows));
  const std::string current = game->user_core_hint;
  for (int slot = 0; slot < visible_rows; ++slot) {
    const int index = scroll + slot;
    if (index >= static_cast<int>(options.size())) break;
    const UiCoreOption &option = options[index];
    const SDL_Rect row{dialog.x + 20, dialog.y + header_height + slot * 45,
                       dialog.w - 40, 38};
    if (index == session.core_selected_index) {
      Fill(renderer, row, SDL_Color{48, 51, 56, 255});
      Fill(renderer, SDL_Rect{row.x, row.y, 4, row.h}, theme.accent);
    }
    const SDL_Color color = index == session.core_selected_index ? theme.accent : kInk;
    DrawText(renderer, option.label, row.x + 14, row.y + 8, 17, color, row.w - 96);
    if (option.core == current) {
      DrawTextRight(renderer, "当前", row.x + row.w - 14, row.y + 9, 15, theme.accent);
    }
  }
  DrawText(renderer, "A选择  B取消", dialog.x + dialog.w / 2,
           dialog.y + dialog.h - 27, 14, kMuted, dialog.w - 40, true);
}

void RenderNoticeOverlay(SDL_Renderer *renderer, const UiSession &session) {
  if (!session.notice.visible) return;
  const ThemeColors &theme = ThemeForSession(session);
  const int canvas_width = CanvasWidth();
  const int canvas_height = CanvasHeight();
  Fill(renderer, SDL_Rect{0, 0, canvas_width, canvas_height}, SDL_Color{0, 0, 0, 132});
  const int dialog_width = std::min(484, std::max(320, canvas_width - 96));
  const SDL_Rect dialog{(canvas_width - dialog_width) / 2,
                        (canvas_height - 180) / 2, dialog_width, 180};
  Fill(renderer, dialog, SDL_Color{17, 18, 20, 252});
  Stroke(renderer, dialog, SDL_Color{112, 116, 122, 255});
  Fill(renderer, SDL_Rect{dialog.x, dialog.y, 5, dialog.h}, theme.accent);
  DrawText(renderer, session.notice.title, dialog.x + dialog.w / 2, dialog.y + 24, 22, kInk,
           dialog.w - 52, true);
  DrawWrappedText(renderer, session.notice.message, dialog.x + 34, dialog.y + 68,
                  dialog.w - 68, 21, 3, 15, kInk);
  DrawText(renderer, "A/B关闭", dialog.x + dialog.w / 2, dialog.y + dialog.h - 34,
           14, kMuted, dialog.w - 52, true);
}

void RenderOsd(SDL_Renderer *renderer, const UiSession &session) {
  if (session.osd_frames_remaining <= 0 || session.osd_text.empty()) return;
  const ThemeColors &theme = ThemeForSession(session);
  const SDL_Rect box{(CanvasWidth() - 256) / 2, CanvasHeight() - 88, 256, 42};
  Fill(renderer, box, SDL_Color{0, 0, 0, 205});
  Stroke(renderer, box, theme.accent);
  DrawText(renderer, session.osd_text, box.x + box.w / 2, box.y + 11, 17, kInk,
           box.w - 20, true);
}

}  // namespace

void SetRendererFontPath(std::string path) {
  g_font_path = std::move(path);
  for (auto &entry : g_fonts) {
    if (entry.second) TTF_CloseFont(entry.second);
  }
  g_fonts.clear();
}

void SetRendererMediaRoots(std::string app_dir, std::string state_dir) {
  g_app_dir = std::move(app_dir);
  g_state_dir = std::move(state_dir);
  g_music_tracks.clear();
  g_music_track_index = 0;
  g_seen_bgm_track_revision = 0;
}

void ClearRendererMediaCache() {
  StopCoverImageWorker();
  ClearTextureCache(&g_cover_cache);
  ClearTextureCache(&g_text_cache);
  g_cover_cache.renderer = nullptr;
  g_text_cache.renderer = nullptr;
  g_video_preview.Stop();
  g_audio_preview.Stop();
  ClearVideoTexture();
  g_pending_video_path.clear();
  g_pending_video_since = 0;
  g_active_video_path.clear();
  g_cover_request_view_key.clear();
  g_pegasus_aspect_view_key.clear();
  g_pegasus_aspect = kDefaultPegasusCoverAspect;
  g_pegasus_aspect_locked = false;
}

bool RenderLibraryView(SDL_Renderer *renderer, const UiSession &session,
                       const UiLayout &layout) {
  if (!renderer) return false;
  g_render_canvas_width = LogicalWidthForLayout(layout);
  g_render_canvas_height = LogicalHeightForLayout(layout);
  if (EnsureImageRuntime()) DrainReadyCoverImages(renderer);
  SetColor(renderer, kBackground);
  SDL_RenderClear(renderer);

  SDL_Rect previous_viewport{};
  float previous_scale_x = 1.0f;
  float previous_scale_y = 1.0f;
  BeginLogicalCanvas(renderer, layout, &previous_viewport, &previous_scale_x,
                     &previous_scale_y);
  Fill(renderer, SDL_Rect{0, 0, CanvasWidth(), CanvasHeight()}, kBackground);
  if (session.view == UiView::Settings) {
    g_video_preview.Stop();
    g_audio_preview.Stop();
    RenderSettingsView(renderer, session);
  } else if (session.view == UiView::About) {
    g_video_preview.Stop();
    g_audio_preview.Stop();
    RenderAboutView(renderer, session);
  } else if (session.view == UiView::Hotkeys) {
    g_video_preview.Stop();
    g_audio_preview.Stop();
    RenderHotkeysView(renderer, session);
  } else if (session.view == UiView::ThemeSelect) {
    g_video_preview.Stop();
    g_audio_preview.Stop();
    RenderThemeSelectView(renderer, session);
  } else {
    const Game *game = SelectedGameForRender(session);
    SyncVideoPreview(session, game);
    SyncAudioPreview(session, game);
    RenderTopBar(renderer, session);
    if (!session.preferences.fullscreen_grid) RenderGameInfo(renderer, session);
    RenderGrid(renderer, session);
    if (session.view == UiView::TargetSelect) {
      RenderTargetSelectOverlay(renderer, session);
    }
    if (session.view == UiView::CoreSelect) {
      RenderCoreSelectOverlay(renderer, session);
    }
  }
  RenderNoticeOverlay(renderer, session);
  RenderOsd(renderer, session);
  EndLogicalCanvas(renderer, previous_viewport, previous_scale_x, previous_scale_y);

  SDL_RenderPresent(renderer);
  return true;
}

}  // namespace mpl

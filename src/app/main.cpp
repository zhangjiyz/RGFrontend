#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/library_builder.h"
#include "app/desktop_ui_app.h"
#include "devices/h700/launch_request_adapter.h"
#include "devices/h700/platform_registry.h"
#include "devices/h700/system_service.h"
#include "services/state_store.h"
#include "ui/sdl_renderer.h"
#include "ui/ui_model.h"

namespace {

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

long long ElapsedMs(Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             Clock::now() - start)
      .count();
}

int ParseInt(const char *text, int fallback) {
  if (!text) return fallback;
  char *end = nullptr;
  const long value = std::strtol(text, &end, 10);
  if (end == text || *end != '\0') return fallback;
  return static_cast<int>(value);
}

bool EnvFlagEnabled(const char *name) {
  const char *value = std::getenv(name);
  if (!value || value[0] == '\0') return false;
  std::string flag(value);
  std::transform(flag.begin(), flag.end(), flag.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return flag == "1" || flag == "true" || flag == "yes" || flag == "on";
}

bool IsStartupFourThreeCompact(int width, int height) {
  return width <= 660 && height >= 470 && height < 700;
}

mpl::H700RegistryOptions BuildPreviewRegistryOptions(
    const std::vector<std::string> &rom_card_roots) {
  mpl::H700RegistryOptions registry_options;
  registry_options.card_roots = rom_card_roots;
  // Desktop preview only needs platforms marked launchable so navigation shows
  // the scanned content; the app layer still does not execute H700 launchers.
  registry_options.retroarch_launcher = "/bin/sh";
  registry_options.nds_launcher = "/bin/sh";
  registry_options.psp_launcher = "/bin/sh";
  registry_options.openbor_launcher = "/bin/sh";
  registry_options.openbor_setup_script = "/bin/sh";
  registry_options.ports_shell = "/bin/sh";
  registry_options.java_launcher = "/bin/sh";
  registry_options.saturn_launcher = "/bin/sh";
  registry_options.saturn_emulator = "/bin/sh";
  registry_options.saturn_bios = "/bin/sh";
  if (const char *enable_saturn = std::getenv("MPL_H700_ENABLE_SATURN")) {
    registry_options.enable_saturn = std::string(enable_saturn) == "1";
  }
  return registry_options;
}

int StartupLogicalWidth(int width, int height) {
  return IsStartupFourThreeCompact(width, height) ? 640 : 720;
}

int StartupLogicalHeight(int width, int height) {
  if (height >= 700) return 720;
  if (IsStartupFourThreeCompact(width, height)) return 480;
  return 480;
}

std::string FirstExistingFont(const std::string &app_dir) {
  const std::vector<std::string> candidates = {
      (fs::u8path(app_dir) / "assets/fonts/ui_font_02.ttf").u8string(),
      "assets/fonts/ui_font_02.ttf",
      "/mnt/vendor/bin/default.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/System/Library/Fonts/PingFang.ttc",
      "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
      "C:/Windows/Fonts/msyh.ttc",
  };
  for (const std::string &path : candidates) {
    std::error_code error;
    if (fs::is_regular_file(fs::u8path(path), error)) return path;
  }
  return {};
}

std::string ExecutableDirectory(const char *argv0) {
  if (!argv0 || std::string(argv0).empty()) return ".";
  std::error_code error;
  fs::path path = fs::u8path(argv0);
  if (path.is_relative()) path = fs::current_path(error) / path;
  path = fs::weakly_canonical(path, error);
  if (error) path = path.lexically_normal();
  if (path.has_parent_path()) return path.parent_path().u8string();
  return ".";
}

std::string FirstExistingStartupLogo(const std::string &app_dir, int logo_style) {
  if (logo_style == 1) {
    const std::vector<std::string> candidates = {
        (fs::u8path(app_dir) / "assets/apps/classic_icon.png").u8string(),
        "H700/assets/apps/classic_icon.png",
    };
    for (const std::string &path : candidates) {
      std::error_code error;
      if (fs::is_regular_file(fs::u8path(path), error)) return path;
    }
  }

  const std::vector<std::string> candidates = {
      (fs::u8path(app_dir) / "assets/apps/RGFrontend.png").u8string(),
      (fs::u8path(app_dir) / "../Imgs/RGFrontend.png").u8string(),
      "H700/assets/apps/RGFrontend.png",
      "assets/apps/RGFrontend.png",
  };
  for (const std::string &path : candidates) {
    std::error_code error;
    if (fs::is_regular_file(fs::u8path(path), error)) return path;
  }
  return {};
}

class StartupScreen {
 public:
  explicit StartupScreen(const mpl::DesktopUiOptions &options, int logo_style) {
    if (options.render_target != mpl::DesktopRenderTarget::Window || options.hidden_window ||
        std::getenv("MPL_DISABLE_STARTUP_SCREEN")) {
      return;
    }
    simple_mode_ = EnvFlagEnabled("MPL_STARTUP_SCREEN_SIMPLE");
    if (const char *text = std::getenv("MPL_STARTUP_SCREEN_TEXT")) {
      simple_text_ = text;
    }
    if (simple_text_.empty()) simple_text_ = "退出中";
    pegasus_style_ = logo_style == 1;
    width_ = options.width > 0 ? options.width : 720;
    height_ = options.height > 0 ? options.height : 480;
    logical_width_ = StartupLogicalWidth(width_, height_);
    logical_height_ = StartupLogicalHeight(width_, height_);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return;
    sdl_initialized_ = true;
    if (TTF_Init() == 0) {
      ttf_initialized_ = true;
      const std::string font_path = FirstExistingFont(options.app_dir);
      if (!font_path.empty()) {
        text_font_ = TTF_OpenFont(font_path.c_str(), 18);
      }
    }
    window_ = SDL_CreateWindow("RGFrontend",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_, height_, SDL_WINDOW_SHOWN);
    if (!window_) return;
    SDL_ShowWindow(window_);
    SDL_RaiseWindow(window_);
    renderer_ = SDL_CreateRenderer(window_, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) return;
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer_, logical_width_, logical_height_);
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != 0) {
      image_initialized_ = true;
      const std::string logo_path = FirstExistingStartupLogo(options.app_dir, logo_style);
      if (!logo_path.empty()) {
        SDL_Surface *surface = IMG_Load(logo_path.c_str());
        if (surface) {
          logo_width_ = surface->w;
          logo_height_ = surface->h;
          logo_texture_ = SDL_CreateTextureFromSurface(renderer_, surface);
          SDL_FreeSurface(surface);
          if (logo_texture_) SDL_SetTextureBlendMode(logo_texture_, SDL_BLENDMODE_BLEND);
        }
      }
    }
    active_ = true;
  }

  ~StartupScreen() {
    if (logo_texture_) SDL_DestroyTexture(logo_texture_);
    if (text_font_) TTF_CloseFont(text_font_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    if (image_initialized_) IMG_Quit();
    if (ttf_initialized_) TTF_Quit();
    if (sdl_initialized_) SDL_Quit();
  }

  void Update(int percent, const std::string &message) {
    if (!active_ || !renderer_) return;
    percent = std::clamp(percent, 0, 100);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {}

    if (pegasus_style_) {
      SDL_SetRenderDrawColor(renderer_, 34, 34, 34, 255);
    } else {
      SDL_SetRenderDrawColor(renderer_, 3, 5, 8, 255);
    }
    SDL_RenderClear(renderer_);

    if (simple_mode_) {
      DrawTextCentered(simple_text_, 0, 226, logical_width_,
                       {229, 235, 239, 255}, text_font_);
      SDL_RenderPresent(renderer_);
      return;
    }

    if (pegasus_style_) {
      DrawPegasusStyle(percent, message);
    } else {
      DrawLogo();
      DrawTextCentered(std::to_string(percent) + "%", 0, 320, logical_width_,
                       {229, 235, 239, 255}, text_font_);
      const SDL_Rect track{(logical_width_ - 320) / 2, 356, 320, 8};
      Fill(track, {31, 39, 46, 255});
      SDL_Rect fill = track;
      fill.w = std::max(0, (track.w * percent) / 100);
      Fill(fill, {99, 178, 218, 255});
      if (!message.empty()) {
        DrawTextCentered(message, 0, 382, logical_width_,
                         {160, 166, 172, 255}, text_font_);
      }
    }

    SDL_RenderPresent(renderer_);
  }

  bool active() const { return active_; }

 private:
  void Fill(const SDL_Rect &rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
  }

  void FillRounded(const SDL_Rect &rect, int radius, SDL_Color color) {
    if (rect.w <= 0 || rect.h <= 0) return;
    radius = std::max(0, std::min(radius, std::min(rect.w, rect.h) / 2));
    if (radius <= 0) {
      Fill(rect, color);
      return;
    }
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_Rect center{rect.x + radius, rect.y, rect.w - radius * 2, rect.h};
    SDL_RenderFillRect(renderer_, &center);
    const int left_cx = rect.x + radius;
    const int right_cx = rect.x + rect.w - radius - 1;
    const int cy = rect.y + radius;
    for (int offset_y = -radius; offset_y <= radius; ++offset_y) {
      const int span = static_cast<int>(
          std::sqrt(std::max(0, radius * radius - offset_y * offset_y)));
      const int y = cy + offset_y;
      SDL_RenderDrawLine(renderer_, left_cx - span, y, left_cx + span, y);
      SDL_RenderDrawLine(renderer_, right_cx - span, y, right_cx + span, y);
    }
  }

  void DrawLogo() {
    if (!logo_texture_ || logo_width_ <= 0 || logo_height_ <= 0) return;
    const int max_size = 200;
    const double scale = std::min(max_size / static_cast<double>(logo_width_),
                                  max_size / static_cast<double>(logo_height_));
    const int draw_width = std::max(1, static_cast<int>(logo_width_ * scale));
    const int draw_height = std::max(1, static_cast<int>(logo_height_ * scale));
    const SDL_Rect rect{(logical_width_ - draw_width) / 2, 106, draw_width, draw_height};
    SDL_RenderCopy(renderer_, logo_texture_, nullptr, &rect);
  }

  void DrawLogoAt(int center_x, int center_y, int max_size) {
    if (!logo_texture_ || logo_width_ <= 0 || logo_height_ <= 0) return;
    const double scale = std::min(max_size / static_cast<double>(logo_width_),
                                  max_size / static_cast<double>(logo_height_));
    const int draw_width = std::max(1, static_cast<int>(logo_width_ * scale));
    const int draw_height = std::max(1, static_cast<int>(logo_height_ * scale));
    const SDL_Rect rect{center_x - draw_width / 2, center_y - draw_height / 2,
                        draw_width, draw_height};
    SDL_RenderCopy(renderer_, logo_texture_, nullptr, &rect);
  }

  void DrawPegasusStyle(int percent, const std::string &message) {
    const int center_x = logical_width_ / 2;
    DrawLogoAt(center_x, 190, 144);
    DrawTextCentered("RGFrontend", 0, 263, logical_width_,
                     {238, 238, 238, 255}, text_font_);

    const SDL_Rect outer{center_x - 162, 318, 324, 24};
    const SDL_Rect track{center_x - 156, 324, 312, 12};
    FillRounded(outer, 12, {17, 17, 17, 255});
    FillRounded(track, 6, {8, 8, 8, 255});
    SDL_Rect fill = track;
    fill.w = std::max(0, (track.w * percent) / 100);
    if (fill.w > 0) {
      FillRounded(fill, 6, {54, 93, 128, 255});
      SDL_RenderSetClipRect(renderer_, &fill);
      SetColor({96, 144, 180, 255});
      for (int x = fill.x - 24; x < fill.x + fill.w + 18; x += 14) {
        SDL_RenderDrawLine(renderer_, x, fill.y + fill.h + 2, x + 11, fill.y - 2);
      }
      SDL_RenderSetClipRect(renderer_, nullptr);
    }
    DrawTextCentered(std::to_string(percent) + "%", 0, 352, logical_width_,
                     {180, 186, 192, 255}, text_font_);
    if (!message.empty()) {
      DrawTextCentered(message, 0, 382, logical_width_,
                       {160, 166, 172, 255}, text_font_);
    }
  }

  void SetColor(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
  }

  void DrawTextCentered(const std::string &text, int x, int y, int width,
                        SDL_Color color, TTF_Font *font) {
    if (!font || text.empty()) return;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
    const SDL_Rect rect{x + (width - surface->w) / 2, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    if (!texture) return;
    SDL_RenderCopy(renderer_, texture, nullptr, &rect);
    SDL_DestroyTexture(texture);
  }

  int width_ = 720;
  int height_ = 480;
  int logical_width_ = 720;
  int logical_height_ = 480;
  bool active_ = false;
  bool simple_mode_ = false;
  bool pegasus_style_ = false;
  bool sdl_initialized_ = false;
  bool ttf_initialized_ = false;
  bool image_initialized_ = false;
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  TTF_Font *text_font_ = nullptr;
  SDL_Texture *logo_texture_ = nullptr;
  std::string simple_text_;
  int logo_width_ = 0;
  int logo_height_ = 0;
};

void PrintHelp() {
  std::cout << "Usage: mpl_desktop_demo [--width N] [--height N] [--max-frames N]\n"
            << "                        [--hidden] [--surface] [--state-dir PATH]\n"
            << "                        [--restore-ui] [--autostart]\n"
            << "                        [--rom-card-root PATH ...]\n";
}

}  // namespace

int main(int argc, char **argv) {
  const auto process_start = Clock::now();
  mpl::DesktopUiOptions options;
  options.app_dir = ExecutableDirectory(argc > 0 ? argv[0] : nullptr);
  std::vector<std::string> rom_card_roots;
  std::string state_dir = "build/desktop-demo-state";
  bool restore_ui = false;
  bool autostart = false;
  std::unique_ptr<mpl::H700LaunchRequestAdapter> launch_adapter;
  std::unique_ptr<mpl::H700SystemService> system_service;
  for (int index = 1; index < argc; ++index) {
    const std::string arg = argv[index];
    if (arg == "--help") {
      PrintHelp();
      return 0;
    } else if (arg == "--width" && index + 1 < argc) {
      options.width = ParseInt(argv[++index], options.width);
    } else if (arg == "--height" && index + 1 < argc) {
      options.height = ParseInt(argv[++index], options.height);
    } else if (arg == "--max-frames" && index + 1 < argc) {
      options.max_frames = ParseInt(argv[++index], options.max_frames);
    } else if (arg == "--hidden") {
      options.hidden_window = true;
    } else if (arg == "--surface") {
      options.render_target = mpl::DesktopRenderTarget::Surface;
    } else if (arg == "--restore-ui") {
      restore_ui = true;
    } else if (arg == "--autostart") {
      autostart = true;
    } else if (arg == "--state-dir" && index + 1 < argc) {
      state_dir = argv[++index];
      options.state_dir = state_dir;
      options.ui_state_path = state_dir + "/ui.state";
      options.game_state_path = state_dir + "/game.state";
    } else if (arg == "--rom-card-root" && index + 1 < argc) {
      rom_card_roots.push_back(argv[++index]);
    } else {
      std::cerr << "Unknown argument: " << arg << '\n';
      PrintHelp();
      return 2;
    }
  }
  options.state_dir = state_dir;
  const std::string ui_font_path = FirstExistingFont(options.app_dir);
  mpl::SetRendererFontPath(ui_font_path);
  std::cerr << "[frontend] ui.font path="
            << (ui_font_path.empty() ? "<none>" : ui_font_path) << '\n';
  const bool launched_from_game = restore_ui;
  int startup_logo_style = 1;
  {
    mpl::UiState state;
    if (mpl::UiStateStore(options.ui_state_path).Load(&state)) {
      startup_logo_style =
          (state.startup_logo_style % mpl::kUiStartupLogoStyleCount +
           mpl::kUiStartupLogoStyleCount) %
          mpl::kUiStartupLogoStyleCount;
    }
  }
  std::cerr << "[perf] startup.begin restore_ui=" << (restore_ui ? 1 : 0)
            << " rom_roots=" << rom_card_roots.size()
            << " autostart=" << (autostart ? 1 : 0)
            << " startup_logo_style=" << startup_logo_style
            << " state_dir=" << state_dir << '\n';
  if (const char *enable_system = std::getenv("MPL_ENABLE_SYSTEM_SERVICE");
      enable_system && std::string(enable_system) == "1") {
    const auto system_start = Clock::now();
    mpl::H700SystemServiceOptions system_options;
    system_options.state_dir = state_dir;
    system_options.app_dir = options.app_dir;
    if (const char *root = std::getenv("MPL_H700_SYSTEM_ROOT")) {
      system_options.root_prefix = root;
    }
    system_service = std::make_unique<mpl::H700SystemService>(std::move(system_options));
    const bool volume_ready =
        autostart ? system_service->SyncStoredVolumeToSystem()
                  : system_service->RestoreVolume();
    if (!volume_ready) {
      std::cerr << "warning: failed to restore H700 volume\n";
    }
    options.system_service = system_service.get();
    options.audio_device_opened_callback = [service = system_service.get()]() {
      if (service->RestoreVolume()) {
        std::cerr << "[frontend] restored H700 volume after SDL audio open\n";
      } else {
        std::cerr << "warning: failed to restore H700 volume after SDL audio open\n";
      }
    };
    std::cerr << "[perf] startup.system_service ms=" << ElapsedMs(system_start)
              << '\n';
  }

  if (!rom_card_roots.empty()) {
    const auto library_start = Clock::now();
    mpl::H700RegistryOptions registry_options = BuildPreviewRegistryOptions(rom_card_roots);
    const std::string cache_path = state_dir + "/library/scan_cache.tsv";
    options.clear_scan_cache = [cache_path]() {
      std::error_code error;
      fs::remove(fs::u8path(cache_path), error);
      error.clear();
      fs::remove(fs::u8path(cache_path + ".tmp"), error);
      std::cerr << "library cache cleared; restarting startup scan cache_path="
                << cache_path << '\n';
      return true;
    };
    {
      mpl::LibraryBuilder builder;
      std::vector<mpl::Platform> platforms = mpl::LoadH700Platforms(registry_options);
      bool library_loaded = false;
      std::unique_ptr<StartupScreen> startup;
      if (!launched_from_game) {
        startup = std::make_unique<StartupScreen>(options, startup_logo_style);
        startup->Update(8, "检查游戏库缓存");
      }
      if (restore_ui) {
        const auto restore_start = Clock::now();
        const mpl::LibraryBuildReport restored =
            builder.RestoreFromCache(platforms, cache_path);
        options.library = restored.library;
        std::cerr << "[perf] startup.restore_attempt ms="
                  << ElapsedMs(restore_start)
                  << " games=" << options.library.games.size() << '\n';
        if (!options.library.games.empty()) {
          std::cerr << "library restored from scan cache games="
                    << options.library.games.size() << '\n';
          library_loaded = true;
        } else {
          restore_ui = false;
        }
      }
      if (!library_loaded && !launched_from_game &&
          builder.CanRestoreFromCache(platforms, cache_path)) {
        if (startup) startup->Update(45, "读取游戏库缓存");
        const auto restore_start = Clock::now();
        const mpl::LibraryBuildReport restored =
            builder.RestoreFromCache(platforms, cache_path);
        options.library = restored.library;
        if (!options.library.games.empty()) {
          library_loaded = true;
          std::cerr << "[perf] startup.fresh_cache_restore ms="
                    << ElapsedMs(restore_start)
                    << " games=" << options.library.games.size() << '\n';
          std::cerr << "library restored from fresh scan cache games="
                    << options.library.games.size() << '\n';
        }
      }
      if (!library_loaded) {
        const auto build_start = Clock::now();
        if (startup) startup->Update(8, "准备扫描目录");
        options.library = builder
                              .Build(std::move(platforms), cache_path,
                                     [&](const mpl::LibraryBuildProgress &progress) {
                                       if (startup) {
                                         startup->Update(progress.percent, progress.message);
                                       }
                                     })
                              .library;
        if (startup) startup->Update(100, "准备进入游戏库");
        if (startup && startup->active()) SDL_Delay(120);
        std::cerr << "[perf] startup.library_build ms=" << ElapsedMs(build_start)
                  << " games=" << options.library.games.size() << '\n';
      }
      if (library_loaded && startup) {
        startup->Update(100, "准备进入游戏库");
        if (startup->active()) SDL_Delay(120);
      }
    }
    std::cerr << "[perf] startup.library_ready ms=" << ElapsedMs(library_start)
              << " games=" << options.library.games.size()
              << " platforms=" << options.library.platforms.size() << '\n';
    options.include_empty_platforms = false;
    const char *enable_launch = std::getenv("MPL_ENABLE_LAUNCH");
    if (enable_launch && std::string(enable_launch) == "1") {
      mpl::H700LaunchRequestAdapterOptions launch_options;
      launch_options.request_path = std::getenv("MPL_LAUNCH_REQUEST")
                                        ? std::getenv("MPL_LAUNCH_REQUEST")
                                        : (state_dir + "/state/launch.request");
      for (const std::string &root : rom_card_roots) {
        launch_options.trusted_roots.push_back(root + "/Roms");
      }
      if (const char *launcher = std::getenv("MPL_H700_RA_LAUNCHER")) {
        launch_options.retroarch_launcher = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_NDS_LAUNCHER")) {
        launch_options.nds_launcher = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_PSP_LAUNCHER")) {
        launch_options.psp_launcher = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_OPENBOR_LAUNCHER")) {
        launch_options.openbor_launcher = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_OPENBOR_SETUP")) {
        launch_options.openbor_setup_script = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_PORTS_SHELL")) {
        launch_options.ports_shell = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_JAVA_LAUNCHER")) {
        launch_options.java_launcher = launcher;
      }
      if (const char *launcher = std::getenv("MPL_H700_SATURN_LAUNCHER")) {
        launch_options.saturn_launcher = launcher;
      }
      if (const char *emulator = std::getenv("MPL_H700_SATURN_EMULATOR")) {
        launch_options.saturn_emulator = emulator;
      }
      if (const char *bios = std::getenv("MPL_H700_SATURN_BIOS")) {
        launch_options.saturn_bios = bios;
      }
      launch_adapter =
          std::make_unique<mpl::H700LaunchRequestAdapter>(std::move(launch_options));
      options.launch_adapter = launch_adapter.get();
    }
  }

  std::cerr << "[perf] startup.before_ui ms=" << ElapsedMs(process_start) << '\n';
  const int rc = mpl::RunDesktopUiApp(options);
  std::cerr << "[perf] startup.total ms=" << ElapsedMs(process_start)
            << " rc=" << rc << '\n';
  return rc;
}

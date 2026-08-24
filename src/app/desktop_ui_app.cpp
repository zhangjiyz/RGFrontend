#include "app/desktop_ui_app.h"

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

#include "app/demo_library.h"
#include "services/state_store.h"
#include "ui/layout.h"
#include "ui/sdl_input.h"
#include "ui/sdl_renderer.h"

namespace mpl {

namespace {

using Clock = std::chrono::steady_clock;

long long ElapsedMs(Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             Clock::now() - start)
      .count();
}

struct LaunchPreparation {
  bool ok = false;
  std::string title;
  std::string message;
};

struct SdlRendererHandle {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Surface *surface = nullptr;
};

void DestroySdlRendererHandle(SdlRendererHandle *handle) {
  if (!handle) return;
  ClearRendererMediaCache();
  if (handle->renderer) SDL_DestroyRenderer(handle->renderer);
  if (handle->surface) SDL_FreeSurface(handle->surface);
  if (handle->window) SDL_DestroyWindow(handle->window);
  handle->renderer = nullptr;
  handle->surface = nullptr;
  handle->window = nullptr;
}

bool CreateRenderer(const DesktopUiOptions &options, SdlRendererHandle *handle) {
  if (!handle) return false;
  if (options.render_target == DesktopRenderTarget::Surface) {
    handle->surface = SDL_CreateRGBSurfaceWithFormat(0, options.width, options.height, 32,
                                                     SDL_PIXELFORMAT_RGBA32);
    if (!handle->surface) return false;
    handle->renderer = SDL_CreateSoftwareRenderer(handle->surface);
    return handle->renderer != nullptr;
  }

  const Uint32 window_flags = options.hidden_window ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN;
  handle->window = SDL_CreateWindow("RGFrontend",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    options.width, options.height, window_flags);
  if (!handle->window) return false;
  SDL_ShowWindow(handle->window);
  SDL_RaiseWindow(handle->window);
  handle->renderer = SDL_CreateRenderer(handle->window, -1,
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!handle->renderer) {
    handle->renderer = SDL_CreateRenderer(handle->window, -1, SDL_RENDERER_SOFTWARE);
  }
  return handle->renderer != nullptr;
}

void MarkLaunched(UiSession *session, const std::string &game_id) {
  if (!session) return;
  std::uint64_t max_recent = 0;
  for (const Game &game : session->library.games) {
    max_recent = std::max(max_recent, game.recent_order);
  }
  for (Game &game : session->library.games) {
    if (game.id == game_id) {
      game.recent_order = max_recent + 1;
      break;
    }
  }
}

UiSystemStatus ToUiStatus(const SystemStatus &status) {
  UiSystemStatus ui;
  ui.battery_percent = status.battery_percent;
  ui.charging = status.charging;
  ui.brightness = status.brightness;
  ui.volume = status.volume;
  ui.autostart_enabled = status.autostart_enabled;
  return ui;
}

void SetOsd(UiSession *session, std::string text, int frames = 100) {
  if (!session) return;
  session->osd_text = std::move(text);
  session->osd_frames_remaining = frames;
}

void RefreshSystemStatus(UiSession *session, SystemService *service) {
  if (!session || !service) return;
  session->system_status = ToUiStatus(service->ReadStatus());
}

bool PollHallSuspend(SystemService *service, int *hall_state) {
  if (!service || !hall_state) return false;
  const int current = service->ReadHallState();
  const bool closed = *hall_state == 1 && current == 0;
  if (current >= 0) *hall_state = current;
  return closed;
}

std::string LaunchErrorMessage(LaunchError error, const std::string &detail) {
  switch (error) {
    case LaunchError::PlatformUnavailable:
      return "当前平台在目标设备上不可启动。";
    case LaunchError::WrongAdapterKind:
      return "平台启动方式与当前适配器不匹配。";
    case LaunchError::LauncherMissing:
      return "系统启动脚本不存在或不可访问。";
    case LaunchError::RomMissing:
      return "ROM文件不存在或无法读取。";
    case LaunchError::RomOutsideTrustedRoots:
      return "ROM路径不在允许的Roms目录内。";
    case LaunchError::PlatformMismatch:
      return "启动请求的平台与游戏平台不匹配。";
    case LaunchError::LauncherMismatch:
      return "启动请求的适配器与平台配置不匹配。";
    case LaunchError::UnsupportedRequestVersion:
      return "启动请求版本不受支持。";
    case LaunchError::RequestWriteFailed:
      return detail.empty() ? "启动请求写入失败。" : detail;
    case LaunchError::None:
      break;
  }
  return detail.empty() ? "未知启动错误。" : detail;
}

LaunchPreparation PrepareLaunchRequest(UiSession *session, LauncherAdapter *adapter,
                                       const std::string &game_id,
                                       const std::string &target_path) {
  if (!session) {
    return LaunchPreparation{false, "启动失败", "前端会话不可用。"};
  }
  if (!adapter) {
    return LaunchPreparation{false, "启动失败", "当前运行模式没有配置启动适配器。"};
  }
  for (const Game &game : session->library.games) {
    if (game.id != game_id) continue;
    const Platform *platform = FindPlatform(session->library, game.platform_id);
    if (!platform) {
      return LaunchPreparation{false, "启动失败", "找不到游戏对应的平台配置。"};
    }
    const std::string launch_path = target_path.empty() ? game.primary_target.path : target_path;
    const LaunchResult launch =
        adapter->PrepareLaunch(*platform, game, launch_path);
    if (!launch.ok) {
      const std::string message = LaunchErrorMessage(launch.error, launch.message);
      std::cerr << "launch failed: game=" << game.id
                << " error=" << static_cast<int>(launch.error)
                << " message=" << launch.message << '\n';
      return LaunchPreparation{false, "启动失败", message};
    }
    std::cerr << "launch request ready game=" << game.id
              << " platform=" << game.platform_id << '\n';
    return LaunchPreparation{true, "", ""};
  }
  std::cerr << "launch failed: game not found id=" << game_id << '\n';
  return LaunchPreparation{false, "启动失败", "找不到选中的游戏。"};
}

void ApplyResult(UiSession *session, const UiActionResult &action_result,
                 const DesktopUiOptions &options, DesktopUiResult *result, bool *running,
                 std::string *exit_reason, int *return_code) {
  if (action_result.intent == UiIntent::None) return;
  if (result) {
    result->last_intent = action_result.intent;
    result->last_game_id = action_result.game_id;
  }
  if (action_result.intent == UiIntent::LaunchGame) {
    const LaunchPreparation preparation =
        PrepareLaunchRequest(session, options.launch_adapter, action_result.game_id,
                             action_result.target_path);
    if (preparation.ok) {
      MarkLaunched(session, action_result.game_id);
      if (exit_reason) *exit_reason = "launch_game";
      if (return_code) *return_code = options.successful_launch_exit_code;
      if (running) *running = false;
    } else if (session) {
      session->notice.visible = true;
      session->notice.title = preparation.title;
      session->notice.message = preparation.message;
      if (result) {
        result->last_notice_title = preparation.title;
        result->last_notice_message = preparation.message;
      }
    }
  } else if (action_result.intent == UiIntent::Back && running) {
    if (exit_reason) *exit_reason = "back_action";
    std::cerr << "back action ignored at root view\n";
  } else if (action_result.intent == UiIntent::OpenMenu && running) {
    if (exit_reason) *exit_reason = "menu_open";
  } else if (action_result.intent == UiIntent::ExitFrontend && running) {
    if (exit_reason) *exit_reason = "exit_frontend";
    *running = false;
  } else if (action_result.intent == UiIntent::ReturnToSystem && running) {
    if (exit_reason) *exit_reason = "return_to_system";
    *running = false;
  } else if (action_result.intent == UiIntent::SuspendSystem && running) {
    if (exit_reason) *exit_reason = "suspend_requested";
    if (return_code) *return_code = options.suspend_exit_code;
    *running = false;
  } else if (action_result.intent == UiIntent::AdjustBrightness && running) {
    if (!options.system_service || !session) {
      if (session) SetOsd(session, "亮度服务不可用");
      return;
    }
    SystemStatus status;
    if (options.system_service->ChangeBrightness(action_result.delta, &status)) {
      session->system_status = ToUiStatus(status);
      SetOsd(session, "亮度 " + std::to_string(session->system_status.brightness));
    } else {
      SetOsd(session, "亮度调整失败");
    }
  } else if (action_result.intent == UiIntent::AdjustVolume && running) {
    if (!options.system_service || !session) {
      if (session) SetOsd(session, "音量服务不可用");
      return;
    }
    SystemStatus status;
    if (options.system_service->ChangeVolume(action_result.delta, &status)) {
      session->system_status = ToUiStatus(status);
      SetOsd(session, "音量 " + std::to_string(session->system_status.volume));
    } else {
      SetOsd(session, "音量调整失败");
    }
  } else if (action_result.intent == UiIntent::SetAutostart && running) {
    if (!options.system_service || !session) {
      if (session) SetOsd(session, "自启动服务不可用");
      return;
    }
    SystemStatus status;
    const bool enabled = action_result.delta != 0;
    if (options.system_service->SetAutostart(enabled, &status)) {
      session->system_status = ToUiStatus(status);
      SetOsd(session, enabled ? "开机自动进入 已开启" : "开机自动进入 已关闭");
    } else {
      SetOsd(session, "自启动设置失败");
    }
  } else if (action_result.intent == UiIntent::ClearCacheAndRescan && running) {
    if (!session || !options.clear_scan_cache) {
      if (session) SetOsd(session, "重新扫描不可用");
      return;
    }
    if (!options.clear_scan_cache()) {
      SetOsd(session, "清空缓存失败");
      return;
    }
    ClearRendererMediaCache();
    if (exit_reason) *exit_reason = "clear_cache_rescan_requested";
    if (return_code) *return_code = options.clear_cache_rescan_exit_code;
    *running = false;
  } else if (action_result.intent == UiIntent::RestartSystem && running) {
    if (exit_reason) *exit_reason = "restart_requested";
    if (return_code) *return_code = options.restart_exit_code;
    *running = false;
  } else if (action_result.intent == UiIntent::ShutdownSystem && running) {
    if (exit_reason) *exit_reason = "shutdown_requested";
    if (return_code) *return_code = options.shutdown_exit_code;
    *running = false;
  }
}

}  // namespace

int RunDesktopUiApp(const DesktopUiOptions &options, DesktopUiResult *result) {
  const auto app_start = Clock::now();
  if (result) *result = DesktopUiResult{};
  if (options.width <= 0 || options.height <= 0) return 2;

  const Uint32 init_flags = options.render_target == DesktopRenderTarget::Surface
                                ? 0
                                : (SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                                   SDL_INIT_GAMECONTROLLER);
  const auto sdl_start = Clock::now();
  if (SDL_Init(init_flags) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return 3;
  }
  std::cerr << "[perf] ui.sdl_init ms=" << ElapsedMs(sdl_start)
            << " flags=" << init_flags << '\n';

  SdlRendererHandle handle;
  const auto renderer_start = Clock::now();
  if (!CreateRenderer(options, &handle)) {
    std::cerr << "SDL renderer creation failed: " << SDL_GetError() << '\n';
    SDL_Quit();
    return 4;
  }
  std::cerr << "[perf] ui.renderer_create ms=" << ElapsedMs(renderer_start)
            << " target="
            << (options.render_target == DesktopRenderTarget::Surface ? "surface" : "window")
            << '\n';
  SetRendererMediaRoots(options.app_dir, options.state_dir);
  SetRendererAudioDeviceOpenedCallback(options.audio_device_opened_callback);

  Library library = options.library.games.empty() && options.library.platforms.empty()
                        ? BuildDemoLibrary()
                        : options.library;
  GameStateStore game_state_store(options.game_state_path);
  const auto game_state_start = Clock::now();
  game_state_store.Load(&library.games);
  std::cerr << "[perf] ui.game_state_load ms=" << ElapsedMs(game_state_start)
            << " games=" << library.games.size() << '\n';

  UiState ui_state;
  UiStateStore ui_state_store(options.ui_state_path);
  const auto ui_state_start = Clock::now();
  if (!ui_state_store.Load(&ui_state)) {
    ui_state.active_platform_id = "all";
  }
  std::cerr << "[perf] ui.ui_state_load ms=" << ElapsedMs(ui_state_start)
            << '\n';

  const auto session_start = Clock::now();
  UiSession session = CreateUiSession(std::move(library), ui_state,
                                      options.include_empty_platforms);
  std::cerr << "[perf] ui.session_create ms=" << ElapsedMs(session_start)
            << " games=" << session.library.games.size()
            << " platforms=" << session.library.platforms.size() << '\n';
  const auto status_start = Clock::now();
  RefreshSystemStatus(&session, options.system_service);
  std::cerr << "[perf] ui.system_status_initial ms=" << ElapsedMs(status_start)
            << '\n';

  bool running = true;
  std::string exit_reason = "unknown";
  int return_code = 0;
  SdlInputRouter input_router;
  std::size_t scripted_index = 0;
  int frames = 0;
  int hall_state = options.system_service ? options.system_service->ReadHallState() : -1;
  bool first_frame_logged = false;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (IsSdlQuitEvent(event)) {
        exit_reason = "sdl_quit";
        running = false;
        break;
      }
      UiAction action;
      if (input_router.Translate(event, &action)) {
        const UiLayout layout = ResolveUiLayout(options.width, options.height);
        ApplyResult(&session, ApplyUiAction(&session, action, layout), options, result,
                    &running, &exit_reason, &return_code);
      }
    }

    UiAction device_action;
    if (running && input_router.PollDeviceAction(&device_action)) {
      const UiLayout layout = ResolveUiLayout(options.width, options.height);
      ApplyResult(&session, ApplyUiAction(&session, device_action, layout), options, result,
                  &running, &exit_reason, &return_code);
    }

    if (scripted_index < options.scripted_actions.size()) {
      const UiLayout layout = ResolveUiLayout(options.width, options.height);
      ApplyResult(&session,
                  ApplyUiAction(&session, options.scripted_actions[scripted_index], layout),
                  options, result, &running, &exit_reason, &return_code);
      ++scripted_index;
    }

    if (!running) break;
    const int hall_poll_interval = std::max(1, options.hall_poll_interval_frames);
    if (options.system_service && frames % hall_poll_interval == 0 &&
        PollHallSuspend(options.system_service, &hall_state)) {
      exit_reason = "hall_suspend_requested";
      return_code = options.automatic_suspend_exit_code;
      running = false;
      break;
    }
    if (session.osd_frames_remaining > 0) --session.osd_frames_remaining;
    if (options.system_service && frames % 60 == 0) {
      RefreshSystemStatus(&session, options.system_service);
    }
    int render_width = options.width;
    int render_height = options.height;
    if (handle.window) SDL_GetWindowSize(handle.window, &render_width, &render_height);
    const auto render_start = Clock::now();
    RenderLibraryView(handle.renderer, session, ResolveUiLayout(render_width, render_height));
    const long long render_ms = ElapsedMs(render_start);
    if (!first_frame_logged) {
      std::cerr << "[perf] ui.first_frame ms=" << ElapsedMs(app_start)
                << " render_ms=" << render_ms << '\n';
      first_frame_logged = true;
    }
    ++frames;
    if (result) result->frames_rendered = frames;

    if (options.max_frames > 0 && frames >= options.max_frames) {
      exit_reason = "max_frames";
      break;
    }
    if (options.max_frames == 0) SDL_Delay(16);
  }

  game_state_store.LimitRecent(&session.library.games, 20);
  const bool game_saved = game_state_store.Save(session.library.games);
  const bool ui_saved = ui_state_store.Save(ExportUiState(session));
  input_router.Close();
  if (result) {
    result->game_state_saved = game_saved;
    result->ui_state_saved = ui_saved;
  }
  if (!game_saved || !ui_saved) exit_reason = "state_save_failed";
  std::cerr << "desktop ui exit reason=" << exit_reason
            << " frames=" << frames
            << " game_saved=" << (game_saved ? 1 : 0)
            << " ui_saved=" << (ui_saved ? 1 : 0) << '\n';
  std::cerr << "[perf] ui.total ms=" << ElapsedMs(app_start)
            << " frames=" << frames
            << " rc=" << (game_saved && ui_saved ? return_code : 5) << '\n';

  DestroySdlRendererHandle(&handle);
  SetRendererAudioDeviceOpenedCallback({});
  SDL_Quit();
  return game_saved && ui_saved ? return_code : 5;
}

}  // namespace mpl

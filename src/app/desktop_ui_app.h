#pragma once

#include <functional>
#include <string>
#include <vector>

#include "domain/library.h"
#include "devices/system_service.h"
#include "launch/launcher_adapter.h"
#include "ui/ui_model.h"

namespace mpl {

enum class DesktopRenderTarget {
  Window,
  Surface,
};

struct DesktopUiOptions {
  int width = 720;
  int height = 480;
  int max_frames = 0;
  bool hidden_window = false;
  bool include_empty_platforms = false;
  DesktopRenderTarget render_target = DesktopRenderTarget::Window;
  std::string ui_state_path = "build/desktop-demo-state/ui.state";
  std::string game_state_path = "build/desktop-demo-state/game.state";
  std::string state_dir = "build/desktop-demo-state";
  std::string app_dir = ".";
  Library library;
  LauncherAdapter *launch_adapter = nullptr;
  SystemService *system_service = nullptr;
  std::function<bool()> clear_scan_cache;
  int successful_launch_exit_code = 20;
  int suspend_exit_code = 21;
  int automatic_suspend_exit_code = 22;
  int hall_poll_interval_frames = 15;
  int restart_exit_code = 30;
  int shutdown_exit_code = 31;
  int clear_cache_rescan_exit_code = 32;
  std::vector<UiAction> scripted_actions;
};

struct DesktopUiResult {
  int frames_rendered = 0;
  UiIntent last_intent = UiIntent::None;
  std::string last_game_id;
  std::string last_notice_title;
  std::string last_notice_message;
  bool ui_state_saved = false;
  bool game_state_saved = false;
};

int RunDesktopUiApp(const DesktopUiOptions &options, DesktopUiResult *result = nullptr);

}  // namespace mpl

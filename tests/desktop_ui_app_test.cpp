#include "app/desktop_ui_app.h"
#include "devices/h700/retroarch_adapter.h"
#include "devices/system_service.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace mpl;
namespace fs = std::filesystem;

namespace {

bool Contains(const fs::path &path, const std::string &needle) {
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str().find(needle) != std::string::npos;
}

class FakeSystemService : public SystemService {
 public:
  SystemStatus ReadStatus() const override { return status; }

  int ReadHallState() const override {
    if (hall_values.empty()) return -1;
    const std::size_t index =
        hall_reads < static_cast<int>(hall_values.size())
            ? static_cast<std::size_t>(hall_reads)
            : hall_values.size() - 1;
    ++hall_reads;
    return hall_values[index];
  }

  bool ChangeBrightness(int delta, SystemStatus *out) override {
    status.brightness += delta;
    if (out) *out = status;
    return true;
  }

  bool ChangeVolume(int delta, SystemStatus *out) override {
    status.volume += delta;
    if (out) *out = status;
    return true;
  }

  bool SetAutostart(bool enabled, SystemStatus *out) override {
    status.autostart_enabled = enabled ? 1 : 0;
    if (out) *out = status;
    return true;
  }

  SystemStatus status{64, true, 5, 6, 0};
  mutable int hall_reads = 0;
  std::vector<int> hall_values;
};

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "multiplatform_launcher_desktop_ui_test";
  fs::remove_all(root);
  fs::create_directories(root);

  DesktopUiOptions options;
  options.width = 720;
  options.height = 480;
  options.max_frames = 6;
  options.render_target = DesktopRenderTarget::Surface;
  options.ui_state_path = (root / "ui.state").u8string();
  options.game_state_path = (root / "game.state").u8string();
  options.scripted_actions = {
      UiAction::TabNext,
      UiAction::TabNext,
      UiAction::TabNext,
      UiAction::Confirm,
      UiAction::ToggleFavorite,
  };

  DesktopUiResult result;
  const int rc = RunDesktopUiApp(options, &result);
  assert(rc == 0);
  assert(result.frames_rendered == 6);
  assert(result.last_intent == UiIntent::LaunchGame);
  assert(result.last_game_id == "gba-castlevania");
  assert(result.ui_state_saved);
  assert(result.game_state_saved);
  assert(fs::exists(root / "ui.state"));
  assert(fs::exists(root / "game.state"));
  assert(Contains(root / "ui.state", "active_platform_id=gba"));
  assert(Contains(root / "game.state", result.last_game_id));

  DesktopUiOptions menu_options;
  menu_options.width = 720;
  menu_options.height = 480;
  menu_options.max_frames = 6;
  menu_options.render_target = DesktopRenderTarget::Surface;
  menu_options.ui_state_path = (root / "menu-ui.state").u8string();
  menu_options.game_state_path = (root / "menu-game.state").u8string();
  menu_options.scripted_actions = {UiAction::Menu};

  DesktopUiResult menu_result;
  const int menu_rc = RunDesktopUiApp(menu_options, &menu_result);
  assert(menu_rc == 0);
  assert(menu_result.frames_rendered == 6);
  assert(menu_result.ui_state_saved);
  assert(menu_result.game_state_saved);
  assert(Contains(root / "menu-ui.state", "focus=grid"));

  DesktopUiOptions exit_options;
  exit_options.width = 720;
  exit_options.height = 480;
  exit_options.max_frames = 18;
  exit_options.render_target = DesktopRenderTarget::Surface;
  exit_options.ui_state_path = (root / "exit-ui.state").u8string();
  exit_options.game_state_path = (root / "exit-game.state").u8string();
  exit_options.scripted_actions = {
      UiAction::Menu,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Confirm,
  };

  DesktopUiResult exit_result;
  const int exit_rc = RunDesktopUiApp(exit_options, &exit_result);
  assert(exit_rc == 31);
  assert(exit_result.frames_rendered == 16);
  assert(exit_result.last_intent == UiIntent::ShutdownSystem);

  DesktopUiOptions suspend_options;
  suspend_options.width = 720;
  suspend_options.height = 480;
  suspend_options.max_frames = 6;
  suspend_options.render_target = DesktopRenderTarget::Surface;
  suspend_options.ui_state_path = (root / "suspend-ui.state").u8string();
  suspend_options.game_state_path = (root / "suspend-game.state").u8string();
  suspend_options.scripted_actions = {UiAction::Power};

  DesktopUiResult suspend_result;
  const int suspend_rc = RunDesktopUiApp(suspend_options, &suspend_result);
  assert(suspend_rc == 21);
  assert(suspend_result.frames_rendered == 0);
  assert(suspend_result.last_intent == UiIntent::SuspendSystem);

  FakeSystemService hall_service;
  hall_service.hall_values = {1, 0};
  DesktopUiOptions hall_options;
  hall_options.width = 720;
  hall_options.height = 480;
  hall_options.max_frames = 6;
  hall_options.render_target = DesktopRenderTarget::Surface;
  hall_options.ui_state_path = (root / "hall-ui.state").u8string();
  hall_options.game_state_path = (root / "hall-game.state").u8string();
  hall_options.system_service = &hall_service;
  hall_options.hall_poll_interval_frames = 1;

  DesktopUiResult hall_result;
  const int hall_rc = RunDesktopUiApp(hall_options, &hall_result);
  assert(hall_rc == 22);
  assert(hall_result.frames_rendered == 0);

  FakeSystemService fake_service;
  DesktopUiOptions system_options;
  system_options.width = 720;
  system_options.height = 480;
  system_options.max_frames = 9;
  system_options.render_target = DesktopRenderTarget::Surface;
  system_options.ui_state_path = (root / "system-ui.state").u8string();
  system_options.game_state_path = (root / "system-game.state").u8string();
  system_options.system_service = &fake_service;
  system_options.scripted_actions = {
      UiAction::Menu,
      UiAction::Down,
      UiAction::Down,
      UiAction::Right,
      UiAction::Down,
      UiAction::Right,
      UiAction::Down,
      UiAction::Left,
  };
  DesktopUiResult system_result;
  const int system_rc = RunDesktopUiApp(system_options, &system_result);
  assert(system_rc == 0);
  assert(fake_service.status.autostart_enabled == 1);
  assert(fake_service.status.brightness == 6);
  assert(fake_service.status.volume == 5);

  bool rescan_called = false;
  DesktopUiOptions rescan_options;
  rescan_options.width = 720;
  rescan_options.height = 480;
  rescan_options.max_frames = 16;
  rescan_options.render_target = DesktopRenderTarget::Surface;
  rescan_options.ui_state_path = (root / "rescan-ui.state").u8string();
  rescan_options.game_state_path = (root / "rescan-game.state").u8string();
  rescan_options.clear_scan_cache = [&]() {
    rescan_called = true;
    return true;
  };
  rescan_options.scripted_actions = {
      UiAction::Menu,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Down,
      UiAction::Confirm,
  };
  DesktopUiResult rescan_result;
  const int rescan_rc = RunDesktopUiApp(rescan_options, &rescan_result);
  assert(rescan_rc == rescan_options.clear_cache_rescan_exit_code);
  assert(rescan_called);
  assert(rescan_result.last_intent == UiIntent::ClearCacheAndRescan);

  const fs::path launch_root = root / "launch-root";
  const fs::path rom_root = launch_root / "Roms";
  const fs::path gba_root = rom_root / "GBA";
  fs::create_directories(gba_root);
  const fs::path rom = gba_root / "sample.gba";
  {
    std::ofstream out(rom);
    out << "rom";
  }
  const fs::path launcher = launch_root / "RA_launch.sh";
  {
    std::ofstream out(launcher);
    out << "#!/bin/sh\nexit 0\n";
  }

  Platform platform;
  platform.id = "gba";
  platform.display_name = "GBA";
  platform.rom_directories = {gba_root.u8string()};
  platform.launcher_id = "h700-retroarch-gba";
  platform.launchable = true;

  Game game;
  game.id = "gba-sample";
  game.platform_id = "gba";
  game.title = "Sample";
  game.primary_target.path = rom.u8string();

  H700RetroArchAdapterOptions launch_options;
  launch_options.request_path = (root / "state" / "launch.request").u8string();
  launch_options.retroarch_launcher = launcher.u8string();
  launch_options.trusted_roots = {rom_root.u8string()};
  H700RetroArchAdapter launch_adapter(launch_options);

  DesktopUiOptions launch_ui_options;
  launch_ui_options.width = 720;
  launch_ui_options.height = 480;
  launch_ui_options.max_frames = 6;
  launch_ui_options.render_target = DesktopRenderTarget::Surface;
  launch_ui_options.ui_state_path = (root / "launch-ui.state").u8string();
  launch_ui_options.game_state_path = (root / "launch-game.state").u8string();
  launch_ui_options.library.platforms = {platform};
  launch_ui_options.library.games = {game};
  launch_ui_options.launch_adapter = &launch_adapter;
  launch_ui_options.scripted_actions = {UiAction::Confirm};

  DesktopUiResult launch_result;
  const int launch_rc = RunDesktopUiApp(launch_ui_options, &launch_result);
  assert(launch_rc == 20);
  assert(launch_result.frames_rendered == 0);
  assert(launch_result.last_intent == UiIntent::LaunchGame);
  assert(fs::exists(root / "state" / "launch.request"));
  assert(Contains(root / "state" / "launch.request", "platform_id=gba"));
  assert(Contains(root / "state" / "launch.request", "rom_path=" + rom.u8string()));

  const fs::path rom_alt = gba_root / "sample-alt.gba";
  {
    std::ofstream out(rom_alt);
    out << "rom-alt";
  }
  game.alternate_targets = {LaunchTarget{rom_alt.u8string(), "Alt"}};

  H700RetroArchAdapterOptions alternate_launch_options;
  alternate_launch_options.request_path = (root / "state-alt" / "launch.request").u8string();
  alternate_launch_options.retroarch_launcher = launcher.u8string();
  alternate_launch_options.trusted_roots = {rom_root.u8string()};
  H700RetroArchAdapter alternate_launch_adapter(alternate_launch_options);

  DesktopUiOptions alternate_ui_options;
  alternate_ui_options.width = 720;
  alternate_ui_options.height = 480;
  alternate_ui_options.max_frames = 8;
  alternate_ui_options.render_target = DesktopRenderTarget::Surface;
  alternate_ui_options.ui_state_path = (root / "alternate-ui.state").u8string();
  alternate_ui_options.game_state_path = (root / "alternate-game.state").u8string();
  alternate_ui_options.library.platforms = {platform};
  alternate_ui_options.library.games = {game};
  alternate_ui_options.launch_adapter = &alternate_launch_adapter;
  alternate_ui_options.scripted_actions = {
      UiAction::Confirm,
      UiAction::Down,
      UiAction::Confirm,
  };

  DesktopUiResult alternate_result;
  const int alternate_rc = RunDesktopUiApp(alternate_ui_options, &alternate_result);
  assert(alternate_rc == 20);
  assert(alternate_result.frames_rendered == 2);
  assert(alternate_result.last_intent == UiIntent::LaunchGame);
  assert(fs::exists(root / "state-alt" / "launch.request"));
  assert(Contains(root / "state-alt" / "launch.request", "rom_path=" + rom_alt.u8string()));

  fs::remove(rom_alt);
  H700RetroArchAdapterOptions failed_launch_options;
  failed_launch_options.request_path = (root / "state-failed" / "launch.request").u8string();
  failed_launch_options.retroarch_launcher = launcher.u8string();
  failed_launch_options.trusted_roots = {rom_root.u8string()};
  H700RetroArchAdapter failed_launch_adapter(failed_launch_options);

  DesktopUiOptions failed_ui_options;
  failed_ui_options.width = 720;
  failed_ui_options.height = 480;
  failed_ui_options.max_frames = 6;
  failed_ui_options.render_target = DesktopRenderTarget::Surface;
  failed_ui_options.ui_state_path = (root / "failed-ui.state").u8string();
  failed_ui_options.game_state_path = (root / "failed-game.state").u8string();
  failed_ui_options.library.platforms = {platform};
  failed_ui_options.library.games = {game};
  failed_ui_options.launch_adapter = &failed_launch_adapter;
  failed_ui_options.scripted_actions = {
      UiAction::Confirm,
      UiAction::Down,
      UiAction::Confirm,
      UiAction::Back,
  };

  DesktopUiResult failed_result;
  const int failed_rc = RunDesktopUiApp(failed_ui_options, &failed_result);
  assert(failed_rc == 0);
  assert(failed_result.frames_rendered == 6);
  assert(failed_result.last_intent == UiIntent::LaunchGame);
  assert(failed_result.last_notice_title == "启动失败");
  assert(failed_result.last_notice_message.find("ROM") != std::string::npos);
  assert(!fs::exists(root / "state-failed" / "launch.request"));
  assert(!Contains(root / "failed-game.state", "gba-sample"));

  fs::remove_all(root);
  return 0;
}

#include "devices/h700/system_service.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace mpl {

namespace {

constexpr const char *kBatteryRoot = "/sys/class/power_supply/axp2202-battery";
constexpr const char *kSystemVolumePath = "/sys/class/power_supply/axp2202-battery/openbor_volume";

std::string ReadText(const std::string &path) {
  std::ifstream input(path);
  std::string value;
  std::getline(input, value);
  return value;
}

int MixerValueForLevel(int level) {
  const int clamped = std::clamp(level, 0, 10);
  return clamped == 0 ? 0 : (clamped * 31 + 5) / 10;
}

bool RunAmixerSet(const char *control, const std::string &value) {
  std::ostringstream command;
  command << "unset LD_PRELOAD LD_LIBRARY_PATH; amixer -q -c 0 set '"
          << control << "' " << value << " >/dev/null 2>&1";
  return std::system(command.str().c_str()) == 0;
}

std::string ShellQuote(const std::string &value) {
  std::string quoted = "'";
  for (char ch : value) {
    if (ch == '\'') quoted += "'\\''";
    else quoted += ch;
  }
  quoted += "'";
  return quoted;
}

}  // namespace

H700SystemService::H700SystemService(H700SystemServiceOptions options)
    : options_(std::move(options)) {}

std::string H700SystemService::SystemPath(const std::string &absolute_path) const {
  if (options_.root_prefix.empty()) return absolute_path;
  if (absolute_path.empty() || absolute_path.front() != '/') return absolute_path;
  return (fs::u8path(options_.root_prefix) / absolute_path.substr(1)).u8string();
}

std::string H700SystemService::AppPath(const std::string &relative_path) const {
  if (options_.app_dir.empty()) return relative_path;
  return (fs::u8path(options_.app_dir) / relative_path).u8string();
}

int H700SystemService::ReadInt(const std::string &path, int fallback) const {
  std::ifstream input(path);
  int value = fallback;
  return input >> value ? value : fallback;
}

bool H700SystemService::WriteInt(const std::string &path, int value) const {
  std::error_code error;
  fs::create_directories(fs::u8path(path).parent_path(), error);
  std::ofstream output(path, std::ios::trunc);
  output << value;
  return static_cast<bool>(output);
}

bool H700SystemService::AutostartHelperAvailable() const {
  std::error_code error;
  return fs::is_regular_file(fs::u8path(AppPath("autostart_ctl.sh")), error);
}

bool H700SystemService::AutostartEnabled() const {
  std::error_code error;
  return fs::is_regular_file(fs::u8path(options_.state_dir) / "autostart.enabled", error);
}

SystemStatus H700SystemService::ReadStatus() const {
  SystemStatus status;
  const std::string battery_root = SystemPath(kBatteryRoot);
  const std::string volume_path = SystemPath(kSystemVolumePath);
  status.battery_percent = ReadInt(battery_root + "/capacity", -1);
  const std::string battery_state = ReadText(battery_root + "/status");
  status.charging = battery_state == "Charging" || battery_state == "Full";
  status.brightness = ReadInt((fs::u8path(options_.state_dir) / "brightness.level").u8string(),
                              ReadInt(battery_root + "/brightness", -1));
  status.volume = ReadInt(volume_path,
                          ReadInt((fs::u8path(options_.state_dir) / "volume.level").u8string(),
                                  -1));
  if (AutostartHelperAvailable() || AutostartEnabled()) {
    status.autostart_enabled = AutostartEnabled() ? 1 : 0;
  }
  return status;
}

int H700SystemService::ReadHallState() const {
  return ReadInt(SystemPath(std::string(kBatteryRoot) + "/hallkey"), -1);
}

bool H700SystemService::ApplyBrightness(int level) const {
  if (!options_.allow_runtime_writes) return true;
  if (!options_.root_prefix.empty()) return true;
#ifdef _WIN32
  (void)level;
  return true;
#else
  constexpr unsigned long kPanelBrightness[10] = {
      5, 10, 20, 35, 50, 70, 100, 140, 200, 255,
  };
  const int display = open("/dev/disp", O_RDWR | O_CLOEXEC);
  if (display < 0) return false;
  unsigned long arguments[4] = {};
  arguments[1] = kPanelBrightness[std::clamp(level, 1, 10) - 1];
  const bool ok = ioctl(display, 0x102, arguments) == 0;
  close(display);
  return ok;
#endif
}

bool H700SystemService::ApplyVolume(int level) const {
  if (!options_.allow_runtime_writes) return true;
  if (!options_.root_prefix.empty()) return true;
  const int clamped = std::clamp(level, 0, 10);
  const bool volume_ok =
      RunAmixerSet("lineout volume", std::to_string(MixerValueForLevel(clamped)));
  if (clamped == 0) {
    RunAmixerSet("SPK", "off");
  } else {
    RunAmixerSet("digital volume", "63");
    RunAmixerSet("LINEOUT", "on");
    RunAmixerSet("SPK", "on");
  }
  return volume_ok;
}

bool H700SystemService::ChangeBrightness(int delta, SystemStatus *status) {
  const std::string battery_root = SystemPath(kBatteryRoot);
  const std::string system_path = battery_root + "/brightness";
  const std::string state_path = (fs::u8path(options_.state_dir) / "brightness.level").u8string();
  const int current = ReadInt(state_path, ReadInt(system_path, 8));
  const int next = std::clamp(current + delta, 1, 10);
  if (!ApplyBrightness(next)) return false;
  const bool wrote_state = WriteInt(state_path, next);
  bool wrote_system = true;
  if (options_.root_prefix.empty() || fs::exists(fs::u8path(system_path))) {
    wrote_system = WriteInt(system_path, next);
  }
  if (status) {
    *status = ReadStatus();
    status->brightness = next;
  }
  return wrote_state && wrote_system;
}

bool H700SystemService::ChangeVolume(int delta, SystemStatus *status) {
  const std::string state_path = (fs::u8path(options_.state_dir) / "volume.level").u8string();
  const std::string system_path = SystemPath(kSystemVolumePath);
  const int current = ReadInt(system_path, ReadInt(state_path, 6));
  const int next = std::clamp(current + delta, 0, 10);
  if (!ApplyVolume(next)) return false;
  const bool wrote_state = WriteInt(state_path, next);
  bool wrote_system = true;
  if (options_.root_prefix.empty() || fs::exists(fs::u8path(system_path))) {
    wrote_system = WriteInt(system_path, next);
  }
  if (status) {
    *status = ReadStatus();
    status->volume = next;
  }
  return wrote_state && wrote_system;
}

bool H700SystemService::SetAutostart(bool enabled, SystemStatus *status) {
  const std::string helper = AppPath("autostart_ctl.sh");
  std::error_code error;
  if (!fs::is_regular_file(fs::u8path(helper), error)) return false;
  if (!options_.allow_runtime_writes) {
    if (status) {
      *status = ReadStatus();
      status->autostart_enabled = enabled ? 1 : 0;
    }
    return true;
  }
  if (!options_.root_prefix.empty()) return false;
  std::ostringstream command;
  command << ShellQuote(helper) << " " << (enabled ? "enable" : "disable");
  if (std::system(command.str().c_str()) != 0) return false;
  if (status) *status = ReadStatus();
  return true;
}

bool H700SystemService::RestoreVolume() const {
  const std::string state_path = (fs::u8path(options_.state_dir) / "volume.level").u8string();
  const std::string system_path = SystemPath(kSystemVolumePath);
  const int current = std::clamp(ReadInt(system_path, ReadInt(state_path, 6)), 0, 10);
  if (!ApplyVolume(current)) return false;
  const bool wrote_state = WriteInt(state_path, current);
  bool wrote_system = true;
  if (options_.root_prefix.empty() || fs::exists(fs::u8path(system_path))) {
    wrote_system = WriteInt(system_path, current);
  }
  return wrote_state && wrote_system;
}

bool H700SystemService::SyncStoredVolumeToSystem() const {
  const std::string state_path = (fs::u8path(options_.state_dir) / "volume.level").u8string();
  const std::string system_path = SystemPath(kSystemVolumePath);
  const int current = std::clamp(ReadInt(state_path, 6), 0, 10);
  if (!ApplyVolume(current)) return false;
  if (options_.root_prefix.empty() || fs::exists(fs::u8path(system_path))) {
    return WriteInt(system_path, current);
  }
  return true;
}

}  // namespace mpl

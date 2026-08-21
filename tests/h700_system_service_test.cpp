#include "devices/h700/system_service.h"

#include <cassert>
#include <filesystem>
#include <fstream>

using namespace mpl;
namespace fs = std::filesystem;

namespace {

int ReadIntFile(const fs::path &path, int fallback) {
  std::ifstream input(path);
  int value = fallback;
  return input >> value ? value : fallback;
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "mpl_h700_system_service_test";
  fs::remove_all(root);
  const fs::path battery = root / "sys/class/power_supply/axp2202-battery";
  const fs::path state = root / "state";
  const fs::path app = root / "app";
  fs::create_directories(battery);
  fs::create_directories(app);
  std::ofstream(battery / "capacity") << "83\n";
  std::ofstream(battery / "status") << "Charging\n";
  std::ofstream(battery / "brightness") << "4\n";
  std::ofstream(battery / "openbor_volume") << "4\n";
  std::ofstream(app / "autostart_ctl.sh") << "#!/bin/sh\nexit 0\n";

  H700SystemServiceOptions options;
  options.app_dir = app.u8string();
  options.root_prefix = root.u8string();
  options.state_dir = state.u8string();
  options.allow_runtime_writes = false;
  H700SystemService service(options);

  SystemStatus status = service.ReadStatus();
  assert(status.battery_percent == 83);
  assert(status.charging);
  assert(status.brightness == 4);
  assert(status.volume == 4);
  assert(status.autostart_enabled == 0);
  assert(service.SetAutostart(true, &status));
  assert(status.autostart_enabled == 1);

  assert(service.RestoreVolume());
  status = service.ReadStatus();
  assert(status.volume == 4);

  fs::create_directories(state);
  std::ofstream(state / "volume.level") << "3\n";
  std::ofstream(battery / "openbor_volume") << "9\n";
  assert(service.SyncStoredVolumeToSystem());
  assert(ReadIntFile(battery / "openbor_volume", -1) == 3);

  assert(service.ChangeBrightness(2, &status));
  assert(status.brightness == 6);
  assert(service.ChangeBrightness(99, &status));
  assert(status.brightness == 10);
  assert(service.ChangeVolume(-2, &status));
  assert(status.volume == 1);
  assert(ReadIntFile(battery / "openbor_volume", -1) == 1);
  assert(service.ChangeVolume(-99, &status));
  assert(status.volume == 0);
  assert(ReadIntFile(battery / "openbor_volume", -1) == 0);
  assert(service.ChangeVolume(99, &status));
  assert(status.volume == 10);
  assert(ReadIntFile(battery / "openbor_volume", -1) == 10);

  fs::remove_all(root);
  return 0;
}

#pragma once

#include <string>
#include <vector>

#include "launch/launcher_adapter.h"

namespace mpl {

struct H700RetroArchAdapterOptions {
  std::string request_path;
  std::string retroarch_launcher = "/mnt/mod/ctrl/RA_launch.sh";
  std::vector<std::string> trusted_roots;
};

class H700RetroArchAdapter : public LauncherAdapter {
 public:
  explicit H700RetroArchAdapter(H700RetroArchAdapterOptions options);

  LaunchCapability Probe(const Platform &platform) const override;
  LaunchResult PrepareLaunch(const Platform &platform, const Game &game,
                             const std::string &rom_path) const override;

 private:
  H700RetroArchAdapterOptions options_;
};

}  // namespace mpl

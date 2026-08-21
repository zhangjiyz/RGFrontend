#pragma once

#include <string>
#include <vector>

#include "domain/game.h"
#include "domain/platform.h"

namespace mpl {

struct LaunchRequest {
  int request_version = 1;
  std::string platform_id;
  std::string game_id;
  std::string rom_path;
  std::string launcher_id;
  std::string core_hint;
};

struct LaunchValidation {
  bool ok = false;
  std::string message;
};

LaunchRequest BuildLaunchRequest(const Platform &platform, const Game &game,
                                 const std::string &rom_path);
LaunchValidation ValidateLaunchRequest(const LaunchRequest &request,
                                       const Platform &platform,
                                       const std::vector<std::string> &trusted_roots);
bool SaveLaunchRequest(const LaunchRequest &request, const std::string &path);
bool LoadLaunchRequest(const std::string &path, LaunchRequest *request);

}  // namespace mpl

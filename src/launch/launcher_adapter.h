#pragma once

#include <string>
#include <vector>

#include "domain/game.h"
#include "domain/platform.h"
#include "launch/launch_request.h"

namespace mpl {

enum class LaunchError {
  None,
  PlatformUnavailable,
  WrongAdapterKind,
  LauncherMissing,
  RomMissing,
  RomOutsideTrustedRoots,
  PlatformMismatch,
  LauncherMismatch,
  UnsupportedRequestVersion,
  RequestWriteFailed,
};

struct LaunchCapability {
  bool available = false;
  LauncherKind kind = LauncherKind::RetroArch;
  std::string launcher_id;
  LaunchError error = LaunchError::None;
  std::string message;
};

struct LaunchResult {
  bool ok = false;
  LaunchError error = LaunchError::None;
  std::string message;
  LaunchRequest request;
};

class LauncherAdapter {
 public:
  virtual ~LauncherAdapter() = default;

  virtual LaunchCapability Probe(const Platform &platform) const = 0;
  virtual LaunchResult PrepareLaunch(const Platform &platform, const Game &game,
                                     const std::string &rom_path) const = 0;
};

LaunchError LaunchErrorFromValidationMessage(const std::string &message);

}  // namespace mpl

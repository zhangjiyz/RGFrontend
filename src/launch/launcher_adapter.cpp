#include "launch/launcher_adapter.h"

namespace mpl {

LaunchError LaunchErrorFromValidationMessage(const std::string &message) {
  if (message == "unsupported request version") return LaunchError::UnsupportedRequestVersion;
  if (message == "platform mismatch") return LaunchError::PlatformMismatch;
  if (message == "launcher mismatch") return LaunchError::LauncherMismatch;
  if (message == "platform not launchable") return LaunchError::PlatformUnavailable;
  if (message == "ROM missing") return LaunchError::RomMissing;
  if (message == "ROM outside trusted roots") return LaunchError::RomOutsideTrustedRoots;
  return LaunchError::RequestWriteFailed;
}

}  // namespace mpl

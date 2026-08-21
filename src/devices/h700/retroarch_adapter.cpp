#include "devices/h700/retroarch_adapter.h"

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace mpl {

namespace {

bool RegularFileExists(const std::string &path) {
  std::error_code error;
  return fs::is_regular_file(fs::u8path(path), error);
}

}  // namespace

H700RetroArchAdapter::H700RetroArchAdapter(H700RetroArchAdapterOptions options)
    : options_(std::move(options)) {}

LaunchCapability H700RetroArchAdapter::Probe(const Platform &platform) const {
  LaunchCapability capability;
  capability.kind = LauncherKind::RetroArch;
  capability.launcher_id = platform.launcher_id;
  if (platform.launcher_kind != LauncherKind::RetroArch) {
    capability.error = LaunchError::WrongAdapterKind;
    capability.message = "platform requires a standalone adapter";
    return capability;
  }
  if (!RegularFileExists(options_.retroarch_launcher)) {
    capability.error = LaunchError::LauncherMissing;
    capability.message = "RetroArch launcher missing: " + options_.retroarch_launcher;
    return capability;
  }
  if (!platform.launchable) {
    capability.error = LaunchError::PlatformUnavailable;
    capability.message = "platform is not marked launchable";
    return capability;
  }
  capability.available = true;
  return capability;
}

LaunchResult H700RetroArchAdapter::PrepareLaunch(const Platform &platform, const Game &game,
                                                 const std::string &rom_path) const {
  LaunchResult result;
  result.request = BuildLaunchRequest(platform, game, rom_path);
  const LaunchCapability capability = Probe(platform);
  if (!capability.available) {
    result.error = capability.error;
    result.message = capability.message;
    return result;
  }

  std::vector<std::string> trusted_roots = options_.trusted_roots;
  if (trusted_roots.empty()) trusted_roots = platform.rom_directories;
  const LaunchValidation validation = ValidateLaunchRequest(result.request, platform,
                                                            trusted_roots);
  if (!validation.ok) {
    result.error = LaunchErrorFromValidationMessage(validation.message);
    result.message = validation.message;
    return result;
  }

  if (!SaveLaunchRequest(result.request, options_.request_path)) {
    result.error = LaunchError::RequestWriteFailed;
    result.message = "failed to write launch request";
    return result;
  }
  result.ok = true;
  return result;
}

}  // namespace mpl

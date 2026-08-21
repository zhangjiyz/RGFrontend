#include "devices/h700/launch_request_adapter.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace mpl {

namespace {

bool RegularFileExists(const std::string &path) {
  std::error_code error;
  return fs::is_regular_file(fs::u8path(path), error);
}

std::vector<std::string> RequiredLauncherPaths(const H700LaunchRequestAdapterOptions &options,
                                               const Platform &platform) {
  if (platform.launcher_kind == LauncherKind::RetroArch) {
    return {options.retroarch_launcher};
  }
  if (platform.id == "nds") {
    return {options.nds_launcher};
  }
  if (platform.id == "psp") {
    return {options.psp_launcher};
  }
  if (platform.id == "openbor") {
    return {options.openbor_setup_script, options.openbor_launcher};
  }
  if (platform.id == "ports") {
    return {options.ports_shell};
  }
  if (platform.id == "java") {
    return {options.java_launcher};
  }
  if (platform.id == "saturn") {
    return {options.saturn_launcher};
  }
  return {};
}

}  // namespace

H700LaunchRequestAdapter::H700LaunchRequestAdapter(H700LaunchRequestAdapterOptions options)
    : options_(std::move(options)) {}

LaunchCapability H700LaunchRequestAdapter::Probe(const Platform &platform) const {
  LaunchCapability capability;
  capability.kind = platform.launcher_kind;
  capability.launcher_id = platform.launcher_id;
  const std::vector<std::string> launcher_paths = RequiredLauncherPaths(options_, platform);
  if (launcher_paths.empty()) {
    capability.error = LaunchError::WrongAdapterKind;
    capability.message = "unsupported H700 launcher kind";
    return capability;
  }
  for (const std::string &launcher_path : launcher_paths) {
    if (!RegularFileExists(launcher_path)) {
      capability.error = LaunchError::LauncherMissing;
      capability.message = "H700 launcher missing: " + launcher_path;
      return capability;
    }
  }
  if (!platform.launchable) {
    capability.error = LaunchError::PlatformUnavailable;
    capability.message = "platform is not marked launchable";
    return capability;
  }
  capability.available = true;
  return capability;
}

LaunchResult H700LaunchRequestAdapter::PrepareLaunch(const Platform &platform,
                                                     const Game &game,
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

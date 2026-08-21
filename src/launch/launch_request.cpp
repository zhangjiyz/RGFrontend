#include "launch/launch_request.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "catalog/path.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

std::string OneLine(std::string value) {
  value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
  value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
  return value;
}

void AssignField(const std::string &key, const std::string &value, LaunchRequest *request) {
  if (key == "request_version") request->request_version = value == "1" ? 1 : 0;
  else if (key == "platform_id") request->platform_id = value;
  else if (key == "game_id") request->game_id = value;
  else if (key == "rom_path") request->rom_path = value;
  else if (key == "launcher_id") request->launcher_id = value;
  else if (key == "core_hint") request->core_hint = value;
}

}  // namespace

LaunchRequest BuildLaunchRequest(const Platform &platform, const Game &game,
                                 const std::string &rom_path) {
  LaunchRequest request;
  request.platform_id = platform.id;
  request.game_id = game.id;
  request.rom_path = rom_path;
  request.launcher_id = platform.launcher_id;
  request.core_hint = game.user_core_hint.empty() ? game.launch_hint.core_hint
                                                  : game.user_core_hint;
  return request;
}

LaunchValidation ValidateLaunchRequest(const LaunchRequest &request,
                                       const Platform &platform,
                                       const std::vector<std::string> &trusted_roots) {
  if (request.request_version != 1) return {false, "unsupported request version"};
  if (request.platform_id != platform.id) return {false, "platform mismatch"};
  if (request.launcher_id != platform.launcher_id) return {false, "launcher mismatch"};
  if (!platform.launchable) return {false, "platform not launchable"};
  std::error_code error;
  if (!fs::is_regular_file(fs::u8path(request.rom_path), error)) return {false, "ROM missing"};
  for (const std::string &root : trusted_roots) {
    if (PathIsInside(fs::u8path(request.rom_path), fs::u8path(root))) return {true, {}};
  }
  return {false, "ROM outside trusted roots"};
}

bool SaveLaunchRequest(const LaunchRequest &request, const std::string &path_text) {
  const fs::path path = fs::u8path(path_text);
  std::error_code error;
  fs::create_directories(path.parent_path(), error);
  const fs::path temporary = fs::u8path(path_text + ".tmp");
  {
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) return false;
    out << "request_version=" << request.request_version << '\n';
    out << "platform_id=" << OneLine(request.platform_id) << '\n';
    out << "game_id=" << OneLine(request.game_id) << '\n';
    out << "rom_path=" << OneLine(request.rom_path) << '\n';
    out << "launcher_id=" << OneLine(request.launcher_id) << '\n';
    if (!request.core_hint.empty()) {
      out << "core_hint=" << OneLine(request.core_hint) << '\n';
    }
    if (!out) return false;
  }
  fs::rename(temporary, path, error);
  if (!error) return true;
  fs::remove(path, error);
  error.clear();
  fs::rename(temporary, path, error);
  return !error;
}

bool LoadLaunchRequest(const std::string &path, LaunchRequest *request) {
  if (!request) return false;
  std::ifstream in(fs::u8path(path));
  if (!in) return false;
  LaunchRequest parsed;
  std::string line;
  while (std::getline(in, line)) {
    const size_t equals = line.find('=');
    if (equals == std::string::npos) continue;
    AssignField(line.substr(0, equals), line.substr(equals + 1), &parsed);
  }
  *request = std::move(parsed);
  return true;
}

}  // namespace mpl

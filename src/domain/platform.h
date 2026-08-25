#pragma once

#include <string>
#include <vector>

namespace mpl {

enum class LauncherKind {
  RetroArch,
  Standalone,
};

struct MediaRule {
  std::vector<std::string> cover_names;
  std::vector<std::string> logo_names;
  std::vector<std::string> video_names;
};

struct Platform {
  std::string id;
  std::string display_name;
  std::vector<std::string> rom_directories;
  std::vector<std::string> directory_aliases;
  std::vector<std::string> platform_aliases;
  std::vector<std::string> extensions;
  std::string arcade_name_database_path;
  MediaRule media_rule;
  std::string launcher_id;
  LauncherKind launcher_kind = LauncherKind::RetroArch;
  int sort_order = 0;
  bool launchable = false;
  std::vector<std::string> diagnostics;
};

}  // namespace mpl

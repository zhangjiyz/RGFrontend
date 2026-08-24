#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mpl {

struct MediaPaths {
  std::string cover;
  std::string logo;
  std::string video;
};

struct LaunchTarget {
  std::string path;
  std::string label;
};

struct FileFingerprint {
  std::uintmax_t size = 0;
  std::int64_t modified_time = 0;
  std::string sample_hash;
};

enum class LaunchHintKind {
  Empty,
  AndroidActivity,
  LinuxCommand,
  Unknown,
};

struct LaunchHint {
  LaunchHintKind kind = LaunchHintKind::Empty;
  std::string raw;
  std::string platform_hint;
  std::string launcher_alias;
  std::string core_hint;
  std::string android_package;
  std::string android_activity;
  std::string command;
  std::string diagnostic;
};

struct Game {
  std::string id;
  std::string platform_id;
  std::string collection_id;
  std::string collection_title;
  std::string title;
  std::string display_title;
  std::string developer;
  std::string publisher;
  std::string genre;
  std::string release;
  std::string external_id;
  std::string description;
  std::string sort_key;
  LaunchTarget primary_target;
  std::vector<LaunchTarget> alternate_targets;
  std::vector<std::string> alternate_target_ids;
  MediaPaths media;
  FileFingerprint fingerprint;
  std::string metadata_path;
  std::string source;
  std::string non_executable_launch_hint;
  LaunchHint launch_hint;
  std::string user_core_hint;
  bool multi_file_entry = false;
  bool favorite = false;
  std::uint64_t recent_order = 0;
};

}  // namespace mpl

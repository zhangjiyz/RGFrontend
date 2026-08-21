#include "catalog/providers/pegasus_provider.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include "catalog/launch_hint_resolver.h"
#include "catalog/path.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

struct ParsedGame {
  std::string collection_title;
  std::string title;
  std::string developer;
  std::string publisher;
  std::string genre;
  std::string release;
  std::string external_id;
  std::string description;
  std::string sort_key;
  std::vector<std::string> files;
  std::string cover;
  std::string logo;
  std::string video;
  std::string launch_hint;
};

struct ResolvedPegasusGame {
  fs::path metadata;
  fs::path base;
  ParsedGame entry;
  std::vector<LaunchTarget> targets;
  std::string root_identity;
  std::string relative_identity;
  std::string legacy_id_text;
};

std::string Trim(std::string value) {
  const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                          [&](char ch) { return !is_space(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(),
                           [&](char ch) { return !is_space(ch); }).base(), value.end());
  return value;
}

std::string UnescapeText(std::string value) {
  std::string result;
  result.reserve(value.size());
  for (size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '\\' && index + 1 < value.size() && value[index + 1] == 'n') {
      result.push_back('\n');
      ++index;
    } else {
      result.push_back(value[index]);
    }
  }
  return result;
}

void AddMetadataFiles(const fs::path &root, std::vector<fs::path> *files) {
  std::error_code error;
  if (fs::is_regular_file(root, error) && root.filename() == "metadata.pegasus.txt") {
    files->push_back(root);
    return;
  }
  if (!fs::is_directory(root, error)) return;

  fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                      error);
  const fs::recursive_directory_iterator end;
  for (; !error && it != end; it.increment(error)) {
    if (it->is_directory(error) && IsIgnoredDataPath(it->path().filename())) {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_directory(error) && it->path().filename() == "media") {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_regular_file(error) && it->path().filename() == "metadata.pegasus.txt") {
      files->push_back(it->path());
    }
  }
}

std::vector<ParsedGame> ParseMetadataFile(const fs::path &metadata) {
  std::ifstream input(metadata, std::ios::binary);
  std::vector<ParsedGame> games;
  ParsedGame current;
  std::string collection_title;
  std::string collection_launch_hint;
  bool has_current = false;
  bool reading_files = false;
  bool reading_launch = false;
  bool reading_collection_launch = false;
  std::string line;

  auto finish = [&]() {
    if (has_current && !current.title.empty()) games.push_back(std::move(current));
    current = ParsedGame{};
    has_current = false;
    reading_files = false;
    reading_launch = false;
  };

  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.rfind("game:", 0) == 0) {
      finish();
      current.collection_title = collection_title;
      current.title = Trim(line.substr(5));
      current.launch_hint = collection_launch_hint;
      has_current = true;
      continue;
    }
    if (!has_current) {
      if (reading_collection_launch && line.rfind("  ", 0) == 0) {
        if (!collection_launch_hint.empty()) collection_launch_hint.push_back('\n');
        collection_launch_hint += Trim(line);
        continue;
      }
      reading_collection_launch = false;
      const size_t colon = line.find(':');
      if (colon == std::string::npos) continue;
      const std::string key = Trim(line.substr(0, colon));
      const std::string value = Trim(line.substr(colon + 1));
      if (key == "collection") {
        collection_title = value;
      } else if (key == "launch") {
        collection_launch_hint = value;
        reading_collection_launch = true;
      }
      continue;
    }
    if (reading_files && line.rfind("  ", 0) == 0) {
      std::string file = Trim(line);
      if (!file.empty()) current.files.push_back(std::move(file));
      continue;
    }
    if (reading_launch && line.rfind("  ", 0) == 0) {
      if (!current.launch_hint.empty()) current.launch_hint.push_back('\n');
      current.launch_hint += Trim(line);
      continue;
    }
    reading_files = false;
    reading_launch = false;

    const size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    const std::string key = Trim(line.substr(0, colon));
    const std::string value = Trim(line.substr(colon + 1));
    if (key == "file") current.files.push_back(value);
    else if (key == "files") reading_files = true;
    else if (key == "developer") current.developer = value;
    else if (key == "publisher") current.publisher = value;
    else if (key == "genre") current.genre = value;
    else if (key == "release") current.release = value;
    else if (key == "x-id") current.external_id = value;
    else if (key == "description") current.description = UnescapeText(value);
    else if (key == "sort-by") current.sort_key = value;
    else if (key == "assets.box_front") current.cover = value;
    else if (key == "assets.logo") current.logo = value;
    else if (key == "assets.video") current.video = value;
    else if (key == "launch") {
      current.launch_hint = value;
      reading_launch = true;
    }
  }
  finish();
  return games;
}

std::string ExistingFile(const fs::path &base, const std::string &candidate) {
  if (candidate.empty()) return {};
  const fs::path path = fs::u8path(candidate).is_absolute() ? fs::u8path(candidate)
                                                            : base / fs::u8path(candidate);
  if (IsIgnoredDataPath(path.filename())) return {};
  std::error_code error;
  if (!fs::is_regular_file(path, error)) return {};
  return NormalizedPath(path).u8string();
}

std::string MetadataRootIdentity(const Platform &platform, const fs::path &base) {
  for (const std::string &root_text : platform.rom_directories) {
    const fs::path root = fs::u8path(root_text);
    if (!PathIsInside(base, root)) continue;
    return RelativeIdentityPath(base, root.parent_path());
  }
  return base.filename().u8string();
}

std::string ResolveMedia(const fs::path &base, const ParsedGame &game,
                         const std::string &explicit_path,
                         const std::vector<std::string> &fallback_names) {
  std::string found = ExistingFile(base, explicit_path);
  if (!found.empty()) return found;
  std::vector<std::string> media_keys = {game.title};
  for (const std::string &file : game.files) {
    const fs::path path = fs::u8path(file);
    media_keys.push_back(path.stem().u8string());
    media_keys.push_back(path.filename().u8string());
  }
  std::sort(media_keys.begin(), media_keys.end());
  media_keys.erase(std::unique(media_keys.begin(), media_keys.end()), media_keys.end());
  for (const std::string &key : media_keys) {
    for (const std::string &name : fallback_names) {
      found = ExistingFile(base, (fs::path("media") / fs::u8path(key) / name).u8string());
      if (!found.empty()) return found;
    }
  }
  return {};
}

}  // namespace

ScanResult PegasusProvider::Scan(const Platform &platform,
                                 const std::vector<ScanRoot> &roots) const {
  ScanResult result;
  std::vector<fs::path> metadata_files;
  for (const ScanRoot &root : roots) AddMetadataFiles(fs::u8path(root.path), &metadata_files);
  std::sort(metadata_files.begin(), metadata_files.end());
  metadata_files.erase(std::unique(metadata_files.begin(), metadata_files.end()),
                       metadata_files.end());
  result.metadata_files = static_cast<int>(metadata_files.size());

  std::vector<ResolvedPegasusGame> resolved_games;
  std::unordered_map<std::string, int> legacy_id_counts;
  for (const fs::path &metadata : metadata_files) {
    const fs::path base = metadata.parent_path();
    const std::string root_identity = LowerAscii(MetadataRootIdentity(platform, base));
    const std::vector<ParsedGame> entries = ParseMetadataFile(metadata);
    result.parsed_entries += static_cast<int>(entries.size());
    for (const ParsedGame &entry : entries) {
      std::vector<LaunchTarget> targets;
      for (const std::string &file : entry.files) {
        const std::string resolved = ExistingFile(base, file);
        if (resolved.empty()) continue;
        if (!PathIsInside(fs::u8path(resolved), base) ||
            !HasAllowedExtension(fs::u8path(resolved), platform.extensions)) {
          ++result.rejected_paths;
          continue;
        }
        targets.push_back(LaunchTarget{resolved, fs::u8path(resolved).filename().u8string()});
      }
      if (targets.empty()) {
        ++result.missing_entries;
        continue;
      }

      const std::string identity =
          RelativeIdentityPath(fs::u8path(targets.front().path), base);
      const std::string legacy_id_text = platform.id + ":" + LowerAscii(identity);
      ++legacy_id_counts[legacy_id_text];
      resolved_games.push_back(ResolvedPegasusGame{metadata, base, entry, std::move(targets),
                                                   root_identity, identity, legacy_id_text});
    }
  }

  std::unordered_set<std::string> seen_ids;
  for (const ResolvedPegasusGame &resolved : resolved_games) {
    const bool needs_scoped_id = legacy_id_counts[resolved.legacy_id_text] > 1;
    const std::string id_text =
        needs_scoped_id
            ? platform.id + ":" + resolved.root_identity + ":" +
                  LowerAscii(resolved.relative_identity)
            : resolved.legacy_id_text;
    const std::string id = StableId(id_text);
    if (!seen_ids.insert(id).second) {
      ++result.duplicate_entries;
      continue;
    }

    Game game;
    game.id = id;
    game.platform_id = platform.id;
    game.collection_title = resolved.entry.collection_title;
    if (!game.collection_title.empty()) {
      game.collection_id = "collection:" + game.collection_title;
    }
    game.title = resolved.entry.title;
    game.developer = resolved.entry.developer;
    game.publisher = resolved.entry.publisher;
    game.genre = resolved.entry.genre;
    game.release = resolved.entry.release;
    game.external_id = resolved.entry.external_id;
    game.description = resolved.entry.description;
    game.sort_key = resolved.entry.sort_key.empty() ? resolved.entry.title
                                                    : resolved.entry.sort_key;
    game.primary_target = resolved.targets.front();
    if (resolved.targets.size() > 1) {
      game.alternate_targets.assign(resolved.targets.begin() + 1, resolved.targets.end());
    }
    for (std::size_t index = 1; index < resolved.targets.size(); ++index) {
      const std::string alternate_identity =
          RelativeIdentityPath(fs::u8path(resolved.targets[index].path), resolved.base);
      const std::string alternate_id_text =
          needs_scoped_id
              ? platform.id + ":" + resolved.root_identity + ":" +
                    LowerAscii(alternate_identity)
              : platform.id + ":" + LowerAscii(alternate_identity);
      game.alternate_target_ids.push_back(StableId(alternate_id_text));
    }
    game.media.cover = ResolveMedia(resolved.base, resolved.entry, resolved.entry.cover,
                                    platform.media_rule.cover_names);
    game.media.logo = ResolveMedia(resolved.base, resolved.entry, resolved.entry.logo,
                                   platform.media_rule.logo_names);
    game.media.video = ResolveMedia(resolved.base, resolved.entry, resolved.entry.video,
                                    platform.media_rule.video_names);
    std::error_code error;
    const fs::path primary_path = fs::u8path(game.primary_target.path);
    game.fingerprint.size = fs::file_size(primary_path, error);
    game.fingerprint.modified_time = PortableModifiedTime(primary_path);
    game.fingerprint.sample_hash = SampleFingerprint(primary_path);
    game.metadata_path = NormalizedPath(resolved.metadata).u8string();
    game.source = "pegasus";
    game.non_executable_launch_hint = resolved.entry.launch_hint;
    game.launch_hint = ResolveLaunchHint(resolved.entry.launch_hint);
    const std::string ext = LowerAscii(primary_path.extension().u8string());
    game.multi_file_entry = ext == ".cue" || ext == ".m3u" || ext == ".chd";
    result.games.push_back(std::move(game));
  }
  return result;
}

}  // namespace mpl

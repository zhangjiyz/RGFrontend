#include "catalog/providers/anbernic_provider.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include "catalog/path.h"
#include "catalog/ports_alias.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

struct PackageEntry {
  std::string path;
  std::string name;
  std::string description;
  std::string developer;
  std::string image;
  std::string marquee;
  std::string video;
};

std::string ReadFile(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  std::ostringstream out;
  out << input.rdbuf();
  return out.str();
}

std::string XmlUnescape(std::string value) {
  struct Replacement {
    const char *from;
    const char *to;
  };
  static const Replacement kReplacements[] = {
      {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"},
  };
  for (const Replacement &replacement : kReplacements) {
    size_t position = 0;
    while ((position = value.find(replacement.from, position)) != std::string::npos) {
      value.replace(position, std::strlen(replacement.from), replacement.to);
      position += std::strlen(replacement.to);
    }
  }
  return value;
}

std::string TagValue(const std::string &block, const std::string &tag) {
  const std::string open = "<" + tag + ">";
  const std::string close = "</" + tag + ">";
  const size_t begin = block.find(open);
  if (begin == std::string::npos) return {};
  const size_t value_begin = begin + open.size();
  const size_t end = block.find(close, value_begin);
  if (end == std::string::npos) return {};
  return XmlUnescape(block.substr(value_begin, end - value_begin));
}

std::vector<PackageEntry> ParseGamelist(const fs::path &path) {
  std::vector<PackageEntry> entries;
  const std::string xml = ReadFile(path);
  size_t position = 0;
  while ((position = xml.find("<game>", position)) != std::string::npos) {
    const size_t begin = position + 6;
    const size_t end = xml.find("</game>", begin);
    if (end == std::string::npos) break;
    const std::string block = xml.substr(begin, end - begin);
    PackageEntry entry;
    entry.path = TagValue(block, "path");
    entry.name = TagValue(block, "name");
    entry.description = TagValue(block, "desc");
    entry.developer = TagValue(block, "developer");
    entry.image = TagValue(block, "image");
    entry.marquee = TagValue(block, "marquee");
    entry.video = TagValue(block, "video");
    if (!entry.path.empty()) entries.push_back(std::move(entry));
    position = end + 7;
  }
  return entries;
}

std::string ResolveExisting(const fs::path &base, const std::string &path_text) {
  if (path_text.empty()) return {};
  const fs::path path = fs::u8path(path_text).is_absolute() ? fs::u8path(path_text)
                                                            : base / fs::u8path(path_text);
  if (IsIgnoredDataPath(path.filename())) return {};
  std::error_code error;
  if (!fs::is_regular_file(path, error)) return {};
  return NormalizedPath(path).u8string();
}

std::string FindPreviewImage(const fs::path &root, const fs::path &rom_path,
                             const std::string &explicit_image) {
  const std::string explicit_found = ResolveExisting(root, explicit_image);
  if (!explicit_found.empty()) return explicit_found;

  const fs::path image_dir = root / "Imgs";
  const fs::path sibling_image_dir = rom_path.parent_path() / "Imgs";
  const std::vector<std::string> bases = {
      rom_path.stem().u8string(),
      rom_path.filename().u8string(),
  };
  const std::vector<std::string> extensions = {".png", ".jpg", ".jpeg"};
  const std::vector<fs::path> image_dirs = {sibling_image_dir, image_dir};
  for (const fs::path &dir : image_dirs) {
    for (const std::string &base : bases) {
      for (const std::string &extension : extensions) {
        const fs::path candidate = dir / fs::u8path(base + extension);
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) return NormalizedPath(candidate).u8string();
      }
    }
  }
  return {};
}

std::string FindPreviewVideo(const fs::path &root, const fs::path &rom_path,
                             const std::string &explicit_video) {
  const std::string explicit_found = ResolveExisting(root, explicit_video);
  if (!explicit_found.empty()) return explicit_found;

  const fs::path system_name = root.filename();
  const std::vector<fs::path> directories = {
      root / "video",
      root / "videos",
      root / "media",
      root / "media" / "videos",
      root / "downloaded_media" / "videos",
      root / "downloaded_media" / system_name / "videos",
  };
  const std::vector<std::string> bases = {
      rom_path.stem().u8string(),
      rom_path.filename().u8string(),
  };
  const std::vector<std::string> extensions = {".mp4", ".mkv", ".avi"};
  for (const fs::path &directory : directories) {
    for (const std::string &base : bases) {
      for (const std::string &extension : extensions) {
        const fs::path candidate = directory / fs::u8path(base + extension);
        if (IsIgnoredDataPath(candidate.filename())) continue;
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) {
          return NormalizedPath(candidate).u8string();
        }
      }
    }
  }
  return {};
}

Game BuildGame(const Platform &platform, const fs::path &root, const fs::path &rom_path,
               const PackageEntry *entry, const std::string &image_path) {
  const std::string relative = RelativeIdentityPath(rom_path, root);
  Game game;
  std::string identity = relative;
  if (platform.id == "ports") {
    const std::string ports_identity = PortsEntryIdentityPath(root, rom_path);
    if (!ports_identity.empty()) identity = "entry-dir:" + ports_identity;
  }
  game.id = StableId(platform.id + ":" + LowerAscii(identity));
  game.platform_id = platform.id;
  game.title = entry && !entry->name.empty() ? entry->name : rom_path.stem().u8string();
  game.description = entry ? entry->description : std::string();
  game.developer = entry ? entry->developer : std::string();
  game.sort_key = game.title;
  game.primary_target = LaunchTarget{NormalizedPath(rom_path).u8string(), "default"};
  game.media.cover = image_path;
  if (entry) {
    game.media.logo = ResolveExisting(root, entry->marquee);
    game.media.video = FindPreviewVideo(root, rom_path, entry->video);
  }
  game.fingerprint.size = fs::file_size(rom_path);
  game.fingerprint.modified_time = PortableModifiedTime(rom_path);
  game.fingerprint.sample_hash = SampleFingerprint(rom_path);
  game.source = "anbernic";
  const std::string ext = LowerAscii(rom_path.extension().u8string());
  game.multi_file_entry = ext == ".cue" || ext == ".m3u" || ext == ".chd";
  return game;
}

std::unordered_map<std::string, PackageEntry> EntriesByPath(const std::vector<PackageEntry> &entries,
                                                            const fs::path &root) {
  std::unordered_map<std::string, PackageEntry> result;
  for (const PackageEntry &entry : entries) {
    const std::string resolved = ResolveExisting(root, entry.path);
    if (!resolved.empty()) result.emplace(NormalizedPath(fs::u8path(resolved)).u8string(), entry);
  }
  return result;
}

}  // namespace

ScanResult AnbernicProvider::Scan(const Platform &platform,
                                  const std::vector<ScanRoot> &roots) const {
  ScanResult result;
  std::unordered_set<std::string> seen_ids;
  for (const ScanRoot &scan_root : roots) {
    const fs::path root = fs::u8path(scan_root.path);
    std::error_code error;
    if (!fs::is_directory(root, error)) continue;

    std::unordered_map<std::string, PackageEntry> entries;
    const fs::path gamelist = root / "gamelist.xml";
    if (fs::is_regular_file(gamelist, error)) {
      ++result.metadata_files;
      const std::vector<PackageEntry> parsed = ParseGamelist(gamelist);
      result.parsed_entries += static_cast<int>(parsed.size());
      entries = EntriesByPath(parsed, root);
    }

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                        error);
    const fs::recursive_directory_iterator end;
    for (; !error && it != end; it.increment(error)) {
      if (it->is_directory(error) && IsIgnoredDataPath(it->path().filename())) {
        it.disable_recursion_pending();
        continue;
      }
      if (it->is_directory(error) && it->path().filename() == "Imgs") {
        it.disable_recursion_pending();
        continue;
      }
      if (!it->is_regular_file(error)) continue;
      const fs::path rom_path = it->path();
      if (IsIgnoredDataPath(rom_path.filename())) continue;
      if (IsLikelySidecarFile(rom_path) || !HasAllowedExtension(rom_path, platform.extensions)) {
        continue;
      }
      if (!PathIsInside(rom_path, root)) {
        ++result.rejected_paths;
        continue;
      }
      const std::string normalized = NormalizedPath(rom_path).u8string();
      const auto entry = entries.find(normalized);
      const PackageEntry *entry_ptr = entry == entries.end() ? nullptr : &entry->second;
      const std::string image = FindPreviewImage(root, rom_path,
                                                 entry_ptr ? entry_ptr->image : std::string());
      if (!entry_ptr && image.empty()) continue;

      Game game = BuildGame(platform, root, rom_path, entry_ptr, image);
      game.metadata_path = entry_ptr ? NormalizedPath(gamelist).u8string() : std::string();
      if (!seen_ids.insert(game.id).second && platform.id != "ports") {
        ++result.duplicate_entries;
        continue;
      }
      result.games.push_back(std::move(game));
    }
    if (error) result.warnings.push_back("anbernic scan stopped at " + root.u8string());
  }
  std::sort(result.games.begin(), result.games.end(), [](const Game &left, const Game &right) {
    if (left.sort_key != right.sort_key) return left.sort_key < right.sort_key;
    return left.primary_target.path < right.primary_target.path;
  });
  return result;
}

}  // namespace mpl

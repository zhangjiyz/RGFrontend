#include "catalog/providers/emulationstation_provider.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_set>

#include "catalog/path.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

struct EsEntry {
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

std::vector<EsEntry> ParseGamelist(const std::string &xml) {
  std::vector<EsEntry> entries;
  size_t position = 0;
  while ((position = xml.find("<game>", position)) != std::string::npos) {
    const size_t begin = position + 6;
    const size_t end = xml.find("</game>", begin);
    if (end == std::string::npos) break;
    const std::string block = xml.substr(begin, end - begin);
    EsEntry entry;
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

std::string FindVideo(const fs::path &root, const fs::path &rom_path,
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

std::vector<fs::path> FindGamelists(const fs::path &root) {
  std::vector<fs::path> gamelists;
  std::error_code error;
  if (fs::is_regular_file(root, error)) {
    if (root.filename() == "gamelist.xml") gamelists.push_back(root);
    return gamelists;
  }
  if (!fs::is_directory(root, error)) return gamelists;

  const fs::path direct = root / "gamelist.xml";
  if (fs::is_regular_file(direct, error)) gamelists.push_back(direct);

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
    if (!it->is_regular_file(error) || it->path().filename() != "gamelist.xml") continue;
    if (NormalizedPath(it->path()) != NormalizedPath(direct)) gamelists.push_back(it->path());
  }
  return gamelists;
}

}  // namespace

ScanResult EmulationStationProvider::Scan(const Platform &platform,
                                          const std::vector<ScanRoot> &roots) const {
  ScanResult result;
  std::unordered_set<std::string> seen_ids;
  for (const ScanRoot &root_text : roots) {
    const fs::path root = fs::u8path(root_text.path);
    for (const fs::path &gamelist : FindGamelists(root)) {
      ++result.metadata_files;
      const fs::path base = gamelist.parent_path();

      const std::vector<EsEntry> entries = ParseGamelist(ReadFile(gamelist));
      result.parsed_entries += static_cast<int>(entries.size());
      for (const EsEntry &entry : entries) {
        const std::string rom_path = ResolveExisting(base, entry.path);
        if (rom_path.empty()) {
          ++result.missing_entries;
          continue;
        }
        if (!PathIsInside(fs::u8path(rom_path), root) ||
            IsIgnoredDataPath(fs::u8path(rom_path).filename()) ||
            !HasAllowedExtension(fs::u8path(rom_path), platform.extensions)) {
          ++result.rejected_paths;
          continue;
        }

        const std::string relative = RelativeIdentityPath(fs::u8path(rom_path), root);
        const std::string id = StableId(platform.id + ":" + LowerAscii(relative));
        if (!seen_ids.insert(id).second) {
          ++result.duplicate_entries;
          continue;
        }

        std::error_code error;
        const fs::path primary_path = fs::u8path(rom_path);
        Game game;
        game.id = id;
        game.platform_id = platform.id;
        game.title = entry.name.empty() ? primary_path.stem().u8string() : entry.name;
        game.sort_key = game.title;
        game.developer = entry.developer;
        game.description = entry.description;
        game.primary_target = LaunchTarget{rom_path, "default"};
        game.media.cover = ResolveExisting(base, entry.image);
        game.media.logo = ResolveExisting(base, entry.marquee);
        game.media.video = FindVideo(base, primary_path, entry.video);
        game.fingerprint.size = fs::file_size(primary_path, error);
        game.fingerprint.modified_time = PortableModifiedTime(primary_path);
        game.fingerprint.sample_hash = SampleFingerprint(primary_path);
        game.metadata_path = NormalizedPath(gamelist).u8string();
        game.source = "emulationstation";
        const std::string ext = LowerAscii(primary_path.extension().u8string());
        game.multi_file_entry = ext == ".cue" || ext == ".m3u" || ext == ".chd";
        result.games.push_back(std::move(game));
      }
    }
  }
  return result;
}

}  // namespace mpl

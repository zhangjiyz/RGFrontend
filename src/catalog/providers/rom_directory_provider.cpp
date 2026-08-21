#include "catalog/providers/rom_directory_provider.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

#include "catalog/path.h"
#include "catalog/ports_alias.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

bool HasSiblingArchiveForDirectory(const Platform &platform, const fs::path &directory) {
  const fs::path parent = directory.parent_path();
  const std::string stem = directory.filename().u8string();
  if (stem.empty()) return false;
  for (const char *extension : {".zip", ".7z"}) {
    const fs::path archive = parent / fs::u8path(stem + extension);
    std::error_code error;
    if (fs::is_regular_file(archive, error) && HasAllowedExtension(archive, platform.extensions)) {
      return true;
    }
  }
  return false;
}

bool IsArcadeChdSidecarDirectory(const Platform &platform, const fs::path &root,
                                 const fs::path &directory) {
  if (directory.parent_path() != root) return false;
  return HasSiblingArchiveForDirectory(platform, directory);
}

bool IsMediaDirectory(const fs::path &path) {
  const std::string name = LowerAscii(path.filename().u8string());
  return name == "imgs" || name == "media" || name == "video" ||
         name == "videos" || name == "downloaded_media";
}

Game BuildMinimalGame(const Platform &platform, const fs::path &root, const fs::path &path) {
  const std::string relative = RelativeIdentityPath(path, root);
  Game game;
  game.platform_id = platform.id;
  std::string identity = relative;
  if (platform.id == "ports") {
    const std::string ports_identity = PortsEntryIdentityPath(root, path);
    if (!ports_identity.empty()) identity = "entry-dir:" + ports_identity;
  }
  game.id = StableId(platform.id + ":" + LowerAscii(identity));
  game.title = path.stem().u8string();
  game.sort_key = game.title;
  game.primary_target = LaunchTarget{NormalizedPath(path).u8string(), "default"};
  std::error_code error;
  game.fingerprint.size = fs::file_size(path, error);
  game.fingerprint.modified_time = PortableModifiedTime(path);
  game.fingerprint.sample_hash = SampleFingerprint(path);
  game.source = "rom-directory";
  const std::string ext = LowerAscii(path.extension().u8string());
  game.multi_file_entry = ext == ".cue" || ext == ".m3u" || ext == ".chd";
  return game;
}

}  // namespace

ScanResult RomDirectoryProvider::Scan(const Platform &platform,
                                      const std::vector<ScanRoot> &roots) const {
  ScanResult result;
  for (const ScanRoot &root_text : roots) {
    const fs::path root = fs::u8path(root_text.path);
    std::error_code error;
    if (!fs::is_directory(root, error)) continue;

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                        error);
    const fs::recursive_directory_iterator end;
    for (; !error && it != end; it.increment(error)) {
      if (it->is_directory(error) &&
          (IsIgnoredDataPath(it->path().filename()) || IsMediaDirectory(it->path()))) {
        it.disable_recursion_pending();
        continue;
      }
      if (it->is_directory(error) &&
          IsArcadeChdSidecarDirectory(platform, root, it->path())) {
        it.disable_recursion_pending();
        continue;
      }
      std::error_code entry_error;
      if (!it->is_regular_file(entry_error)) continue;
      const fs::path path = it->path();
      if (IsIgnoredDataPath(path.filename())) continue;
      if (IsLikelySidecarFile(path)) continue;
      if (!HasAllowedExtension(path, platform.extensions)) continue;
      ++result.scanned_files;
      if (!PathIsInside(path, root)) {
        ++result.rejected_paths;
        continue;
      }
      result.games.push_back(BuildMinimalGame(platform, root, path));
    }
    if (error) result.warnings.push_back("scan stopped at " + root.u8string());
  }

  std::sort(result.games.begin(), result.games.end(), [](const Game &left, const Game &right) {
    return left.sort_key < right.sort_key;
  });
  return result;
}

}  // namespace mpl

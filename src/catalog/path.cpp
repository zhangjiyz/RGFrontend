#include "catalog/path.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace mpl {

std::string LowerAscii(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

std::string StableId(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (unsigned char ch : text) {
    hash ^= ch;
    hash *= 1099511628211ULL;
  }
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << hash;
  return out.str();
}

fs::path NormalizedPath(const fs::path &path) {
  std::error_code error;
  fs::path resolved = fs::weakly_canonical(path, error);
  if (!error) return resolved;
  return path.lexically_normal();
}

bool HasAllowedExtension(const fs::path &path, const std::vector<std::string> &extensions) {
  const std::string actual = LowerAscii(path.extension().u8string());
  for (std::string expected : extensions) {
    expected = LowerAscii(std::move(expected));
    if (!expected.empty() && expected.front() != '.') expected.insert(expected.begin(), '.');
    if (actual == expected) return true;
  }
  return false;
}

bool IsLikelySidecarFile(const fs::path &path) {
  const std::string ext = LowerAscii(path.extension().u8string());
  if (ext == ".wav" || ext == ".ape" || ext == ".flac" || ext == ".toc") {
    return true;
  }
  if (ext != ".bin" && ext != ".iso") return false;

  const fs::path parent = path.parent_path();
  const std::string stem = LowerAscii(path.stem().u8string());
  const bool track_like = stem.rfind("track", 0) == 0 || stem.find(" track") != std::string::npos ||
                          stem.find("(track") != std::string::npos;
  for (const char *descriptor_ext : {".cue", ".ccd", ".mds", ".gdi", ".m3u"}) {
    std::error_code error;
    if (fs::is_regular_file(parent / fs::u8path(path.stem().u8string() + descriptor_ext),
                            error)) {
      return true;
    }
  }
  if (!track_like) return false;

  std::error_code error;
  fs::directory_iterator it(parent, fs::directory_options::skip_permission_denied, error);
  const fs::directory_iterator end;
  for (; !error && it != end; it.increment(error)) {
    if (!it->is_regular_file(error)) continue;
    const std::string sibling_ext = LowerAscii(it->path().extension().u8string());
    if (sibling_ext == ".cue" || sibling_ext == ".ccd" || sibling_ext == ".mds" ||
        sibling_ext == ".gdi" || sibling_ext == ".m3u") {
      return true;
    }
  }
  return false;
}

bool IsIgnoredDataPath(const fs::path &path) {
  for (const fs::path &component : path) {
    const std::string name = component.u8string();
    if (name.empty() || name == "/" || name == "." || name == "..") continue;
    if (name == "__MACOSX") return true;
    if (!name.empty() && name.front() == '.') return true;
    if (name.rfind("._", 0) == 0) return true;
  }
  return false;
}

bool PathIsInside(const fs::path &path, const fs::path &root) {
  const fs::path normalized_path = NormalizedPath(path);
  const fs::path normalized_root = NormalizedPath(root);
  auto path_it = normalized_path.begin();
  for (auto root_it = normalized_root.begin(); root_it != normalized_root.end(); ++root_it) {
    if (path_it == normalized_path.end() || *path_it != *root_it) return false;
    ++path_it;
  }
  return true;
}

std::string RelativeIdentityPath(const fs::path &path, const fs::path &root) {
  std::error_code error;
  fs::path relative = fs::relative(NormalizedPath(path), NormalizedPath(root), error);
  if (error || relative.empty()) relative = path.filename();
  return relative.lexically_normal().u8string();
}

std::int64_t PortableModifiedTime(const fs::path &path) {
  std::error_code error;
  const auto value = fs::last_write_time(path, error);
  if (error) return 0;
  return value.time_since_epoch().count();
}

std::string SampleFingerprint(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return {};
  constexpr std::size_t kMaxBytes = 4096;
  std::uint64_t hash = 1469598103934665603ULL;
  char buffer[kMaxBytes];
  input.read(buffer, sizeof(buffer));
  const std::streamsize bytes = input.gcount();
  for (std::streamsize index = 0; index < bytes; ++index) {
    hash ^= static_cast<unsigned char>(buffer[index]);
    hash *= 1099511628211ULL;
  }
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << hash;
  return out.str();
}

}  // namespace mpl

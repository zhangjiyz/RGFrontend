#include "catalog/library_builder.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "catalog/arcade_name_database.h"
#include "catalog/launch_hint_resolver.h"
#include "catalog/path.h"
#include "catalog/providers/anbernic_provider.h"
#include "catalog/providers/emulationstation_provider.h"
#include "catalog/providers/pegasus_provider.h"
#include "catalog/providers/rom_directory_provider.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

constexpr int kScanCacheVersion = 28;

struct CachedRoot {
  std::string platform_id;
  std::string root;
  std::int64_t mtime = 0;
  std::string arcade_name_database_path;
  std::int64_t arcade_name_database_mtime = 0;
};

struct CachedLibrary {
  std::vector<CachedRoot> roots;
  std::vector<Game> games;
};

struct CachedRootSummary {
  bool version_ok = false;
  bool has_games = false;
  std::vector<CachedRoot> roots;
};

using Clock = std::chrono::steady_clock;

long long ElapsedMs(Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             Clock::now() - start)
      .count();
}

std::string CleanField(std::string value) {
  for (char &ch : value) {
    if (ch == '\t' || ch == '\n' || ch == '\r') ch = ' ';
  }
  return value;
}

std::string JoinAlternateTargets(const std::vector<LaunchTarget> &targets) {
  std::string result;
  for (const LaunchTarget &target : targets) {
    if (!result.empty()) result.push_back('\x1e');
    result += target.path;
    result.push_back('\x1f');
    result += target.label;
  }
  return result;
}

std::vector<LaunchTarget> SplitAlternateTargets(const std::string &field) {
  std::vector<LaunchTarget> targets;
  std::size_t begin = 0;
  while (begin < field.size()) {
    const std::size_t end = field.find('\x1e', begin);
    const std::string item = field.substr(begin, end == std::string::npos
                                                     ? std::string::npos
                                                     : end - begin);
    const std::size_t separator = item.find('\x1f');
    if (separator != std::string::npos) {
      targets.push_back(LaunchTarget{item.substr(0, separator),
                                     item.substr(separator + 1)});
    }
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  return targets;
}

std::string JoinStrings(const std::vector<std::string> &values) {
  std::string result;
  for (const std::string &value : values) {
    if (!result.empty()) result.push_back('\x1e');
    result += value;
  }
  return result;
}

std::vector<std::string> SplitStrings(const std::string &field) {
  std::vector<std::string> values;
  std::size_t begin = 0;
  while (begin < field.size()) {
    const std::size_t end = field.find('\x1e', begin);
    values.push_back(field.substr(begin, end == std::string::npos
                                             ? std::string::npos
                                             : end - begin));
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  return values;
}

CachedLibrary LoadScanCache(const std::string &cache_path) {
  CachedLibrary cache;
  if (cache_path.empty()) return cache;
  std::ifstream in(fs::u8path(cache_path));
  std::string line;
  bool version_ok = false;
  while (std::getline(in, line)) {
    if (line.rfind("# mpl_scan_cache_version=", 0) == 0) {
      version_ok = std::atoi(line.substr(25).c_str()) == kScanCacheVersion;
      continue;
    }
    if (line.empty() || line.front() == '#') continue;
    std::istringstream row(line);
    std::string kind;
    std::getline(row, kind, '\t');
    if (kind == "root") {
      CachedRoot root;
      std::string mtime;
      std::string game_count;
      std::string arcade_name_database_mtime;
      if (std::getline(row, root.platform_id, '\t') &&
          std::getline(row, root.root, '\t') &&
          std::getline(row, mtime, '\t')) {
        root.mtime = std::atoll(mtime.c_str());
        std::getline(row, game_count, '\t');
        std::getline(row, root.arcade_name_database_path, '\t');
        if (std::getline(row, arcade_name_database_mtime, '\t')) {
          root.arcade_name_database_mtime =
              std::atoll(arcade_name_database_mtime.c_str());
        }
        cache.roots.push_back(std::move(root));
      }
    } else if (kind == "game") {
      Game game;
      std::string size;
      std::string mtime;
      if (std::getline(row, game.id, '\t') &&
          std::getline(row, game.platform_id, '\t') &&
          std::getline(row, game.primary_target.path, '\t') &&
          std::getline(row, game.title, '\t') &&
          std::getline(row, game.source, '\t') &&
          std::getline(row, game.sort_key, '\t') &&
          std::getline(row, size, '\t') &&
          std::getline(row, mtime, '\t') &&
          std::getline(row, game.fingerprint.sample_hash, '\t')) {
        game.primary_target.label = "default";
        game.fingerprint.size = static_cast<std::uintmax_t>(std::strtoull(size.c_str(), nullptr, 10));
        game.fingerprint.modified_time = std::atoll(mtime.c_str());
        std::string multi_file_entry;
        std::getline(row, game.media.cover, '\t');
        std::getline(row, game.media.logo, '\t');
        std::getline(row, game.media.video, '\t');
        std::getline(row, game.developer, '\t');
        std::getline(row, game.description, '\t');
        std::getline(row, game.metadata_path, '\t');
        std::getline(row, game.non_executable_launch_hint, '\t');
        game.launch_hint = ResolveLaunchHint(game.non_executable_launch_hint);
        std::getline(row, multi_file_entry, '\t');
        game.multi_file_entry = multi_file_entry == "1";
        std::getline(row, game.collection_id, '\t');
        std::getline(row, game.collection_title, '\t');
        std::getline(row, game.publisher, '\t');
        std::getline(row, game.genre, '\t');
        std::getline(row, game.release, '\t');
        std::getline(row, game.external_id, '\t');
        std::string alternate_targets;
        std::getline(row, alternate_targets, '\t');
        game.alternate_targets = SplitAlternateTargets(alternate_targets);
        std::string alternate_target_ids;
        std::getline(row, alternate_target_ids, '\t');
        game.alternate_target_ids = SplitStrings(alternate_target_ids);
        std::string primary_target_label;
        if (std::getline(row, primary_target_label, '\t') && !primary_target_label.empty()) {
          game.primary_target.label = primary_target_label;
        }
        std::getline(row, game.display_title, '\t');
        cache.games.push_back(std::move(game));
      }
    }
  }
  if (!version_ok) return {};
  return cache;
}

CachedRootSummary LoadScanCacheRootSummary(const std::string &cache_path) {
  CachedRootSummary summary;
  if (cache_path.empty()) return summary;
  std::ifstream in(fs::u8path(cache_path));
  std::string line;
  while (std::getline(in, line)) {
    if (line.rfind("# mpl_scan_cache_version=", 0) == 0) {
      summary.version_ok = std::atoi(line.substr(25).c_str()) == kScanCacheVersion;
      continue;
    }
    if (line.empty() || line.front() == '#') continue;
    std::istringstream row(line);
    std::string kind;
    std::getline(row, kind, '\t');
    if (kind == "root") {
      CachedRoot root;
      std::string mtime;
      std::string game_count;
      std::string arcade_name_database_mtime;
      if (std::getline(row, root.platform_id, '\t') &&
          std::getline(row, root.root, '\t') &&
          std::getline(row, mtime, '\t')) {
        root.mtime = std::atoll(mtime.c_str());
        std::getline(row, game_count, '\t');
        std::getline(row, root.arcade_name_database_path, '\t');
        if (std::getline(row, arcade_name_database_mtime, '\t')) {
          root.arcade_name_database_mtime =
              std::atoll(arcade_name_database_mtime.c_str());
        }
        summary.roots.push_back(std::move(root));
      }
    } else if (kind == "game") {
      summary.has_games = true;
    }
  }
  return summary;
}

bool CachedRootIsFresh(const CachedLibrary &cache, const Platform &platform,
                       const std::string &root) {
  const std::int64_t current_mtime = PortableModifiedTime(fs::u8path(root));
  const std::int64_t current_database_mtime =
      PortableModifiedTime(fs::u8path(platform.arcade_name_database_path));
  for (const CachedRoot &cached : cache.roots) {
    if (cached.platform_id == platform.id && cached.root == root &&
        cached.mtime == current_mtime &&
        cached.arcade_name_database_path == platform.arcade_name_database_path &&
        cached.arcade_name_database_mtime == current_database_mtime) {
      return true;
    }
  }
  return false;
}

bool CachedRootIsFresh(const CachedRootSummary &cache, const Platform &platform,
                       const std::string &root) {
  const std::int64_t current_mtime = PortableModifiedTime(fs::u8path(root));
  const std::int64_t current_database_mtime =
      PortableModifiedTime(fs::u8path(platform.arcade_name_database_path));
  for (const CachedRoot &cached : cache.roots) {
    if (cached.platform_id == platform.id && cached.root == root &&
        cached.mtime == current_mtime &&
        cached.arcade_name_database_path == platform.arcade_name_database_path &&
        cached.arcade_name_database_mtime == current_database_mtime) {
      return true;
    }
  }
  return false;
}

void MergeGame(const Game &game, LibraryBuildReport *report,
               std::unordered_map<std::string, size_t> *index_by_id) {
  for (const std::string &alternate_id : game.alternate_target_ids) {
    const auto alternate = index_by_id->find(alternate_id);
    if (alternate == index_by_id->end()) continue;
    const std::size_t remove_index = alternate->second;
    if (remove_index >= report->library.games.size()) continue;
    report->library.games.erase(report->library.games.begin() +
                                static_cast<std::ptrdiff_t>(remove_index));
    index_by_id->clear();
    for (std::size_t index = 0; index < report->library.games.size(); ++index) {
      (*index_by_id)[report->library.games[index].id] = index;
    }
    ++report->merged_duplicates;
  }

  const auto found = index_by_id->find(game.id);
  if (found == index_by_id->end()) {
    index_by_id->emplace(game.id, report->library.games.size());
    report->library.games.push_back(game);
  } else {
    Game merged = game;
    const Game &previous = report->library.games[found->second];
    const bool anbernic_overlay = game.source == "anbernic" &&
                                  previous.source == "rom-directory";
    if (anbernic_overlay) {
      merged.primary_target = previous.primary_target;
      merged.fingerprint = previous.fingerprint;
      merged.multi_file_entry = previous.multi_file_entry;
      if (!previous.title.empty() &&
          game.title == fs::u8path(game.primary_target.path).stem().u8string()) {
        merged.title = previous.title;
        merged.sort_key = previous.sort_key;
      }
    }
    if (merged.media.cover.empty()) merged.media.cover = previous.media.cover;
    if (merged.media.logo.empty()) merged.media.logo = previous.media.logo;
    if (merged.media.video.empty()) merged.media.video = previous.media.video;
    if (merged.display_title.empty()) merged.display_title = previous.display_title;
    if (merged.developer.empty()) merged.developer = previous.developer;
    if (merged.publisher.empty()) merged.publisher = previous.publisher;
    if (merged.genre.empty()) merged.genre = previous.genre;
    if (merged.release.empty()) merged.release = previous.release;
    if (merged.external_id.empty()) merged.external_id = previous.external_id;
    if (merged.description.empty()) merged.description = previous.description;
    if (merged.metadata_path.empty()) merged.metadata_path = previous.metadata_path;
    if (merged.non_executable_launch_hint.empty()) {
      merged.non_executable_launch_hint = previous.non_executable_launch_hint;
      merged.launch_hint = previous.launch_hint;
    }
    if (merged.collection_id.empty()) merged.collection_id = previous.collection_id;
    if (merged.collection_title.empty()) merged.collection_title = previous.collection_title;
    auto add_target = [](std::vector<LaunchTarget> *targets,
                         const LaunchTarget &target,
                         const std::string &primary_path) {
      if (!targets || target.path.empty() || target.path == primary_path) return;
      const auto duplicate = std::find_if(targets->begin(), targets->end(),
                                          [&](const LaunchTarget &existing) {
        return existing.path == target.path;
      });
      if (duplicate == targets->end()) targets->push_back(target);
    };
    std::vector<LaunchTarget> targets = merged.alternate_targets;
    for (const LaunchTarget &target : previous.alternate_targets) {
      add_target(&targets, target, merged.primary_target.path);
    }
    add_target(&targets, previous.primary_target, merged.primary_target.path);
    add_target(&targets, game.primary_target, merged.primary_target.path);
    merged.alternate_targets = std::move(targets);
    if (merged.alternate_target_ids.empty()) {
      merged.alternate_target_ids = previous.alternate_target_ids;
    }
    if (merged.primary_target.label.empty()) merged.primary_target.label = previous.primary_target.label;
    merged.favorite = previous.favorite;
    merged.recent_order = previous.recent_order;
    merged.user_core_hint = previous.user_core_hint;
    report->library.games[found->second] = std::move(merged);
    ++report->merged_duplicates;
  }
}

void AppendWarnings(const ScanResult &result, LibraryBuildReport *report) {
  report->warnings.insert(report->warnings.end(), result.warnings.begin(),
                          result.warnings.end());
}

void ApplyArcadeNames(const ArcadeNameDatabase *database, ScanResult *result) {
  if (!database || !result) return;
  for (Game &game : result->games) database->Apply(&game);
}

bool IsEmptyArchive(const Game &game) {
  const fs::path path = fs::u8path(game.primary_target.path);
  const std::string extension = LowerAscii(path.extension().u8string());
  if (extension != ".zip" && extension != ".7z") return false;

  std::error_code error;
  if (!fs::is_regular_file(path, error) || error) return false;
  error.clear();
  return fs::file_size(path, error) == 0 && !error;
}

void RemoveEmptyArchives(ScanResult *result) {
  if (!result) return;
  result->games.erase(
      std::remove_if(result->games.begin(), result->games.end(), IsEmptyArchive),
      result->games.end());
}

bool IsArcadePlatform(const std::string &platform_id) {
  for (const char *id : {"atomiswave", "cps1", "cps2", "cps3", "fbneo", "hbmame",
                         "mame", "naomi", "neogeo", "pgm2", "varcade"}) {
    if (platform_id == id) return true;
  }
  return false;
}

void RemoveArcadeSupportArchives(const Platform &platform, ScanResult *result) {
  if (!result || !IsArcadePlatform(platform.id)) return;
  result->games.erase(
      std::remove_if(result->games.begin(), result->games.end(), [](const Game &game) {
        const std::string filename =
            LowerAscii(fs::u8path(game.primary_target.path).filename().u8string());
        return filename == "neogeo.zip" || filename == "pgm.zip";
      }),
      result->games.end());
}

bool HasMetadataFile(const fs::path &root, const char *filename) {
  std::error_code error;
  if (fs::is_regular_file(root, error)) {
    return root.filename() == filename;
  }
  if (!fs::is_directory(root, error)) return false;
  if (fs::is_regular_file(root / filename, error)) return true;

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
    if (it->is_regular_file(error) && it->path().filename() == filename) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> PegasusDiscoveryRoots(const std::vector<Platform> &platforms) {
  std::vector<std::string> roots;
  for (const Platform &platform : platforms) {
    for (const std::string &directory : platform.rom_directories) {
      const std::string root = NormalizedPath(fs::u8path(directory).parent_path()).u8string();
      if (std::find(roots.begin(), roots.end(), root) == roots.end()) roots.push_back(root);
    }
  }
  return roots;
}

bool PackageAlreadyCovered(const PegasusPackageInfo &package,
                           const std::vector<Platform> &platforms) {
  for (const Platform &platform : platforms) {
    for (const std::string &directory : platform.rom_directories) {
      if (PathIsInside(fs::u8path(package.root_path), fs::u8path(directory))) return true;
    }
  }
  return false;
}

bool StartsWithPlatformName(const std::string &collection_title,
                            const std::string &candidate) {
  const std::string title = LowerAscii(collection_title);
  const std::string name = LowerAscii(candidate);
  if (name.empty() || title.rfind(name, 0) != 0) return false;
  if (title.size() == name.size()) return true;
  return !std::isalnum(static_cast<unsigned char>(title[name.size()]));
}

int CollectionMatchScore(const PegasusPackageInfo &package, const Platform &platform) {
  std::vector<std::string> names = {platform.id, platform.display_name};
  names.insert(names.end(), platform.directory_aliases.begin(),
               platform.directory_aliases.end());
  names.insert(names.end(), platform.platform_aliases.begin(),
               platform.platform_aliases.end());
  int score = 0;
  for (const std::string &name : names) {
    if (LowerAscii(package.collection_title) == LowerAscii(name)) {
      score = std::max(score, 300);
    } else if (StartsWithPlatformName(package.collection_title, name)) {
      score = std::max(score, 250);
    }
  }
  return score;
}

int ExtensionMatchScore(const PegasusPackageInfo &package, const Platform &platform) {
  if (package.extensions.empty()) return 0;
  bool has_specific_extension = false;
  for (const std::string &extension : package.extensions) {
    const std::string normalized = LowerAscii(extension);
    if (normalized != ".zip" && normalized != ".7z") has_specific_extension = true;
    bool supported = false;
    for (std::string candidate : platform.extensions) {
      candidate = LowerAscii(std::move(candidate));
      if (!candidate.empty() && candidate.front() != '.') {
        candidate.insert(candidate.begin(), '.');
      }
      if (candidate == normalized) {
        supported = true;
        break;
      }
    }
    if (!supported) return -1;
  }
  return has_specific_extension ? 100 : 10;
}

Platform *ResolvePackagePlatform(const PegasusPackageInfo &package,
                                 std::vector<Platform> *platforms) {
  if (!platforms) return nullptr;
  int best_score = -1;
  Platform *best = nullptr;
  bool tied = false;
  for (Platform &platform : *platforms) {
    const int extension_score = ExtensionMatchScore(package, platform);
    if (extension_score < 0) continue;
    int score = CollectionMatchScore(package, platform) + extension_score;
    if (std::find(package.platform_hints.begin(), package.platform_hints.end(), platform.id) !=
        package.platform_hints.end()) {
      score += 200;
    }
    if (score > best_score) {
      best_score = score;
      best = &platform;
      tied = false;
    } else if (score == best_score) {
      tied = true;
    }
  }
  return best_score >= 100 && !tied ? best : nullptr;
}

bool IsExplicitlyDisabledPackage(const PegasusPackageInfo &package) {
  return LowerAscii(package.collection_title) == "fc-hd";
}

void ExpandPegasusPackageRoots(std::vector<Platform> *platforms,
                               std::vector<std::string> *warnings = nullptr) {
  if (!platforms) return;
  const std::vector<std::string> discovery_roots = PegasusDiscoveryRoots(*platforms);
  const std::vector<PegasusPackageInfo> packages =
      PegasusProvider().DiscoverPackages(discovery_roots);
  for (const PegasusPackageInfo &package : packages) {
    if (PackageAlreadyCovered(package, *platforms)) continue;
    if (IsExplicitlyDisabledPackage(package)) {
      if (warnings) warnings->push_back("disabled Pegasus package: " + package.metadata_path);
      continue;
    }
    Platform *platform = ResolvePackagePlatform(package, platforms);
    if (!platform) {
      if (warnings) warnings->push_back("unresolved Pegasus package: " + package.metadata_path);
      continue;
    }
    const auto duplicate = std::find_if(
        platform->rom_directories.begin(), platform->rom_directories.end(),
        [&](const std::string &existing) {
          std::error_code error;
          return fs::equivalent(fs::u8path(existing), fs::u8path(package.root_path), error) &&
                 !error;
        });
    if (duplicate == platform->rom_directories.end()) {
      platform->rom_directories.push_back(package.root_path);
    }
  }
}

void WriteScanCache(const Library &library, const std::string &cache_path,
                    LibraryBuildReport *report) {
  if (cache_path.empty()) return;
  const fs::path path = fs::u8path(cache_path);
  std::error_code error;
  fs::create_directories(path.parent_path(), error);
  const fs::path temporary = fs::u8path(cache_path + ".tmp");
  {
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) return;
    out << "# mpl_scan_cache_version=" << kScanCacheVersion << '\n';
    out << "# root\tplatform_id\troot\troot_mtime\tgame_count"
        << "\tarcade_name_database_path\tarcade_name_database_mtime\n";
    for (const Platform &platform : library.platforms) {
      for (const std::string &root_text : platform.rom_directories) {
        int count = 0;
        for (const Game &game : library.games) {
          if (game.platform_id == platform.id &&
              PathIsInside(fs::u8path(game.primary_target.path), fs::u8path(root_text))) {
            ++count;
          }
        }
        out << "root\t" << platform.id << '\t' << CleanField(root_text) << '\t'
            << PortableModifiedTime(fs::u8path(root_text)) << '\t' << count << '\t'
            << CleanField(platform.arcade_name_database_path) << '\t'
            << PortableModifiedTime(fs::u8path(platform.arcade_name_database_path)) << '\n';
        ++report->cache_records_written;
      }
    }
    out << "# game\tgame_id\tplatform_id\trom_path\ttitle\tsource\tsort_key\tsize\tmtime\tsample"
        << "\tcover\tlogo\tvideo\tdeveloper\tdescription\tmetadata_path\tlaunch_hint\tmulti_file"
        << "\tcollection_id\tcollection_title\tpublisher\tgenre\trelease\texternal_id"
        << "\talternate_targets\talternate_target_ids\tprimary_target_label\tdisplay_title\n";
    for (const Game &game : library.games) {
      out << "game\t" << game.id << '\t' << game.platform_id << '\t'
          << CleanField(game.primary_target.path) << '\t' << CleanField(game.title) << '\t'
          << CleanField(game.source) << '\t' << CleanField(game.sort_key) << '\t'
          << game.fingerprint.size << '\t' << game.fingerprint.modified_time << '\t'
          << game.fingerprint.sample_hash << '\t' << CleanField(game.media.cover) << '\t'
          << CleanField(game.media.logo) << '\t' << CleanField(game.media.video) << '\t'
          << CleanField(game.developer) << '\t' << CleanField(game.description) << '\t'
          << CleanField(game.metadata_path) << '\t'
          << CleanField(game.non_executable_launch_hint) << '\t'
          << (game.multi_file_entry ? 1 : 0) << '\t'
          << CleanField(game.collection_id) << '\t'
          << CleanField(game.collection_title) << '\t'
          << CleanField(game.publisher) << '\t'
          << CleanField(game.genre) << '\t'
          << CleanField(game.release) << '\t'
          << CleanField(game.external_id) << '\t'
          << CleanField(JoinAlternateTargets(game.alternate_targets)) << '\t'
          << CleanField(JoinStrings(game.alternate_target_ids)) << '\t'
          << CleanField(game.primary_target.label) << '\t'
          << CleanField(game.display_title) << '\n';
    }
    if (!out) return;
  }
  fs::rename(temporary, path, error);
  if (!error) return;
  fs::remove(path, error);
  error.clear();
  fs::rename(temporary, path, error);
}

}  // namespace

LibraryBuildReport LibraryBuilder::Build(std::vector<Platform> platforms,
                                         const std::string &cache_path,
                                         ProgressCallback progress) const {
  const auto total_start = Clock::now();
  LibraryBuildReport report;
  ExpandPegasusPackageRoots(&platforms, &report.warnings);
  if (progress) progress({5, "读取扫描缓存"});
  const auto cache_start = Clock::now();
  const CachedLibrary cache = LoadScanCache(cache_path);
  std::cerr << "[perf] library.cache_load ms=" << ElapsedMs(cache_start)
            << " roots=" << cache.roots.size()
            << " games=" << cache.games.size()
            << " path=" << cache_path << '\n';
  RomDirectoryProvider rom_provider;
  AnbernicProvider anbernic_provider;
  EmulationStationProvider es_provider;
  PegasusProvider pegasus_provider;
  std::unordered_map<std::string, ArcadeNameDatabase> arcade_name_databases;
  for (const Platform &platform : platforms) {
    if (platform.arcade_name_database_path.empty() ||
        arcade_name_databases.find(platform.arcade_name_database_path) !=
            arcade_name_databases.end()) {
      continue;
    }
    ArcadeNameDatabase database;
    if (database.Load(platform.arcade_name_database_path)) {
      arcade_name_databases.emplace(platform.arcade_name_database_path,
                                    std::move(database));
    }
  }
  std::unordered_map<std::string, size_t> index_by_id;
  const int total_steps = std::max(1, static_cast<int>(platforms.size()) * 4);
  int completed_steps = 0;
  const auto report_progress = [&](const Platform &platform, const char *stage) {
    if (!progress) return;
    const int percent = 8 + (completed_steps * 84) / total_steps;
    progress({std::clamp(percent, 8, 92),
              std::string(stage) + " " + platform.display_name});
  };

  for (const Platform &platform : platforms) {
    const auto platform_start = Clock::now();
    report_progress(platform, "准备");
    const ArcadeNameDatabase *arcade_names = nullptr;
    const auto database = arcade_name_databases.find(platform.arcade_name_database_path);
    if (database != arcade_name_databases.end()) arcade_names = &database->second;
    std::vector<ScanRoot> roots;
    roots.reserve(platform.rom_directories.size());
    std::unordered_set<std::string> cached_roots;
    int platform_cached_games = 0;
    for (const std::string &directory : platform.rom_directories) {
      if (CachedRootIsFresh(cache, platform, directory)) {
        cached_roots.insert(directory);
        ++report.skipped_roots;
      } else {
        roots.push_back({directory});
      }
    }

    for (const Game &game : cache.games) {
      if (game.platform_id != platform.id) continue;
      for (const std::string &root : cached_roots) {
        if (PathIsInside(fs::u8path(game.primary_target.path), fs::u8path(root))) {
          MergeGame(game, &report, &index_by_id);
          ++report.cached_games;
          ++platform_cached_games;
          break;
        }
      }
    }

    if (roots.empty()) {
      completed_steps += 4;
      report_progress(platform, "使用缓存");
      std::cerr << "[perf] library.platform id=" << platform.id
                << " ms=" << ElapsedMs(platform_start)
                << " roots=" << platform.rom_directories.size()
                << " cached_roots=" << cached_roots.size()
                << " scan_roots=0 cached_games=" << platform_cached_games
                << " pegasus_ms=0 rom_ms=0 es_ms=0 anbernic_ms=0"
                << '\n';
      continue;
    }

    std::vector<ScanRoot> pegasus_roots;
    std::vector<ScanRoot> es_roots;
    std::vector<ScanRoot> fallback_roots;
    pegasus_roots.reserve(roots.size());
    es_roots.reserve(roots.size());
    fallback_roots.reserve(roots.size());
    for (const ScanRoot &root : roots) {
      const fs::path root_path = fs::u8path(root.path);
      if (HasMetadataFile(root_path, "metadata.pegasus.txt")) {
        pegasus_roots.push_back(root);
      } else if (HasMetadataFile(root_path, "gamelist.xml")) {
        es_roots.push_back(root);
      } else {
        fallback_roots.push_back(root);
      }
    }

    report_progress(platform, "读取天马数据");
    const auto pegasus_start = Clock::now();
    ScanResult pegasus = pegasus_provider.Scan(platform, pegasus_roots);
    RemoveEmptyArchives(&pegasus);
    RemoveArcadeSupportArchives(platform, &pegasus);
    ApplyArcadeNames(arcade_names, &pegasus);
    const long long pegasus_ms = ElapsedMs(pegasus_start);
    report.pegasus_games += static_cast<int>(pegasus.games.size());
    AppendWarnings(pegasus, &report);
    for (const Game &game : pegasus.games) MergeGame(game, &report, &index_by_id);
    ++completed_steps;

    report_progress(platform, "扫描ROM");
    const auto rom_start = Clock::now();
    ScanResult roms = rom_provider.Scan(platform, fallback_roots);
    RemoveEmptyArchives(&roms);
    RemoveArcadeSupportArchives(platform, &roms);
    ApplyArcadeNames(arcade_names, &roms);
    const long long rom_ms = ElapsedMs(rom_start);
    report.rom_directory_games += static_cast<int>(roms.games.size());
    AppendWarnings(roms, &report);
    for (const Game &game : roms.games) MergeGame(game, &report, &index_by_id);
    ++completed_steps;

    report_progress(platform, "读取ES数据");
    const auto es_start = Clock::now();
    ScanResult es = es_provider.Scan(platform, es_roots);
    RemoveEmptyArchives(&es);
    RemoveArcadeSupportArchives(platform, &es);
    ApplyArcadeNames(arcade_names, &es);
    const long long es_ms = ElapsedMs(es_start);
    report.emulationstation_games += static_cast<int>(es.games.size());
    AppendWarnings(es, &report);
    for (const Game &game : es.games) MergeGame(game, &report, &index_by_id);
    ++completed_steps;

    report_progress(platform, "读取图片数据");
    const auto anbernic_start = Clock::now();
    ScanResult anbernic = anbernic_provider.Scan(platform, fallback_roots);
    RemoveEmptyArchives(&anbernic);
    RemoveArcadeSupportArchives(platform, &anbernic);
    ApplyArcadeNames(arcade_names, &anbernic);
    const long long anbernic_ms = ElapsedMs(anbernic_start);
    report.anbernic_games += static_cast<int>(anbernic.games.size());
    AppendWarnings(anbernic, &report);
    for (const Game &game : anbernic.games) MergeGame(game, &report, &index_by_id);
    ++completed_steps;
    std::cerr << "[perf] library.platform id=" << platform.id
              << " ms=" << ElapsedMs(platform_start)
              << " roots=" << platform.rom_directories.size()
              << " cached_roots=" << cached_roots.size()
              << " scan_roots=" << roots.size()
              << " cached_games=" << platform_cached_games
              << " pegasus_ms=" << pegasus_ms
              << " pegasus_games=" << pegasus.games.size()
              << " rom_ms=" << rom_ms
              << " rom_games=" << roms.games.size()
              << " es_ms=" << es_ms
              << " es_games=" << es.games.size()
              << " anbernic_ms=" << anbernic_ms
              << " anbernic_games=" << anbernic.games.size()
              << '\n';
  }

  if (progress) progress({94, "整理游戏列表"});
  const auto sort_start = Clock::now();
  std::sort(platforms.begin(), platforms.end(), [](const Platform &left,
                                                   const Platform &right) {
    if (left.sort_order != right.sort_order) return left.sort_order < right.sort_order;
    return left.display_name < right.display_name;
  });
  std::stable_sort(report.library.games.begin(), report.library.games.end(),
                   [](const Game &left, const Game &right) {
    if (left.platform_id != right.platform_id) return left.platform_id < right.platform_id;
    if (left.sort_key != right.sort_key) return left.sort_key < right.sort_key;
    return left.title < right.title;
  });
  std::cerr << "[perf] library.sort ms=" << ElapsedMs(sort_start)
            << " games=" << report.library.games.size()
            << " platforms=" << platforms.size() << '\n';
  report.library.platforms = std::move(platforms);
  if (progress) progress({97, "写入扫描缓存"});
  const auto write_start = Clock::now();
  WriteScanCache(report.library, cache_path, &report);
  std::uintmax_t cache_bytes = 0;
  if (!cache_path.empty()) {
    std::error_code error;
    cache_bytes = fs::file_size(fs::u8path(cache_path), error);
    if (error) cache_bytes = 0;
  }
  std::cerr << "[perf] library.cache_write ms=" << ElapsedMs(write_start)
            << " records=" << report.cache_records_written
            << " bytes=" << cache_bytes << '\n';
  if (progress) progress({100, "准备进入游戏库"});
  std::cerr << "[perf] library.build_total ms=" << ElapsedMs(total_start)
            << " games=" << report.library.games.size()
            << " cached_games=" << report.cached_games
            << " skipped_roots=" << report.skipped_roots
            << " warnings=" << report.warnings.size() << '\n';
  return report;
}

bool LibraryBuilder::CanRestoreFromCache(const std::vector<Platform> &platforms,
                                         const std::string &cache_path) const {
  const auto total_start = Clock::now();
  std::vector<Platform> expanded_platforms = platforms;
  ExpandPegasusPackageRoots(&expanded_platforms);
  const CachedRootSummary cache = LoadScanCacheRootSummary(cache_path);
  int checked_roots = 0;
  int stale_roots = 0;
  bool fresh = cache.version_ok && cache.has_games && !cache.roots.empty();
  std::unordered_set<std::string> expected_roots;
  for (const Platform &platform : expanded_platforms) {
    for (const std::string &directory : platform.rom_directories) {
      ++checked_roots;
      expected_roots.insert(platform.id + "\n" + NormalizedPath(fs::u8path(directory)).u8string());
      if (!CachedRootIsFresh(cache, platform, directory)) {
        ++stale_roots;
        fresh = false;
      }
    }
  }
  for (const CachedRoot &root : cache.roots) {
    const std::string key = root.platform_id + "\n" +
                            NormalizedPath(fs::u8path(root.root)).u8string();
    if (expected_roots.find(key) == expected_roots.end()) {
      ++stale_roots;
      fresh = false;
    }
  }
  std::cerr << "[perf] library.cache_fresh_check ms=" << ElapsedMs(total_start)
            << " fresh=" << (fresh ? 1 : 0)
            << " checked_roots=" << checked_roots
            << " stale_roots=" << stale_roots
            << " cache_roots=" << cache.roots.size()
            << " has_games=" << (cache.has_games ? 1 : 0) << '\n';
  return fresh;
}

LibraryBuildReport LibraryBuilder::RestoreFromCache(std::vector<Platform> platforms,
                                                    const std::string &cache_path) const {
  const auto total_start = Clock::now();
  LibraryBuildReport report;
  ExpandPegasusPackageRoots(&platforms, &report.warnings);
  const auto cache_start = Clock::now();
  const CachedLibrary cache = LoadScanCache(cache_path);
  std::cerr << "[perf] library.restore_cache_load ms=" << ElapsedMs(cache_start)
            << " roots=" << cache.roots.size()
            << " games=" << cache.games.size()
            << " path=" << cache_path << '\n';
  if (cache.games.empty()) {
    report.library.platforms = std::move(platforms);
    std::cerr << "[perf] library.restore_total ms=" << ElapsedMs(total_start)
              << " games=0 cached_games=0 skipped_roots=0\n";
    return report;
  }

  std::unordered_set<std::string> platform_ids;
  for (const Platform &platform : platforms) {
    platform_ids.insert(platform.id);
  }
  for (const Game &game : cache.games) {
    if (platform_ids.find(game.platform_id) == platform_ids.end()) continue;
    report.library.games.push_back(game);
    ++report.cached_games;
  }

  std::sort(platforms.begin(), platforms.end(), [](const Platform &left,
                                                   const Platform &right) {
    if (left.sort_order != right.sort_order) return left.sort_order < right.sort_order;
    return left.display_name < right.display_name;
  });
  std::stable_sort(report.library.games.begin(), report.library.games.end(),
                   [](const Game &left, const Game &right) {
    if (left.platform_id != right.platform_id) return left.platform_id < right.platform_id;
    if (left.sort_key != right.sort_key) return left.sort_key < right.sort_key;
    return left.title < right.title;
  });
  report.library.platforms = std::move(platforms);
  report.skipped_roots = static_cast<int>(cache.roots.size());
  std::cerr << "[perf] library.restore_total ms=" << ElapsedMs(total_start)
            << " games=" << report.library.games.size()
            << " cached_games=" << report.cached_games
            << " skipped_roots=" << report.skipped_roots << '\n';
  return report;
}

}  // namespace mpl

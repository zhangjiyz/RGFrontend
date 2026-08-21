#pragma once

#include <functional>
#include <string>
#include <vector>

#include "catalog/scan_result.h"
#include "domain/library.h"

namespace mpl {

struct LibraryBuildReport {
  Library library;
  int rom_directory_games = 0;
  int anbernic_games = 0;
  int emulationstation_games = 0;
  int pegasus_games = 0;
  int merged_duplicates = 0;
  int cached_games = 0;
  int skipped_roots = 0;
  int cache_records_written = 0;
  std::vector<std::string> warnings;
};

struct LibraryBuildProgress {
  int percent = 0;
  std::string message;
};

class LibraryBuilder {
 public:
  using ProgressCallback = std::function<void(const LibraryBuildProgress &)>;

  LibraryBuildReport Build(std::vector<Platform> platforms,
                           const std::string &cache_path = {},
                           ProgressCallback progress = {}) const;
  bool CanRestoreFromCache(const std::vector<Platform> &platforms,
                           const std::string &cache_path = {}) const;
  LibraryBuildReport RestoreFromCache(std::vector<Platform> platforms,
                                      const std::string &cache_path = {}) const;
};

}  // namespace mpl

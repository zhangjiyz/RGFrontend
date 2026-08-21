#pragma once

#include <string>
#include <vector>

#include "domain/game.h"

namespace mpl {

struct ScanRoot {
  std::string path;
};

struct ScanResult {
  std::vector<Game> games;
  int scanned_files = 0;
  int metadata_files = 0;
  int parsed_entries = 0;
  int missing_entries = 0;
  int duplicate_entries = 0;
  int rejected_paths = 0;
  std::vector<std::string> warnings;
};

}  // namespace mpl

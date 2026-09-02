#pragma once

#include <string>
#include <vector>

#include "catalog/scan_result.h"
#include "domain/platform.h"

namespace mpl {

struct PegasusPackageInfo {
  std::string root_path;
  std::string metadata_path;
  std::string collection_title;
  std::vector<std::string> extensions;
  std::vector<std::string> platform_hints;
};

class PegasusProvider {
 public:
  std::vector<PegasusPackageInfo> DiscoverPackages(
      const std::vector<std::string> &rom_roots) const;
  ScanResult Scan(const Platform &platform, const std::vector<ScanRoot> &roots) const;
};

}  // namespace mpl

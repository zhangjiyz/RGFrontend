#pragma once

#include <vector>

#include "catalog/scan_result.h"
#include "domain/platform.h"

namespace mpl {

class PegasusProvider {
 public:
  ScanResult Scan(const Platform &platform, const std::vector<ScanRoot> &roots) const;
};

}  // namespace mpl

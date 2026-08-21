#include "domain/library.h"

namespace mpl {

const Platform *FindPlatform(const Library &library, const std::string &platform_id) {
  for (const Platform &platform : library.platforms) {
    if (platform.id == platform_id) return &platform;
  }
  return nullptr;
}

}  // namespace mpl

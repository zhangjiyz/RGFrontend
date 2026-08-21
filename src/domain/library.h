#pragma once

#include <string>
#include <vector>

#include "domain/game.h"
#include "domain/platform.h"

namespace mpl {

struct Library {
  std::vector<Platform> platforms;
  std::vector<Game> games;
};

const Platform *FindPlatform(const Library &library, const std::string &platform_id);

}  // namespace mpl

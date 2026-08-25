#pragma once

#include <string>
#include <unordered_map>

#include "domain/game.h"

namespace mpl {

class ArcadeNameDatabase {
 public:
  bool Load(const std::string &path);
  bool Apply(Game *game) const;
  bool empty() const { return titles_.empty(); }

 private:
  std::unordered_map<std::string, std::string> titles_;
};

}  // namespace mpl

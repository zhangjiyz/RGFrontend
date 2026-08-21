#pragma once

#include <string>

#include "domain/game.h"

namespace mpl {

LaunchHint ResolveLaunchHint(const std::string &raw_hint);

const char *LaunchHintKindName(LaunchHintKind kind);

}  // namespace mpl

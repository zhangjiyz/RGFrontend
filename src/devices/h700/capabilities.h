#pragma once

#include "devices/device_capabilities.h"
#include "devices/h700/platform_registry.h"

namespace mpl {

DeviceCapabilities LoadH700Capabilities(const H700RegistryOptions &options);

}  // namespace mpl

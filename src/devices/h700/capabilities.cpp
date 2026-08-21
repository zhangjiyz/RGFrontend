#include "devices/h700/capabilities.h"

namespace mpl {

DeviceCapabilities LoadH700Capabilities(const H700RegistryOptions &options) {
  DeviceCapabilities capabilities;
  capabilities.device_id = "h700-stock-linux";
  capabilities.display_name = "H700 Stock Linux";
  for (const std::string &root : options.card_roots) {
    capabilities.content_roots.push_back(ContentRoot{root, true, false});
  }
  capabilities.platforms = LoadH700Platforms(options);
  capabilities.sdl_video_driver = "mali";
  capabilities.sdl_audio_driver = "alsa";
  capabilities.uses_vendor_frontend_lifecycle = true;
  return capabilities;
}

}  // namespace mpl

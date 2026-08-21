#pragma once

#include <string>
#include <vector>

#include "domain/platform.h"

namespace mpl {

struct ContentRoot {
  std::string path;
  bool removable = true;
  bool writable = false;
};

struct DeviceCapabilities {
  std::string device_id;
  std::string display_name;
  std::vector<ContentRoot> content_roots;
  std::vector<Platform> platforms;
  std::string sdl_video_driver;
  std::string sdl_audio_driver;
  bool uses_vendor_frontend_lifecycle = false;
};

}  // namespace mpl

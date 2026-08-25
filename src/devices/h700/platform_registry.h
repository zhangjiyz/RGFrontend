#pragma once

#include <string>
#include <vector>

#include "domain/platform.h"

namespace mpl {

struct H700RegistryOptions {
  std::vector<std::string> card_roots = {"/mnt/mmc", "/mnt/sdcard"};
  std::string retroarch_launcher = "/mnt/vendor/deep/retro/retroarch";
  std::string nds_launcher = "/mnt/vendor/ctrl/setNDS.sh";
  std::string psp_launcher = "/mnt/vendor/deep/ppsspp/PPSSPPSDL";
  std::string openbor_launcher = "/mnt/vendor/deep/openBOR/OpenBOR.dge";
  std::string openbor_setup_script = "/mnt/vendor/deep/openBOR/scripts/openbor.sh";
  std::string ports_shell = "/bin/bash";
  std::string java_launcher = "/mnt/vendor/deep/emuJava/launch.sh";
  std::string saturn_launcher = "/mnt/vendor/ctrl/setSaturn.sh";
  std::string saturn_emulator = "/emuelec/saturn/yabasanshiro";
  std::string saturn_bios = "/emuelec/saturn/bios/saturn_bios.bin";
  std::string arcade_name_database = "/mnt/vendor/bin/arcade-plus.csv";
  bool enable_saturn = true;
};

std::vector<Platform> LoadH700Platforms(const H700RegistryOptions &options);

}  // namespace mpl

#pragma once

#include <string>
#include <vector>

#include "launch/launcher_adapter.h"

namespace mpl {

struct H700LaunchRequestAdapterOptions {
  std::string request_path;
  std::string retroarch_launcher = "/mnt/mod/ctrl/RA_launch.sh";
  std::string nds_launcher = "/mnt/vendor/ctrl/setNDS.sh";
  std::string psp_launcher = "/mnt/vendor/deep/ppsspp/PPSSPPSDL";
  std::string openbor_launcher = "/mnt/vendor/deep/openBOR/OpenBOR.dge";
  std::string openbor_setup_script = "/mnt/vendor/deep/openBOR/scripts/openbor.sh";
  std::string ports_shell = "/bin/bash";
  std::string java_launcher = "/mnt/vendor/deep/emuJava/launch.sh";
  std::string saturn_launcher = "/mnt/vendor/ctrl/setSaturn.sh";
  std::string saturn_emulator = "/emuelec/saturn/yabasanshiro";
  std::string saturn_bios = "/emuelec/saturn/bios/saturn_bios.bin";
  std::vector<std::string> trusted_roots;
};

class H700LaunchRequestAdapter : public LauncherAdapter {
 public:
  explicit H700LaunchRequestAdapter(H700LaunchRequestAdapterOptions options);

  LaunchCapability Probe(const Platform &platform) const override;
  LaunchResult PrepareLaunch(const Platform &platform, const Game &game,
                             const std::string &rom_path) const override;

 private:
  H700LaunchRequestAdapterOptions options_;
};

}  // namespace mpl

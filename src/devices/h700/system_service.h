#pragma once

#include <string>

#include "devices/system_service.h"

namespace mpl {

struct H700SystemServiceOptions {
  std::string state_dir = "/mnt/data/multiplatform-launcher";
  std::string app_dir;
  std::string root_prefix;
  bool allow_runtime_writes = true;
};

class H700SystemService : public SystemService {
 public:
  explicit H700SystemService(H700SystemServiceOptions options);

  SystemStatus ReadStatus() const override;
  int ReadHallState() const override;
  bool ChangeBrightness(int delta, SystemStatus *status) override;
  bool ChangeVolume(int delta, SystemStatus *status) override;
  bool SetAutostart(bool enabled, SystemStatus *status) override;
  bool RestoreVolume() const;
  bool SyncStoredVolumeToSystem() const;

 private:
  std::string SystemPath(const std::string &absolute_path) const;
  std::string AppPath(const std::string &relative_path) const;
  int ReadInt(const std::string &path, int fallback) const;
  bool WriteInt(const std::string &path, int value) const;
  bool AutostartHelperAvailable() const;
  bool AutostartEnabled() const;
  bool ApplyBrightness(int level) const;
  bool ApplyVolume(int level) const;

  H700SystemServiceOptions options_;
};

}  // namespace mpl

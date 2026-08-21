#pragma once

#include <string>

namespace mpl {

struct SystemStatus {
  int battery_percent = -1;
  bool charging = false;
  int brightness = -1;
  int volume = -1;
  int autostart_enabled = -1;
};

class SystemService {
 public:
  virtual ~SystemService() = default;

  virtual SystemStatus ReadStatus() const = 0;
  virtual int ReadHallState() const { return -1; }
  virtual bool ChangeBrightness(int delta, SystemStatus *status) = 0;
  virtual bool ChangeVolume(int delta, SystemStatus *status) = 0;
  virtual bool SetAutostart(bool enabled, SystemStatus *status) = 0;
};

}  // namespace mpl

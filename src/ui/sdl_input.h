#pragma once

#include <vector>

#include "ui/ui_model.h"

union SDL_Event;
struct _SDL_GameController;
struct _SDL_Joystick;
typedef struct _SDL_GameController SDL_GameController;
typedef struct _SDL_Joystick SDL_Joystick;

namespace mpl {

bool IsSdlQuitEvent(const SDL_Event &event);
bool MapSdlEventToUiAction(const SDL_Event &event, UiAction *action);

class SdlInputRouter {
 public:
  SdlInputRouter();
  ~SdlInputRouter();

  SdlInputRouter(const SdlInputRouter &) = delete;
  SdlInputRouter &operator=(const SdlInputRouter &) = delete;

  void Close();
  bool PollDeviceAction(UiAction *action);
  bool Translate(const SDL_Event &event, UiAction *action);

 private:
  void OpenEvdevInputs();

  SDL_GameController *controller_ = nullptr;
  SDL_Joystick *joystick_ = nullptr;
  std::vector<int> evdev_input_fds_;
  bool has_evdev_gamepad_ = false;
  bool input_debug_ = false;
  int horizontal_axis_direction_ = 0;
  int vertical_axis_direction_ = 0;
};

}  // namespace mpl

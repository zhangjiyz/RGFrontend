#include "ui/sdl_input.h"

#include <SDL.h>

#include <cstdlib>
#include <iostream>
#include <string>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/input.h>
#include <sys/ioctl.h>
#endif
#endif

namespace mpl {

SdlInputRouter::SdlInputRouter() {
  input_debug_ = std::getenv("MPL_INPUT_DEBUG") != nullptr;
  OpenEvdevInputs();
  SDL_GameControllerEventState(SDL_ENABLE);
  SDL_JoystickEventState(SDL_ENABLE);
  for (int index = 0; index < SDL_NumJoysticks(); ++index) {
    if (!SDL_IsGameController(index)) continue;
    controller_ = SDL_GameControllerOpen(index);
    if (controller_) break;
  }
  if (!controller_ && SDL_NumJoysticks() > 0) {
    joystick_ = SDL_JoystickOpen(0);
  }
}

SdlInputRouter::~SdlInputRouter() {
  Close();
}

void SdlInputRouter::Close() {
#ifndef _WIN32
  for (int fd : evdev_input_fds_) {
    if (fd >= 0) close(fd);
  }
  evdev_input_fds_.clear();
#endif
  if (controller_) SDL_GameControllerClose(controller_);
  if (joystick_) SDL_JoystickClose(joystick_);
  controller_ = nullptr;
  joystick_ = nullptr;
}

bool IsSdlQuitEvent(const SDL_Event &event) {
  return event.type == SDL_QUIT;
}

bool IsSdlGamepadEvent(const SDL_Event &event) {
  return event.type == SDL_CONTROLLERBUTTONDOWN ||
         event.type == SDL_CONTROLLERBUTTONUP ||
         event.type == SDL_CONTROLLERAXISMOTION ||
         event.type == SDL_JOYBUTTONDOWN ||
         event.type == SDL_JOYBUTTONUP ||
         event.type == SDL_JOYAXISMOTION;
}

bool MapSdlEventToUiAction(const SDL_Event &event, UiAction *action) {
  if (!action) return false;

  if (event.type == SDL_KEYUP) {
    if (event.key.keysym.sym == SDLK_m) {
      *action = UiAction::MenuRelease;
      return true;
    }
    return false;
  }

  if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
    switch (event.key.keysym.sym) {
      case SDLK_UP:
      case SDLK_w:
        *action = UiAction::Up;
        return true;
      case SDLK_DOWN:
      case SDLK_s:
        *action = UiAction::Down;
        return true;
      case SDLK_LEFT:
      case SDLK_a:
        *action = UiAction::Left;
        return true;
      case SDLK_RIGHT:
      case SDLK_d:
        *action = UiAction::Right;
        return true;
      case SDLK_RETURN:
      case SDLK_SPACE:
        *action = UiAction::Confirm;
        return true;
      case SDLK_ESCAPE:
      case SDLK_BACKSPACE:
        *action = UiAction::Back;
        return true;
      case SDLK_x:
        *action = UiAction::ToggleTitles;
        return true;
      case SDLK_f:
        *action = UiAction::ToggleFavorite;
        return true;
      case SDLK_y:
        *action = UiAction::OpenCoreSelect;
        return true;
      case SDLK_g:
        *action = UiAction::ToggleFullscreenGrid;
        return true;
      case SDLK_MINUS:
        *action = UiAction::GridSmaller;
        return true;
      case SDLK_EQUALS:
      case SDLK_PLUS:
        *action = UiAction::GridLarger;
        return true;
      case SDLK_t:
        *action = UiAction::QuickTheme;
        return true;
      case SDLK_b:
        *action = UiAction::NextBgm;
        return true;
      case SDLK_TAB:
      case SDLK_RIGHTBRACKET:
        *action = UiAction::TabNext;
        return true;
      case SDLK_LEFTBRACKET:
        *action = UiAction::TabPrevious;
        return true;
      case SDLK_PAGEUP:
        *action = UiAction::PagePrevious;
        return true;
      case SDLK_PAGEDOWN:
        *action = UiAction::PageNext;
        return true;
      case SDLK_m:
      case SDLK_RETURN2:
        *action = UiAction::MenuPress;
        return true;
      case SDLK_POWER:
        *action = UiAction::Power;
        return true;
      default:
        return false;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN) {
    switch (event.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_DPAD_UP:
        *action = UiAction::Up;
        return true;
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        *action = UiAction::Down;
        return true;
      case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        *action = UiAction::Left;
        return true;
      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        *action = UiAction::Right;
        return true;
      case SDL_CONTROLLER_BUTTON_A:
        *action = UiAction::Confirm;
        return true;
      case SDL_CONTROLLER_BUTTON_B:
        *action = UiAction::Back;
        return true;
      case SDL_CONTROLLER_BUTTON_X:
        *action = UiAction::ToggleTitles;
        return true;
      case SDL_CONTROLLER_BUTTON_Y:
        *action = UiAction::OpenCoreSelect;
        return true;
      case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        *action = UiAction::TabPrevious;
        return true;
      case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        *action = UiAction::TabNext;
        return true;
      case SDL_CONTROLLER_BUTTON_START:
        *action = UiAction::PageNext;
        return true;
      case SDL_CONTROLLER_BUTTON_BACK:
        *action = UiAction::PagePrevious;
        return true;
      default:
        return false;
    }
  }

  if (event.type == SDL_JOYBUTTONDOWN) {
    switch (event.jbutton.button) {
      case 0:
        *action = UiAction::Confirm;
        return true;
      case 1:
        *action = UiAction::Back;
        return true;
      case 2:
        *action = UiAction::ToggleTitles;
        return true;
      case 3:
        *action = UiAction::OpenCoreSelect;
        return true;
      case 4:
        *action = UiAction::TabPrevious;
        return true;
      case 5:
        *action = UiAction::TabNext;
        return true;
      case 8:
        *action = UiAction::PagePrevious;
        return true;
      case 9:
        *action = UiAction::PageNext;
        return true;
      default:
        return false;
    }
  }

  return false;
}

void SdlInputRouter::OpenEvdevInputs() {
#ifdef __linux__
  for (int index = 0; index < 8; ++index) {
    const std::string path = "/dev/input/event" + std::to_string(index);
    const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) continue;
    char name[128] = {};
    const bool named = ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0;
    const std::string device_name = named ? std::string(name) : std::string();
    const bool input_source =
        device_name == "axp2202-pek" || device_name == "dierct-keys-polled" ||
        device_name == "ANBERNIC-keys";
    if (!input_source) {
      close(fd);
      continue;
    }
    if (device_name == "ANBERNIC-keys") has_evdev_gamepad_ = true;
    evdev_input_fds_.push_back(fd);
    if (input_debug_) {
      std::cerr << "[input] evdev input=" << path << " name=" << device_name << '\n';
    }
  }
#endif
}

bool SdlInputRouter::PollDeviceAction(UiAction *action) {
  if (!action) return false;
#ifdef __linux__
  for (int fd : evdev_input_fds_) {
    input_event event{};
    while (read(fd, &event, sizeof(event)) == sizeof(event)) {
      if (input_debug_ && (event.type == EV_KEY || event.type == EV_ABS)) {
        std::cerr << "[input] evdev type=" << event.type
                  << " code=" << event.code
                  << " value=" << event.value << '\n';
      }
      if (event.type == EV_KEY) {
        if (event.value != 1) continue;
        switch (event.code) {
          case BTN_SOUTH:
            *action = UiAction::Confirm;
            return true;
          case BTN_EAST:
            *action = UiAction::Back;
            return true;
          case BTN_NORTH:
            *action = UiAction::ToggleTitles;
            return true;
#ifdef BTN_C
          case BTN_C:
            *action = UiAction::OpenCoreSelect;
            return true;
#endif
          case BTN_WEST:
            *action = UiAction::TabPrevious;
            return true;
          case BTN_Z:
            *action = UiAction::TabNext;
            return true;
          case BTN_TR:
            *action = UiAction::Menu;
            return true;
          case BTN_TL:
            *action = UiAction::ToggleFavorite;
            return true;
          case BTN_SELECT:
            *action = UiAction::PagePrevious;
            return true;
          case BTN_START:
            *action = UiAction::PageNext;
            return true;
          case KEY_VOLUMEDOWN:
            *action = UiAction::AdjustVolumeDown;
            return true;
          case KEY_VOLUMEUP:
            *action = UiAction::AdjustVolumeUp;
            return true;
          case KEY_POWER:
            *action = UiAction::Power;
            return true;
          default:
            break;
        }
      } else if (event.type == EV_ABS) {
        if (event.code == ABS_HAT0X) {
          if (event.value < 0) {
            *action = UiAction::Left;
            return true;
          }
          if (event.value > 0) {
            *action = UiAction::Right;
            return true;
          }
        } else if (event.code == ABS_HAT0Y) {
          if (event.value < 0) {
            *action = UiAction::Up;
            return true;
          }
          if (event.value > 0) {
            *action = UiAction::Down;
            return true;
          }
        }
      }
    }
  }
#endif
  return false;
}

bool SdlInputRouter::Translate(const SDL_Event &event, UiAction *action) {
  if (has_evdev_gamepad_ && event.type == SDL_JOYBUTTONDOWN && event.jbutton.button == 8) {
    if (action) *action = UiAction::MenuPress;
    return action != nullptr;
  }
  if (has_evdev_gamepad_ && event.type == SDL_JOYBUTTONUP && event.jbutton.button == 8) {
    if (action) *action = UiAction::MenuRelease;
    return action != nullptr;
  }
  if (has_evdev_gamepad_ && IsSdlGamepadEvent(event)) return false;
  if (input_debug_ && event.type == SDL_CONTROLLERBUTTONDOWN) {
    std::cerr << "[input] controller button=" << static_cast<int>(event.cbutton.button) << '\n';
  } else if (input_debug_ && event.type == SDL_JOYBUTTONDOWN) {
    std::cerr << "[input] joy button=" << static_cast<int>(event.jbutton.button) << '\n';
  }
  if (MapSdlEventToUiAction(event, action)) return true;
  if (!action || (event.type != SDL_CONTROLLERAXISMOTION && event.type != SDL_JOYAXISMOTION)) {
    return false;
  }
  const int axis = event.type == SDL_CONTROLLERAXISMOTION ? event.caxis.axis : event.jaxis.axis;
  const int value = event.type == SDL_CONTROLLERAXISMOTION ? event.caxis.value : event.jaxis.value;
  if (axis != SDL_CONTROLLER_AXIS_LEFTX && axis != SDL_CONTROLLER_AXIS_LEFTY) {
    return false;
  }

  constexpr int kThreshold = 17000;
  const int direction = value > kThreshold ? 1 : (value < -kThreshold ? -1 : 0);
  if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
    if (direction != 0 && direction != horizontal_axis_direction_) {
      horizontal_axis_direction_ = direction;
      *action = direction < 0 ? UiAction::Left : UiAction::Right;
      return true;
    }
    if (direction == 0) horizontal_axis_direction_ = 0;
  } else {
    if (direction != 0 && direction != vertical_axis_direction_) {
      vertical_axis_direction_ = direction;
      *action = direction < 0 ? UiAction::Up : UiAction::Down;
      return true;
    }
    if (direction == 0) vertical_axis_direction_ = 0;
  }
  return false;
}

}  // namespace mpl

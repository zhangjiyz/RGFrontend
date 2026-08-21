#include "ui/sdl_input.h"

#include <SDL.h>

#include <cassert>

using namespace mpl;

namespace {

UiAction KeyAction(SDL_Keycode key) {
  SDL_Event event{};
  event.type = SDL_KEYDOWN;
  event.key.repeat = 0;
  event.key.keysym.sym = key;
  UiAction action = UiAction::Menu;
  assert(MapSdlEventToUiAction(event, &action));
  return action;
}

UiAction ButtonAction(Uint8 button) {
  SDL_Event event{};
  event.type = SDL_CONTROLLERBUTTONDOWN;
  event.cbutton.button = button;
  UiAction action = UiAction::Menu;
  assert(MapSdlEventToUiAction(event, &action));
  return action;
}

UiAction JoyButtonAction(Uint8 button) {
  SDL_Event event{};
  event.type = SDL_JOYBUTTONDOWN;
  event.jbutton.button = button;
  UiAction action = UiAction::Confirm;
  assert(MapSdlEventToUiAction(event, &action));
  return action;
}

}  // namespace

int main() {
  assert(KeyAction(SDLK_RIGHT) == UiAction::Right);
  assert(KeyAction(SDLK_RETURN) == UiAction::Confirm);
  assert(KeyAction(SDLK_ESCAPE) == UiAction::Back);
  assert(KeyAction(SDLK_x) == UiAction::ToggleTitles);
  assert(KeyAction(SDLK_f) == UiAction::ToggleFavorite);
  assert(KeyAction(SDLK_y) == UiAction::OpenCoreSelect);
  assert(KeyAction(SDLK_g) == UiAction::ToggleFullscreenGrid);
  assert(KeyAction(SDLK_MINUS) == UiAction::GridSmaller);
  assert(KeyAction(SDLK_EQUALS) == UiAction::GridLarger);
  assert(KeyAction(SDLK_t) == UiAction::QuickTheme);
  assert(KeyAction(SDLK_b) == UiAction::NextBgm);
  assert(KeyAction(SDLK_TAB) == UiAction::TabNext);
  assert(KeyAction(SDLK_RIGHTBRACKET) == UiAction::TabNext);
  assert(KeyAction(SDLK_LEFTBRACKET) == UiAction::TabPrevious);
  assert(KeyAction(SDLK_PAGEUP) == UiAction::PagePrevious);
  assert(KeyAction(SDLK_PAGEDOWN) == UiAction::PageNext);
  assert(KeyAction(SDLK_m) == UiAction::MenuPress);
  assert(KeyAction(SDLK_POWER) == UiAction::Power);

  SDL_Event repeated{};
  repeated.type = SDL_KEYDOWN;
  repeated.key.repeat = 1;
  repeated.key.keysym.sym = SDLK_RIGHT;
  UiAction ignored = UiAction::Menu;
  assert(!MapSdlEventToUiAction(repeated, &ignored));

  assert(ButtonAction(SDL_CONTROLLER_BUTTON_DPAD_LEFT) == UiAction::Left);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_A) == UiAction::Confirm);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_B) == UiAction::Back);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_X) == UiAction::ToggleTitles);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_Y) == UiAction::OpenCoreSelect);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) == UiAction::TabNext);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_START) == UiAction::PageNext);
  assert(ButtonAction(SDL_CONTROLLER_BUTTON_BACK) == UiAction::PagePrevious);

  assert(JoyButtonAction(0) == UiAction::Confirm);
  assert(JoyButtonAction(1) == UiAction::Back);
  assert(JoyButtonAction(2) == UiAction::ToggleTitles);
  assert(JoyButtonAction(3) == UiAction::OpenCoreSelect);
  assert(JoyButtonAction(4) == UiAction::TabPrevious);
  assert(JoyButtonAction(5) == UiAction::TabNext);
  assert(JoyButtonAction(8) == UiAction::PagePrevious);
  assert(JoyButtonAction(9) == UiAction::PageNext);

  SDL_Event quit{};
  quit.type = SDL_QUIT;
  assert(IsSdlQuitEvent(quit));

  SdlInputRouter router;
  SDL_Event axis{};
  axis.type = SDL_CONTROLLERAXISMOTION;
  axis.caxis.axis = SDL_CONTROLLER_AXIS_LEFTX;
  axis.caxis.value = 20000;
  UiAction action = UiAction::Menu;
  assert(router.Translate(axis, &action));
  assert(action == UiAction::Right);
  assert(!router.Translate(axis, &action));
  axis.caxis.value = 0;
  assert(!router.Translate(axis, &action));
  axis.caxis.value = -20000;
  assert(router.Translate(axis, &action));
  assert(action == UiAction::Left);

  SDL_Event joy_axis{};
  joy_axis.type = SDL_JOYAXISMOTION;
  joy_axis.jaxis.axis = 1;
  joy_axis.jaxis.value = 20000;
  assert(router.Translate(joy_axis, &action));
  assert(action == UiAction::Down);
  router.Close();

  return 0;
}

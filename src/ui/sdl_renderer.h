#pragma once

#include <functional>
#include <string>

#include "ui/layout.h"
#include "ui/ui_model.h"

struct SDL_Renderer;

namespace mpl {

void SetRendererFontPath(std::string path);
void SetRendererMediaRoots(std::string app_dir, std::string state_dir);
void SetRendererAudioDeviceOpenedCallback(std::function<void()> callback);
void ClearRendererMediaCache();
bool RenderLibraryView(SDL_Renderer *renderer, const UiSession &session,
                       const UiLayout &layout);

}  // namespace mpl

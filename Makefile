CXX ?= c++
TARGET_DIR := build
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS += -Isrc -MMD -MP
SDL2_CONFIG ?= sdl2-config
SDL_CFLAGS := $(shell $(SDL2_CONFIG) --cflags 2>/dev/null)
SDL_LIBS := $(shell $(SDL2_CONFIG) --libs 2>/dev/null)
SDL_TTF_LIBS ?= -lSDL2_ttf
SDL_IMAGE_LIBS ?= -lSDL2_image

CORE_SOURCES := \
	src/catalog/launch_hint_resolver.cpp \
	src/catalog/library_builder.cpp \
	src/catalog/path.cpp \
	src/catalog/ports_alias.cpp \
	src/catalog/providers/anbernic_provider.cpp \
	src/catalog/providers/emulationstation_provider.cpp \
	src/catalog/providers/pegasus_provider.cpp \
	src/catalog/providers/rom_directory_provider.cpp \
	src/devices/h700/capabilities.cpp \
	src/devices/h700/launch_request_adapter.cpp \
	src/devices/h700/platform_registry.cpp \
	src/devices/h700/retroarch_adapter.cpp \
	src/devices/h700/system_service.cpp \
	src/domain/library.cpp \
	src/launch/launcher_adapter.cpp \
	src/launch/launch_request.cpp \
	src/services/state_store.cpp

UI_SOURCES := \
	src/ui/layout.cpp \
	src/ui/ui_model.cpp

SDL_UI_SOURCES := \
	$(UI_SOURCES) \
	src/ui/media_playback.cpp \
	src/ui/sdl_input.cpp \
	src/ui/sdl_renderer.cpp

APP_SOURCES := \
	src/app/demo_library.cpp \
	src/app/desktop_ui_app.cpp

P0_TEST := $(TARGET_DIR)/p0_test
P1_TEST := $(TARGET_DIR)/p1_test
ANBERNIC_TEST := $(TARGET_DIR)/anbernic_provider_test
LAUNCH_ADAPTER_TEST := $(TARGET_DIR)/launch_adapter_test
LAUNCH_HINT_RESOLVER_TEST := $(TARGET_DIR)/launch_hint_resolver_test
H700_SYSTEM_SERVICE_TEST := $(TARGET_DIR)/h700_system_service_test
UI_MODEL_TEST := $(TARGET_DIR)/ui_model_test
UI_SDL_INPUT_TEST := $(TARGET_DIR)/ui_sdl_input_test
UI_SDL_SMOKE_TEST := $(TARGET_DIR)/ui_sdl_smoke_test
DESKTOP_UI_APP_TEST := $(TARGET_DIR)/desktop_ui_app_test
DESKTOP_DEMO := $(TARGET_DIR)/mpl_desktop_demo
APP_BUNDLE := $(TARGET_DIR)/RGFrontend.app

.PHONY: all test desktop-demo app-bundle clean

all: test

$(P0_TEST): tests/p0_test.cpp $(CORE_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(P1_TEST): tests/p1_test.cpp $(CORE_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(ANBERNIC_TEST): tests/anbernic_provider_test.cpp $(CORE_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(LAUNCH_ADAPTER_TEST): tests/launch_adapter_test.cpp $(CORE_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(LAUNCH_HINT_RESOLVER_TEST): tests/launch_hint_resolver_test.cpp $(CORE_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(H700_SYSTEM_SERVICE_TEST): tests/h700_system_service_test.cpp $(CORE_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(UI_MODEL_TEST): tests/ui_model_test.cpp $(CORE_SOURCES) $(UI_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(UI_SDL_SMOKE_TEST): tests/ui_sdl_smoke_test.cpp $(CORE_SOURCES) $(SDL_UI_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(SDL_CFLAGS) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS) $(SDL_TTF_LIBS) $(SDL_IMAGE_LIBS)

$(UI_SDL_INPUT_TEST): tests/ui_sdl_input_test.cpp $(UI_SOURCES) src/ui/sdl_input.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(SDL_CFLAGS) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS)

$(DESKTOP_UI_APP_TEST): tests/desktop_ui_app_test.cpp $(CORE_SOURCES) $(SDL_UI_SOURCES) $(APP_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(SDL_CFLAGS) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS) $(SDL_TTF_LIBS) $(SDL_IMAGE_LIBS)

$(DESKTOP_DEMO): src/app/main.cpp $(CORE_SOURCES) $(SDL_UI_SOURCES) $(APP_SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(SDL_CFLAGS) $(CXXFLAGS) $^ -o $@ $(SDL_LIBS) $(SDL_TTF_LIBS) $(SDL_IMAGE_LIBS)

desktop-demo: $(DESKTOP_DEMO)

app-bundle: $(DESKTOP_DEMO)
	@mkdir -p "$(APP_BUNDLE)/Contents/MacOS"
	@mkdir -p "$(APP_BUNDLE)/Contents/Resources"
	cp "$(DESKTOP_DEMO)" "$(APP_BUNDLE)/Contents/MacOS/mpl_desktop_demo"
	printf '%s\n' \
	  '<?xml version="1.0" encoding="UTF-8"?>' \
	  '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
	  '<plist version="1.0">' \
	  '<dict>' \
	  '  <key>CFBundleExecutable</key>' \
	  '  <string>mpl_desktop_demo</string>' \
	  '  <key>CFBundleIdentifier</key>' \
	  '  <string>local.rgfrontend.demo</string>' \
	  '  <key>CFBundleName</key>' \
	  '  <string>RGFrontend</string>' \
	  '  <key>CFBundlePackageType</key>' \
	  '  <string>APPL</string>' \
	  '  <key>CFBundleShortVersionString</key>' \
	  '  <string>0.1</string>' \
	  '  <key>LSMinimumSystemVersion</key>' \
	  '  <string>11.0</string>' \
	  '</dict>' \
	  '</plist>' >"$(APP_BUNDLE)/Contents/Info.plist"

test: $(P0_TEST) $(P1_TEST) $(ANBERNIC_TEST) $(LAUNCH_ADAPTER_TEST) $(LAUNCH_HINT_RESOLVER_TEST) $(H700_SYSTEM_SERVICE_TEST) $(UI_MODEL_TEST) $(UI_SDL_INPUT_TEST) $(UI_SDL_SMOKE_TEST) $(DESKTOP_UI_APP_TEST)
	./$(P0_TEST)
	./$(P1_TEST)
	./$(ANBERNIC_TEST)
	./$(LAUNCH_ADAPTER_TEST)
	./$(LAUNCH_HINT_RESOLVER_TEST)
	./$(H700_SYSTEM_SERVICE_TEST)
	./$(UI_MODEL_TEST)
	./$(UI_SDL_INPUT_TEST)
	./$(UI_SDL_SMOKE_TEST)
	./$(DESKTOP_UI_APP_TEST)
	sh tests/shell/h700_launch_request_test.sh
	sh tests/shell/h700_game_volume_test.sh
	sh tests/shell/h700_run_frontend_test.sh
	sh tests/shell/h700_autostart_test.sh
	sh tests/shell/h700_build_in_ubuntu22_test.sh
	sh tests/shell/h700_collect_capabilities_test.sh
	sh tests/shell/h700_system_immutability_test.sh

clean:
	rm -rf $(TARGET_DIR)

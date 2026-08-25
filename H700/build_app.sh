#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

APP_ID="${MPL_H700_APP_ID:-RGFrontend}"
ENTRY_NAME="${MPL_H700_ENTRY_NAME:-RGFrontend}"
FORCE_STOCK_ROUTE="${MPL_H700_FORCE_STOCK_ROUTE:-0}"
BINARY_NAME="${MPL_H700_BINARY_NAME:-mpl_h700_frontend}"
VERSION="${MPL_H700_VERSION:-1.0.2}"
SYSROOT="${MPL_H700_SYSROOT:-$SCRIPT_DIR/sysroot}"
APP_ICON="${MPL_H700_APP_ICON:-$SCRIPT_DIR/assets/apps/RGFrontend.png}"
CLASSIC_ICON="${MPL_H700_CLASSIC_ICON:-$SCRIPT_DIR/assets/apps/classic_icon.png}"
if [ -n "${CROSS_CXX:-}" ]; then
  CXX="$CROSS_CXX"
elif command -v aarch64-linux-gnu-g++-11 >/dev/null 2>&1; then
  CXX="aarch64-linux-gnu-g++-11"
else
  CXX="aarch64-linux-gnu-g++"
fi
PKG_CONFIG_CMD="${CROSS_PKG_CONFIG:-pkg-config}"
READ_ELF="${CROSS_READELF:-aarch64-linux-gnu-readelf}"
TARGET="${MPL_H700_TARGET_TRIPLE:-aarch64-linux-gnu}"
BUILD_DIR="${MPL_H700_BUILD_DIR:-$REPO_ROOT/build/h700}"
DIST_DIR="${MPL_H700_DIST_DIR:-$SCRIPT_DIR/dist_app}"
STAGE_ROOT="$DIST_DIR/release_stage"
RUNTIME_DIR="$STAGE_ROOT/Roms/APPS/$APP_ID"
ENTRY_PATH="$STAGE_ROOT/Roms/APPS/$ENTRY_NAME.sh"
ICON_PATH="$STAGE_ROOT/Roms/APPS/Imgs/$ENTRY_NAME.png"
RUNTIME_ICON_PATH="$RUNTIME_DIR/assets/apps/RGFrontend.png"
RUNTIME_CLASSIC_ICON_PATH="$RUNTIME_DIR/assets/apps/classic_icon.png"
LICENSE_DIR="$RUNTIME_DIR/licenses"
ARCHIVE_PATH="$DIST_DIR/$APP_ID-H700-$VERSION.zip"

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

[ -d "$SYSROOT" ] || fail "sysroot not found: $SYSROOT"
[ -d "$SYSROOT/usr/include" ] || fail "sysroot is missing usr/include: $SYSROOT"
[ -f "$APP_ICON" ] || fail "app icon not found: $APP_ICON"
command -v "$CXX" >/dev/null 2>&1 || fail "cross compiler not found: $CXX"
case "$FORCE_STOCK_ROUTE" in
  0|1) ;;
  *) fail "MPL_H700_FORCE_STOCK_ROUTE must be 0 or 1" ;;
esac

case "$CXX" in
  *clang++*)
    CXX_TARGET_FLAGS="--target=$TARGET --sysroot=$SYSROOT"
    ;;
  *)
    CXX_TARGET_FLAGS="--sysroot=$SYSROOT"
    ;;
esac

SDL_CFLAGS=""
SDL_LIBS=""
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/pkgconfig:$SYSROOT/usr/lib/aarch64-linux-gnu/pkgconfig:$SYSROOT/usr/share/pkgconfig"

if command -v "$PKG_CONFIG_CMD" >/dev/null 2>&1 &&
   "$PKG_CONFIG_CMD" --exists sdl2 SDL2_ttf SDL2_image 2>/dev/null; then
  SDL_CFLAGS="$($PKG_CONFIG_CMD --cflags sdl2 SDL2_ttf SDL2_image)"
  SDL_LIBS="$($PKG_CONFIG_CMD --libs sdl2 SDL2_ttf SDL2_image)"
else
  SDL_INCLUDE_DIR="${MPL_H700_SDL_INCLUDE_DIR:-$SYSROOT/usr/include/SDL2}"
  SDL_CFLAGS="-I$SDL_INCLUDE_DIR -I$SYSROOT/usr/include -D_REENTRANT"
  SDL_LIBS="-L$SYSROOT/usr/lib/aarch64-linux-gnu -L$SYSROOT/usr/lib -L$SYSROOT/lib/aarch64-linux-gnu -lSDL2_image -lSDL2_ttf -lSDL2 -lasound"
fi

EXTRA_CFLAGS="${MPL_H700_EXTRA_CFLAGS:-}"
EXTRA_LDFLAGS="${MPL_H700_LDFLAGS:-}"
EXTRA_LIBS="${MPL_H700_EXTRA_LIBS:-}"
RPATH_LINK="-Wl,-rpath-link,$SYSROOT/usr/lib/aarch64-linux-gnu -Wl,-rpath-link,$SYSROOT/usr/lib -Wl,-rpath-link,$SYSROOT/lib/aarch64-linux-gnu -Wl,--allow-shlib-undefined"

SOURCES="
$REPO_ROOT/src/app/main.cpp
$REPO_ROOT/src/app/demo_library.cpp
$REPO_ROOT/src/app/desktop_ui_app.cpp
$REPO_ROOT/src/catalog/arcade_name_database.cpp
$REPO_ROOT/src/catalog/launch_hint_resolver.cpp
$REPO_ROOT/src/catalog/library_builder.cpp
$REPO_ROOT/src/catalog/path.cpp
$REPO_ROOT/src/catalog/ports_alias.cpp
$REPO_ROOT/src/catalog/providers/anbernic_provider.cpp
$REPO_ROOT/src/catalog/providers/emulationstation_provider.cpp
$REPO_ROOT/src/catalog/providers/pegasus_provider.cpp
$REPO_ROOT/src/catalog/providers/rom_directory_provider.cpp
$REPO_ROOT/src/devices/h700/capabilities.cpp
$REPO_ROOT/src/devices/h700/launch_request_adapter.cpp
$REPO_ROOT/src/devices/h700/platform_registry.cpp
$REPO_ROOT/src/devices/h700/retroarch_adapter.cpp
$REPO_ROOT/src/devices/h700/system_service.cpp
$REPO_ROOT/src/domain/library.cpp
$REPO_ROOT/src/launch/launcher_adapter.cpp
$REPO_ROOT/src/launch/launch_request.cpp
$REPO_ROOT/src/services/state_store.cpp
$REPO_ROOT/src/ui/layout.cpp
$REPO_ROOT/src/ui/media_playback.cpp
$REPO_ROOT/src/ui/ui_model.cpp
$REPO_ROOT/src/ui/sdl_input.cpp
$REPO_ROOT/src/ui/sdl_renderer.cpp
"

mkdir -p "$BUILD_DIR"

# Intentionally compile the same modular frontend as the desktop demo, but for
# the H700 target ABI. Device-specific paths are supplied by the APPS launcher.
"$CXX" $CXX_TARGET_FLAGS \
  -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -pthread \
  -I"$REPO_ROOT/src" $SDL_CFLAGS $EXTRA_CFLAGS \
  $SOURCES \
  -o "$BUILD_DIR/$BINARY_NAME" \
  $EXTRA_LDFLAGS $SDL_LIBS $EXTRA_LIBS -pthread $RPATH_LINK

rm -rf "$STAGE_ROOT"
mkdir -p "$RUNTIME_DIR"
mkdir -p "$(dirname -- "$ICON_PATH")"
mkdir -p "$(dirname -- "$RUNTIME_ICON_PATH")"
mkdir -p "$LICENSE_DIR"
cp "$BUILD_DIR/$BINARY_NAME" "$RUNTIME_DIR/$BINARY_NAME"
cp "$SCRIPT_DIR/launcher/run_frontend.sh" "$RUNTIME_DIR/run_frontend.sh"
cp "$SCRIPT_DIR/launcher/launch_request.sh" "$RUNTIME_DIR/launch_request.sh"
cp "$SCRIPT_DIR/launcher/prepare_ra_launcher.sh" "$RUNTIME_DIR/prepare_ra_launcher.sh"
cp "$SCRIPT_DIR/launcher/game_volume.sh" "$RUNTIME_DIR/game_volume.sh"
cp "$SCRIPT_DIR/launcher/autostart_ctl.sh" "$RUNTIME_DIR/autostart_ctl.sh"
cp "$SCRIPT_DIR/launcher/autostart_launch.sh" "$RUNTIME_DIR/autostart_launch.sh"
cp "$SCRIPT_DIR/tools/check_system_immutability.sh" "$RUNTIME_DIR/check_system_immutability.sh"
cp "$REPO_ROOT/LICENSE.md" "$LICENSE_DIR/LICENSE.md"
cp "$REPO_ROOT/NOTICE.md" "$LICENSE_DIR/NOTICE.md"
cp "$REPO_ROOT/NOTICE.zh-CN.md" "$LICENSE_DIR/NOTICE.zh-CN.md"
cp "$REPO_ROOT/THIRD_PARTY_NOTICES.md" "$LICENSE_DIR/THIRD_PARTY_NOTICES.md"
cp "$APP_ICON" "$ICON_PATH"
cp "$APP_ICON" "$RUNTIME_ICON_PATH"
if [ -f "$CLASSIC_ICON" ]; then
  cp "$CLASSIC_ICON" "$RUNTIME_CLASSIC_ICON_PATH"
fi
chmod 755 "$RUNTIME_DIR/$BINARY_NAME" \
  "$RUNTIME_DIR/run_frontend.sh" \
  "$RUNTIME_DIR/launch_request.sh" \
  "$RUNTIME_DIR/prepare_ra_launcher.sh" \
  "$RUNTIME_DIR/game_volume.sh" \
  "$RUNTIME_DIR/autostart_ctl.sh" \
  "$RUNTIME_DIR/autostart_launch.sh" \
  "$RUNTIME_DIR/check_system_immutability.sh"

if command -v "$READ_ELF" >/dev/null 2>&1; then
  "$READ_ELF" -h "$RUNTIME_DIR/$BINARY_NAME" >/dev/null
fi

if [ "$FORCE_STOCK_ROUTE" = "1" ]; then
  cat >"$ENTRY_PATH" <<EOF
#!/bin/sh
export MPL_H700_MOD_RA_LAUNCHER=/tmp/rgfrontend-force-stock-no-mod
APP_DIR="\$(CDPATH= cd -- "\$(dirname -- "\$0")/$APP_ID" && pwd)"
exec "\$APP_DIR/run_frontend.sh" "\$@"
EOF
else
  cat >"$ENTRY_PATH" <<EOF
#!/bin/sh
APP_DIR="\$(CDPATH= cd -- "\$(dirname -- "\$0")/$APP_ID" && pwd)"
exec "\$APP_DIR/run_frontend.sh" "\$@"
EOF
fi
chmod 755 "$ENTRY_PATH"

cat >"$RUNTIME_DIR/build-info.txt" <<EOF
app_id=$APP_ID
entry_name=$ENTRY_NAME
version=$VERSION
force_stock_route=$FORCE_STOCK_ROUTE
binary=$BINARY_NAME
sysroot=$SYSROOT
icon=Roms/APPS/Imgs/$ENTRY_NAME.png
runtime_icon=assets/apps/RGFrontend.png
classic_icon=assets/apps/classic_icon.png
game_volume=game_volume.sh
autostart_ctl=autostart_ctl.sh
autostart_launch=autostart_launch.sh
license_dir=licenses
EOF

command -v zip >/dev/null 2>&1 || fail "zip is required to create the release archive"
rm -f "$ARCHIVE_PATH"
(
  cd "$STAGE_ROOT"
  zip -q -r "$ARCHIVE_PATH" Roms
)

printf 'H700 app stage ready: %s\n' "$STAGE_ROOT"
printf 'H700 release archive ready: %s\n' "$ARCHIVE_PATH"

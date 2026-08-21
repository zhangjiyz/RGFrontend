#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
TARGET="${MPL_H700_TARGET:-root@10.1.1.233}"
SYSROOT="${MPL_H700_SYSROOT:-$REPO_ROOT/H700/sysroot}"
SYSROOT_MODE="${MPL_H700_SYSROOT_MODE:-minimal}"
SDL_HEADER_OVERLAY="${MPL_H700_SDL_HEADER_OVERLAY:-}"
REMOTE_ARCHIVE="${MPL_H700_REMOTE_SYSROOT_ARCHIVE:-/tmp/mpl-h700-sysroot.tar}"
TMP_ARCHIVE="$(mktemp "${TMPDIR:-/tmp}/mpl-h700-sysroot.XXXXXX")"

cleanup() {
  rm -f "$TMP_ARCHIVE"
}
trap cleanup EXIT

mkdir -p "$SYSROOT/usr" "$SYSROOT/lib"

printf 'syncing H700 sysroot from %s to %s\n' "$TARGET" "$SYSROOT"

# This only reads target system headers/libs. A temporary archive is written to
# /tmp on the device so password-based SSH sessions do not corrupt tar streams.
ssh "$TARGET" '
set -eu
cd /
archive="'"$REMOTE_ARCHIVE"'"
rm -f "$archive"
{
'"$(
  if [ "$SYSROOT_MODE" = "full" ]; then
    cat <<'REMOTE'
for path in \
  usr/include \
  usr/lib/gcc \
  usr/lib/aarch64-linux-gnu \
  usr/lib/pkgconfig \
  usr/share/pkgconfig \
  lib/aarch64-linux-gnu; do
  [ -e "$path" ] && printf "%s\0" "$path"
done
REMOTE
  else
    cat <<'REMOTE'
for path in \
  usr/include \
  usr/lib/gcc \
  usr/lib/pkgconfig \
  usr/share/pkgconfig; do
  [ -e "$path" ] && printf "%s\0" "$path"
done
for dir in usr/lib/aarch64-linux-gnu lib/aarch64-linux-gnu; do
  [ -d "$dir" ] || continue
  find "$dir" -maxdepth 1 \( -type f -o -type l \) \
    \( -name "libSDL2*" \
       -o -name "libasound*" \
       -o -name "libc.so*" \
       -o -name "libm.so*" \
       -o -name "libpthread.so*" \
       -o -name "libdl.so*" \
       -o -name "librt.so*" \
       -o -name "libresolv.so*" \
       -o -name "libgcc_s.so*" \
       -o -name "ld-linux*.so*" \
       -o -name "crt*.o" \
       -o -name "Scrt1.o" \
       -o -name "crti.o" \
       -o -name "crtn.o" \
       -o -name "lib*_nonshared.a" \) \
    -print0
done
REMOTE
  fi
)"'
} | tar --null -cf "$archive" -T -
ls -lh "$archive"
'

scp "$TARGET:$REMOTE_ARCHIVE" "$TMP_ARCHIVE"
ssh "$TARGET" "rm -f '$REMOTE_ARCHIVE'"

tar -xf "$TMP_ARCHIVE" -C "$SYSROOT"

for libdir in \
  "$SYSROOT/lib/aarch64-linux-gnu" \
  "$SYSROOT/usr/lib/aarch64-linux-gnu"; do
  [ -d "$libdir" ] || continue
  for pair in \
    "c libc.so.6" \
    "m libm.so.6" \
    "pthread libpthread.so.0" \
    "dl libdl.so.2" \
    "rt librt.so.1" \
    "resolv libresolv.so.2"; do
    set -- $pair
    link="$libdir/lib$1.so"
    target="$2"
    if [ -e "$libdir/$target" ] && { [ ! -e "$link" ] || [ -L "$link" ]; }; then
      [ -L "$link" ] && unlink "$link"
      ln -s "$target" "$link"
    fi
  done
done

if [ -e "$SYSROOT/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1" ] &&
   { [ ! -e "$SYSROOT/lib/ld-linux-aarch64.so.1" ] || [ -L "$SYSROOT/lib/ld-linux-aarch64.so.1" ]; }; then
  [ -L "$SYSROOT/lib/ld-linux-aarch64.so.1" ] && unlink "$SYSROOT/lib/ld-linux-aarch64.so.1"
  ln -s "aarch64-linux-gnu/ld-linux-aarch64.so.1" "$SYSROOT/lib/ld-linux-aarch64.so.1"
fi

if [ -n "$SDL_HEADER_OVERLAY" ]; then
  [ -d "$SDL_HEADER_OVERLAY" ] || {
    printf 'error: SDL header overlay not found: %s\n' "$SDL_HEADER_OVERLAY" >&2
    exit 1
  }
  mkdir -p "$SYSROOT/usr/include/SDL2"
  for header in SDL_image.h SDL_ttf.h; do
    if [ -f "$SDL_HEADER_OVERLAY/$header" ]; then
      cp "$SDL_HEADER_OVERLAY/$header" "$SYSROOT/usr/include/SDL2/$header"
    fi
  done
fi

printf 'sysroot sync complete: %s\n' "$SYSROOT"

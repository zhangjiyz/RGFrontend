#!/bin/sh
set -eu

ROOT="${1:-${MPL_H700_ROOT:-/}}"
REPORT="${2:-${MPL_H700_REPORT:-diagnostics/h700-capabilities.txt}}"

case "$ROOT" in
  */) ROOT="${ROOT%/}" ;;
esac
[ -n "$ROOT" ] || ROOT="/"

report_dir="$(dirname "$REPORT")"
mkdir -p "$report_dir"
: >"$REPORT"

write_line() {
  printf '%s\n' "$*" >>"$REPORT"
}

host_path() {
  case "$ROOT" in
    /) printf '%s\n' "$1" ;;
    *) printf '%s%s\n' "$ROOT" "$1" ;;
  esac
}

display_path() {
  path="$1"
  case "$ROOT" in
    /) printf '%s\n' "$path" ;;
    *) printf '%s\n' "${path#"$ROOT"}" ;;
  esac
}

basename_of() {
  path="$1"
  printf '%s\n' "${path##*/}"
}

probe_path() {
  label="$1"
  path="$2"
  full="$(host_path "$path")"
  if [ -d "$full" ]; then
    write_line "$label=dir:$path"
  elif [ -f "$full" ]; then
    if [ -x "$full" ]; then
      write_line "$label=file executable:$path"
    else
      write_line "$label=file:$path"
    fi
  else
    write_line "$label=missing:$path"
  fi
}

count_files() {
  dir="$1"
  find "$dir" -type f 2>/dev/null | wc -l | tr -d ' '
}

report_rom_roots() {
  write_line "[rom_roots]"
  for root in /mnt/mmc/Roms /mnt/sdcard/Roms; do
    full_root="$(host_path "$root")"
    if [ ! -d "$full_root" ]; then
      write_line "rom_root=missing path=$root"
      continue
    fi
    write_line "rom_root=present path=$root"
    find "$full_root" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | sort | while IFS= read -r platform_dir; do
      platform_name="$(basename_of "$platform_dir")"
      files="$(count_files "$platform_dir")"
      write_line "platform_dir=$(display_path "$platform_dir") name=$platform_name file_count=$files"
    done
  done
}

report_core_dir() {
  write_line "[cores]"
  core_dir="$(host_path /mnt/vendor/deep/retro/cores)"
  if [ ! -d "$core_dir" ]; then
    write_line "core_dir=missing path=/mnt/vendor/deep/retro/cores"
    return
  fi
  write_line "core_dir=present path=/mnt/vendor/deep/retro/cores"
  find "$core_dir" -maxdepth 1 -type f -name '*.so' 2>/dev/null | sort | while IFS= read -r core; do
    write_line "core=$(basename_of "$core")"
  done
}

report_core_defaults() {
  write_line "[core_defaults]"
  map_file="$(host_path /mnt/mod/ctrl/configs/CORES.txt)"
  if [ -f "$map_file" ]; then
    write_line "supplemental_core_map=present path=/mnt/mod/ctrl/configs/CORES.txt authoritative=0"
  else
    write_line "supplemental_core_map=missing path=/mnt/mod/ctrl/configs/CORES.txt authoritative=0"
  fi
  write_line "core_defaults=dmenu_builtin owner=RGFrontend priority=1"
}

report_configs() {
  write_line "[retroarch_configs]"
  found=0
  for base in /mnt/vendor /mnt/mod /etc; do
    full_base="$(host_path "$base")"
    [ -d "$full_base" ] || continue
    find "$full_base" -maxdepth 6 -type f \( -name 'retroarch*.cfg' -o -name '*retroarch*.cfg' \) 2>/dev/null | sort | while IFS= read -r cfg; do
      write_line "config=$(display_path "$cfg")"
    done
    if find "$full_base" -maxdepth 6 -type f \( -name 'retroarch*.cfg' -o -name '*retroarch*.cfg' \) 2>/dev/null | grep -q .; then
      found=1
    fi
  done
  [ "$found" -eq 1 ] || write_line "config=none_found"
}

report_app_entries() {
  write_line "[app_entries]"
  for path in /mnt/mmc/APPS /mnt/sdcard/APPS /mnt/mmc/Apps /mnt/sdcard/Apps; do
    probe_path app_entry_root "$path"
  done
}

report_launcher_hints() {
  write_line "[launcher_hints]"
  write_line "ra_launcher_order=mod_then_stock"
  probe_path ra_launcher_mod /mnt/mod/ctrl/RA_launch.sh
  probe_path ra_launcher_stock /mnt/vendor/deep/retro/retroarch
  for base in /mnt/mod /mnt/vendor /etc; do
    full_base="$(host_path "$base")"
    [ -d "$full_base" ] || continue
    grep -R -n -E 'RA_launch|_libretro\.so|retroarch' "$full_base" 2>/dev/null | head -n 80 | while IFS= read -r hit; do
      case "$ROOT" in
        /) write_line "hint=$hit" ;;
        *) write_line "hint=${hit#"$ROOT"}" ;;
      esac
    done
  done
}

report_binary_info() {
  write_line "[binary_info]"
  if command -v file >/dev/null 2>&1; then
    launcher="$(host_path /mnt/vendor/deep/retro/retroarch)"
    [ -f "$launcher" ] && file "$launcher" 2>/dev/null | while IFS= read -r line; do write_line "file=${line#"$ROOT"}"; done
    core_dir="$(host_path /mnt/vendor/deep/retro/cores)"
    [ -d "$core_dir" ] && find "$core_dir" -maxdepth 1 -type f -name '*.so' 2>/dev/null | sort | head -n 20 | while IFS= read -r core; do
      file "$core" 2>/dev/null | while IFS= read -r line; do write_line "file=${line#"$ROOT"}"; done
    done
  else
    write_line "file_tool=unavailable"
  fi
}

write_line "# H700 capability collection"
write_line "root=$ROOT"
write_line "report=$REPORT"
write_line "[known_paths]"
probe_path mmc_root /mnt/mmc
probe_path sdcard_root /mnt/sdcard
probe_path vendor_root /mnt/vendor
probe_path mod_root /mnt/mod
report_rom_roots
report_core_dir
report_core_defaults
report_configs
report_app_entries
report_launcher_hints
report_binary_info
write_line "[done]"
write_line "status=ok"

printf '%s\n' "$REPORT"

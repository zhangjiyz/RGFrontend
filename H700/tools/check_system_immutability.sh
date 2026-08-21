#!/bin/sh
set -eu

usage() {
  cat <<'EOF'
Usage:
  check_system_immutability.sh snapshot OUT_DIR [ROOT]
  check_system_immutability.sh compare BEFORE_DIR AFTER_DIR

snapshot writes read-only hashes and file lists into OUT_DIR.
ROOT defaults to MPL_H700_ROOT or /, so the script can run on-device or against
an extracted/mounted root filesystem.
EOF
}

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 2
}

mode="${1:-}"
[ -n "$mode" ] || {
  usage >&2
  exit 2
}

normalize_root() {
  root="$1"
  case "$root" in
    */) root="${root%/}" ;;
  esac
  [ -n "$root" ] || root="/"
  printf '%s\n' "$root"
}

host_path() {
  root="$1"
  path="$2"
  case "$root" in
    /) printf '%s\n' "$path" ;;
    *) printf '%s%s\n' "$root" "$path" ;;
  esac
}

display_path() {
  root="$1"
  full="$2"
  case "$root" in
    /) printf '%s\n' "$full" ;;
    *) printf '%s\n' "${full#"$root"}" ;;
  esac
}

hash_file() {
  file="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$file" | awk '{print $1}'
  else
    cksum "$file" | awk '{print "cksum:" $1 ":" $2}'
  fi
}

record_file() {
  root="$1"
  file="$2"
  list="$3"
  hashes="$4"
  [ -f "$file" ] || return 0
  shown="$(display_path "$root" "$file")"
  hash="$(hash_file "$file" 2>/dev/null || printf unreadable)"
  size="$(wc -c <"$file" 2>/dev/null | tr -d ' ' || printf unknown)"
  printf '%s\t%s\n' "$shown" "$size" >>"$list"
  printf '%s\t%s\n' "$hash" "$shown" >>"$hashes"
}

record_tree() {
  root="$1"
  relative="$2"
  list="$3"
  hashes="$4"
  shift 4
  dir="$(host_path "$root" "$relative")"
  [ -d "$dir" ] || return 0
  find "$dir" "$@" -type f 2>/dev/null | sort | while IFS= read -r file; do
    record_file "$root" "$file" "$list" "$hashes"
  done
}

write_snapshot() {
  out_dir="$1"
  root="$(normalize_root "$2")"
  mkdir -p "$out_dir"
  protected_list="$out_dir/protected-files.tsv"
  protected_hashes="$out_dir/protected-hashes.tsv"
  runtime_list="$out_dir/runtime-files.tsv"
  runtime_hashes="$out_dir/runtime-hashes.tsv"
  meta="$out_dir/metadata.txt"
  : >"$protected_list"
  : >"$protected_hashes"
  : >"$runtime_list"
  : >"$runtime_hashes"

  {
    printf 'root=%s\n' "$root"
    printf 'created_at=%s\n' "$(date '+%F %T %Z' 2>/dev/null || date)"
    printf 'host=%s\n' "$(hostname 2>/dev/null || printf unknown)"
  } >"$meta"

  record_file "$root" "$(host_path "$root" /mnt/vendor/deep/retro/retroarch)" "$protected_list" "$protected_hashes"
  record_file "$root" "$(host_path "$root" /mnt/mod/ctrl/RA_launch.sh)" "$protected_list" "$protected_hashes"
  record_file "$root" "$(host_path "$root" /mnt/mod/ctrl/configs/CORES.txt)" "$protected_list" "$protected_hashes"
  record_file "$root" "$(host_path "$root" /mnt/mod/ctrl/configs/system.cfg)" "$protected_list" "$protected_hashes"

  record_tree "$root" /mnt/vendor/deep/retro/cores "$protected_list" "$protected_hashes" -maxdepth 1
  record_tree "$root" /mnt/vendor/ctrl "$protected_list" "$protected_hashes" -maxdepth 2 \( -name '*.sh' -o -name '*.py' -o -name '*.conf' -o -name '*.cfg' \)
  record_tree "$root" /mnt/mod/ctrl "$protected_list" "$protected_hashes" -maxdepth 2 \( -name '*.sh' -o -name '*.txt' -o -name '*.cfg' \) ! -name '*.log'
  record_tree "$root" /etc/init.d "$protected_list" "$protected_hashes" -maxdepth 1
  record_tree "$root" /etc/systemd/system "$protected_list" "$protected_hashes" -maxdepth 2 \( -name '*.service' -o -name '*.target' -o -name '*.timer' \)
  record_file "$root" "$(host_path "$root" /etc/rc.local)" "$protected_list" "$protected_hashes"

  record_tree "$root" /.config/retroarch "$runtime_list" "$runtime_hashes" -maxdepth 5 \( -name 'retroarch*.cfg' -o -path '*/config/*' -o -path '*/remaps/*' -o -path '*/shaders/*' -o -path '*/cheats/*' \)
  record_tree "$root" /mnt/vendor/deep/retro/config "$runtime_list" "$runtime_hashes" -maxdepth 4
  record_tree "$root" /mnt/vendor/deep/retro/remaps "$runtime_list" "$runtime_hashes" -maxdepth 4
  record_tree "$root" /mnt/vendor/deep/retro/shaders "$runtime_list" "$runtime_hashes" -maxdepth 4
  record_tree "$root" /mnt/vendor/deep/retro/cheats "$runtime_list" "$runtime_hashes" -maxdepth 4
  record_tree "$root" /mnt/mod/ctrl/configs "$runtime_list" "$runtime_hashes" -maxdepth 1 \( -name '*.log' -o -name '*history*' \)

  sort -u "$protected_list" >"$protected_list.tmp" && mv "$protected_list.tmp" "$protected_list"
  sort -u "$protected_hashes" >"$protected_hashes.tmp" && mv "$protected_hashes.tmp" "$protected_hashes"
  sort -u "$runtime_list" >"$runtime_list.tmp" && mv "$runtime_list.tmp" "$runtime_list"
  sort -u "$runtime_hashes" >"$runtime_hashes.tmp" && mv "$runtime_hashes.tmp" "$runtime_hashes"

  printf 'snapshot=%s\n' "$out_dir"
}

diff_file() {
  diff_before="$1"
  diff_after="$2"
  diff_label="$3"
  diff_report="$4"
  if diff -u "$diff_before" "$diff_after" >"$diff_report/$diff_label.diff" 2>/dev/null; then
    rm -f "$diff_report/$diff_label.diff"
    return 0
  fi
  return 1
}

compare_snapshots() {
  before="$1"
  after="$2"
  [ -d "$before" ] || fail "before snapshot not found: $before"
  [ -d "$after" ] || fail "after snapshot not found: $after"
  report="$after/compare-report"
  mkdir -p "$report"
  status=0

  {
    printf '# H700 system immutability compare\n'
    printf 'before=%s\n' "$before"
    printf 'after=%s\n' "$after"
  } >"$report/summary.txt"

  if diff_file "$before/protected-hashes.tsv" "$after/protected-hashes.tsv" protected-hashes "$report"; then
    printf 'protected_hashes=unchanged\n' >>"$report/summary.txt"
  else
    printf 'protected_hashes=changed\n' >>"$report/summary.txt"
    status=20
  fi

  if diff_file "$before/protected-files.tsv" "$after/protected-files.tsv" protected-files "$report"; then
    printf 'protected_file_list=unchanged\n' >>"$report/summary.txt"
  else
    printf 'protected_file_list=changed\n' >>"$report/summary.txt"
    status=20
  fi

  if diff_file "$before/runtime-hashes.tsv" "$after/runtime-hashes.tsv" runtime-hashes "$report"; then
    printf 'runtime_hashes=unchanged\n' >>"$report/summary.txt"
  else
    printf 'runtime_hashes=changed_observed\n' >>"$report/summary.txt"
  fi

  if diff_file "$before/runtime-files.tsv" "$after/runtime-files.tsv" runtime-files "$report"; then
    printf 'runtime_file_list=unchanged\n' >>"$report/summary.txt"
  else
    printf 'runtime_file_list=changed_observed\n' >>"$report/summary.txt"
  fi

  printf 'report=%s\n' "$report"
  return "$status"
}

case "$mode" in
  snapshot)
    out_dir="${2:-}"
    [ -n "$out_dir" ] || fail "snapshot requires OUT_DIR"
    root="${3:-${MPL_H700_ROOT:-/}}"
    write_snapshot "$out_dir" "$root"
    ;;
  compare)
    before="${2:-}"
    after="${3:-}"
    [ -n "$before" ] || fail "compare requires BEFORE_DIR"
    [ -n "$after" ] || fail "compare requires AFTER_DIR"
    compare_snapshots "$before" "$after"
    ;;
  --help|-h|help)
    usage
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac

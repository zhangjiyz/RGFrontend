#include "catalog/launch_hint_resolver.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace mpl {

namespace {

std::string Trim(std::string value) {
  const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                          [&](char ch) { return !is_space(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(),
                           [&](char ch) { return !is_space(ch); }).base(), value.end());
  return value;
}

std::string LowerAscii(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

std::string Basename(std::string path) {
  const size_t slash = path.find_last_of("/\\");
  if (slash != std::string::npos) path = path.substr(slash + 1);
  return path;
}

std::vector<std::string> TokenizeCommand(const std::string &text) {
  std::vector<std::string> tokens;
  std::string current;
  char quote = 0;
  bool escaped = false;
  for (char ch : text) {
    if (escaped) {
      current.push_back(ch);
      escaped = false;
      continue;
    }
    if (ch == '\\' && quote != '\'') {
      escaped = true;
      continue;
    }
    if (quote != 0) {
      if (ch == quote) quote = 0;
      else current.push_back(ch);
      continue;
    }
    if (ch == '\'' || ch == '"') {
      quote = ch;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (!current.empty()) {
        tokens.push_back(current);
        current.clear();
      }
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty()) tokens.push_back(current);
  return tokens;
}

bool Contains(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

bool HasTokenBasename(const std::vector<std::string> &tokens, const std::string &name) {
  return std::any_of(tokens.begin(), tokens.end(), [&](const std::string &token) {
    return LowerAscii(Basename(token)) == name;
  });
}

void ApplyCoreHint(const std::string &lower, LaunchHint *hint) {
  struct CoreMap {
    const char *needle;
    const char *core;
    const char *platform;
    const char *launcher;
  };
  static const CoreMap kCores[] = {
      {"mgba", "mgba_libretro.so", "gba", "retroarch"},
      {"gpsp", "gpsp_libretro.so", "gba", "retroarch"},
      {"vba_next", "vba_next_libretro.so", "gba", "retroarch"},
      {"vba-next", "vba_next_libretro.so", "gba", "retroarch"},
      {"vbam", "vbam_libretro.so", "gba", "retroarch"},
      {"vba-m", "vbam_libretro.so", "gba", "retroarch"},
      {"gambatte", "gambatte_libretro.so", "gb", "retroarch"},
      {"mesen", "mesen_libretro.so", "fc_hd", "retroarch"},
      {"fceumm", "fceumm_libretro.so", "fc", "retroarch"},
      {"snes9x", "snes9x_libretro.so", "sfc", "retroarch"},
      {"picodrive", "picodrive_libretro.so", "md", "retroarch"},
      {"genesis_plus_gx", "genesis_plus_gx_libretro.so", "md", "retroarch"},
      {"pcsx_rearmed", "pcsx_rearmed_libretro.so", "ps", "retroarch"},
      {"mupen64plus", "mupen64plus_next_libretro.so", "n64", "retroarch"},
      {"mednafen_saturn", "mednafen_saturn_libretro.so", "saturn", "retroarch"},
      {"yabasanshiro", "yabasanshiro_libretro.so", "saturn", "retroarch"},
      {"yabause", "yabause_libretro.so", "saturn", "retroarch"},
      {"mamearcade", "mame2022xtreme_libretro.so", "mame", "retroarch"},
      {"mame2022xtreme", "mame2022xtreme_libretro.so", "mame", "retroarch"},
      {"fbneo_plus", "fbneo_plus_libretro.so", "fbneo", "retroarch"},
      {"fbneo", "fbneo_libretro.so", "fbneo", "retroarch"},
  };
  for (const CoreMap &entry : kCores) {
    if (!Contains(lower, entry.needle)) continue;
    hint->core_hint = entry.core;
    if (hint->platform_hint.empty()) hint->platform_hint = entry.platform;
    if (hint->launcher_alias.empty()) hint->launcher_alias = entry.launcher;
    return;
  }
}

void ApplyAndroidPackageHint(const std::string &package_name, LaunchHint *hint) {
  const std::string lower = LowerAscii(package_name);
  if (Contains(lower, "ppsspp")) {
    hint->platform_hint = "psp";
    hint->launcher_alias = "ppsspp";
  } else if (Contains(lower, "retroarch")) {
    hint->launcher_alias = "retroarch";
  } else if (Contains(lower, "dolphin")) {
    hint->platform_hint = "gc-wii";
    hint->launcher_alias = "dolphin";
  } else if (Contains(lower, "citra") || Contains(lower, "lime3ds") ||
             Contains(lower, "azahar")) {
    hint->platform_hint = "3ds";
    hint->launcher_alias = "citra";
  } else if (Contains(lower, "melonds") || Contains(lower, "drastic")) {
    hint->platform_hint = "nds";
    hint->launcher_alias = Contains(lower, "drastic") ? "drastic" : "melonds";
  } else if (Contains(lower, "yaba") || Contains(lower, "yabasanshiro")) {
    hint->platform_hint = "saturn";
    hint->launcher_alias = "yabasanshiro";
  } else if (Contains(lower, "flycast") || Contains(lower, "reicast")) {
    hint->platform_hint = "dreamcast";
    hint->launcher_alias = Contains(lower, "flycast") ? "flycast" : "reicast";
  }
}

void ApplyLinuxCommandHint(const std::string &command, LaunchHint *hint) {
  const std::string command_lower = LowerAscii(command);
  const std::string lower = LowerAscii(Basename(command));
  if (Contains(command_lower, "emujava")) {
    hint->platform_hint = "java";
    hint->launcher_alias = "freej2me";
  } else if (Contains(lower, "retroarch")) {
    hint->launcher_alias = "retroarch";
  } else if (Contains(lower, "ppsspp")) {
    hint->platform_hint = "psp";
    hint->launcher_alias = "ppsspp";
  } else if (Contains(lower, "drastic")) {
    hint->platform_hint = "nds";
    hint->launcher_alias = "drastic";
  } else if (Contains(lower, "openbor")) {
    hint->platform_hint = "openbor";
    hint->launcher_alias = "openbor";
  } else if (Contains(lower, "flycast")) {
    hint->platform_hint = "dreamcast";
    hint->launcher_alias = "flycast";
  } else if (Contains(lower, "yabasanshiro") || Contains(lower, "saturn")) {
    hint->platform_hint = "saturn";
    hint->launcher_alias = "saturn";
  } else if (Contains(lower, "dosbox")) {
    hint->platform_hint = "dos";
    hint->launcher_alias = "dosbox";
  } else if (Contains(lower, "scummvm")) {
    hint->platform_hint = "scummvm";
    hint->launcher_alias = "scummvm";
  }
}

}  // namespace

const char *LaunchHintKindName(LaunchHintKind kind) {
  switch (kind) {
    case LaunchHintKind::Empty:
      return "empty";
    case LaunchHintKind::AndroidActivity:
      return "android-activity";
    case LaunchHintKind::LinuxCommand:
      return "linux-command";
    case LaunchHintKind::Unknown:
      return "unknown";
  }
  return "unknown";
}

LaunchHint ResolveLaunchHint(const std::string &raw_hint) {
  LaunchHint hint;
  hint.raw = Trim(raw_hint);
  if (hint.raw.empty()) return hint;

  const std::vector<std::string> tokens = TokenizeCommand(hint.raw);
  const std::string lower = LowerAscii(hint.raw);
  ApplyCoreHint(lower, &hint);

  const bool looks_android = HasTokenBasename(tokens, "am") &&
                             HasTokenBasename(tokens, "start");
  if (looks_android) {
    hint.kind = LaunchHintKind::AndroidActivity;
    for (size_t index = 0; index + 1 < tokens.size(); ++index) {
      if (tokens[index] != "-n") continue;
      const std::string component = tokens[index + 1];
      const size_t slash = component.find('/');
      hint.android_package = slash == std::string::npos ? component : component.substr(0, slash);
      hint.android_activity = slash == std::string::npos ? std::string{} : component.substr(slash + 1);
      ApplyAndroidPackageHint(hint.android_package, &hint);
      break;
    }
    if (hint.android_package.empty()) {
      hint.diagnostic = "android am start command without -n package/activity component";
    }
    return hint;
  }

  if (!tokens.empty()) {
    hint.kind = LaunchHintKind::LinuxCommand;
    hint.command = tokens.front();
    ApplyLinuxCommandHint(hint.command, &hint);
    if (hint.launcher_alias.empty() && hint.core_hint.empty()) {
      hint.kind = LaunchHintKind::Unknown;
      hint.diagnostic = "launch command is preserved as metadata only";
    }
    return hint;
  }

  hint.kind = LaunchHintKind::Unknown;
  hint.diagnostic = "launch hint could not be tokenized";
  return hint;
}

}  // namespace mpl

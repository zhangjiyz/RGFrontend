#include "devices/h700/platform_registry.h"

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace mpl {

namespace {

struct PlatformSpec {
  const char *id;
  const char *display_name;
  std::vector<std::string> directory_names;
  std::vector<std::string> extensions;
  std::vector<std::string> aliases;
};

std::vector<std::string> RomDirectories(const H700RegistryOptions &options,
                                        const std::vector<std::string> &names) {
  std::vector<std::string> result;
  for (const std::string &card : options.card_roots) {
    for (const std::string &name : names) {
      const fs::path candidate = fs::u8path(card) / "Roms" / fs::u8path(name);
      bool duplicate = false;
      for (const std::string &existing : result) {
        std::error_code error;
        if (fs::equivalent(candidate, fs::u8path(existing), error) && !error) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate) result.push_back(candidate.u8string());
    }
  }
  return result;
}

bool FileExists(const std::string &path) {
  std::error_code error;
  return fs::is_regular_file(fs::u8path(path), error);
}

bool UsesArcadeNameDatabase(const std::string &platform_id) {
  for (const char *id : {"atomiswave", "cps1", "cps2", "cps3", "fbneo", "hbmame",
                         "mame", "naomi", "neogeo", "pgm2", "varcade"}) {
    if (platform_id == id) return true;
  }
  return false;
}

Platform RetroArchPlatform(const H700RegistryOptions &options, std::string id,
                           std::string display_name, std::vector<std::string> directory_names,
                           std::vector<std::string> extensions, int sort_order) {
  Platform platform;
  platform.id = std::move(id);
  platform.display_name = std::move(display_name);
  platform.rom_directories = RomDirectories(options, directory_names);
  platform.directory_aliases = std::move(directory_names);
  platform.extensions = std::move(extensions);
  platform.media_rule.cover_names = {"boxfront.png", "boxFront.png", "cover.png", "cover.jpg",
                                     "boxfront.jpg", "boxFront.jpg", "cover.png", "cover.jpg",
                                     "image.png", "image.jpg"};
  platform.media_rule.logo_names = {"logo.png", "logo.jpg", "marquee.png", "marquee.jpg"};
  platform.media_rule.video_names = {"video.mp4", "video.mkv"};
  platform.launcher_id = "h700-retroarch-" + platform.id;
  platform.launcher_kind = LauncherKind::RetroArch;
  platform.sort_order = sort_order;
  platform.launchable = FileExists(options.retroarch_launcher);
  if (!platform.launchable) {
    platform.diagnostics.push_back("missing launcher: " + options.retroarch_launcher);
  }
  return platform;
}

Platform StandalonePlatform(const H700RegistryOptions &options, std::string id,
                            std::string display_name, std::vector<std::string> directory_names,
                            std::vector<std::string> extensions, std::string launcher_id,
                            std::vector<std::string> required_paths, int sort_order) {
  Platform platform;
  platform.id = std::move(id);
  platform.display_name = std::move(display_name);
  platform.rom_directories = RomDirectories(options, directory_names);
  platform.directory_aliases = std::move(directory_names);
  platform.extensions = std::move(extensions);
  platform.media_rule.cover_names = {"boxfront.png", "boxFront.png", "cover.png", "cover.jpg",
                                     "boxfront.jpg", "boxFront.jpg", "image.png",
                                     "image.jpg"};
  platform.media_rule.logo_names = {"logo.png", "logo.jpg", "marquee.png", "marquee.jpg"};
  platform.media_rule.video_names = {"video.mp4", "video.mkv"};
  platform.launcher_id = std::move(launcher_id);
  platform.launcher_kind = LauncherKind::Standalone;
  platform.sort_order = sort_order;
  platform.launchable = true;
  for (const std::string &path : required_paths) {
    if (FileExists(path)) continue;
    platform.launchable = false;
    platform.diagnostics.push_back("missing standalone launcher: " + path);
  }
  return platform;
}

}  // namespace

std::vector<Platform> LoadH700Platforms(const H700RegistryOptions &options) {
  const std::vector<PlatformSpec> specs = {
      {"a2600", "A2600", {"A2600"}, {".a26", ".bin", ".zip", ".7z"}, {"atari2600"}},
      {"a5200", "A5200", {"A5200"}, {".a52", ".zip", ".7z"}, {"atari5200"}},
      {"a7800", "A7800", {"A7800"}, {".a78", ".bin", ".zip", ".7z"}, {"atari7800"}},
      {"a800", "A800", {"A800"}, {".atr", ".rom", ".zip", ".7z"}, {"atari800"}},
      {"amiga", "AMIGA", {"AMIGA"}, {".adf", ".uae", ".ipf", ".dms", ".adz",
                                        ".lha", ".m3u", ".hdf", ".hdz", ".iso",
                                        ".cue", ".chd", ".zip", ".7z"}, {}},
      {"atarist", "ATARIST", {"ATARIST"}, {".st", ".msa", ".stx", ".dim",
                                            ".ipf", ".m3u", ".zip", ".7z"}, {}},
      {"atomiswave", "ATOMISWAVE", {"ATOMISWAVE"}, {".zip", ".7z", ".chd"}, {}},
      {"c64", "C64", {"C64"}, {".d64", ".d71", ".d80", ".d81", ".d82", ".g64",
                                  ".g41", ".x64", ".t64", ".tap", ".prg", ".p00",
                                  ".crt", ".bin", ".cmd", ".m3u", ".vsf", ".nib",
                                  ".nbz", ".zip", ".7z"}, {"commodore64"}},
      {"cps1", "CPS1", {"CPS1"}, {".zip", ".7z"}, {}},
      {"cps2", "CPS2", {"CPS2"}, {".zip", ".7z"}, {}},
      {"cps3", "CPS3", {"CPS3"}, {".zip", ".7z"}, {}},
      {"dos", "DOS", {"DOS"}, {".dosz", ".com", ".bat", ".exe", ".zip", ".7z"}, {}},
      {"dreamcast", "DREAMCAST", {"DREAMCAST", "DC", "DC hack"},
       {".chd", ".cdi", ".gdi", ".cue", ".iso", ".bin", ".zip", ".m3u", ".7z"},
       {"dc"}},
      {"easyrpg", "EASYRPG", {"EASYRPG"}, {".ldb", ".sh", ".zip"}, {}},
      {"fbneo", "FBNEO",
       {"FBNEO", "FBNEO ACT", "FBNEO ACT hack", "FBNEO ACT V",
        "FBNEO ETC", "FBNEO ETC V", "FBNEO FLY", "FBNEO FLY V",
        "FBNEO FTG", "FBNEO FTG hack", "FBNEO RAC", "FBNEO SPO",
        "FBNEO STG", "FBNEO STG hack", "FBNEO STG V"},
       {".zip", ".7z"}, {"fbn"}},
      {"fc", "FC", {"FC", "NES", "FC hack"}, {".nes", ".zip", ".7z"},
       {"nes", "famicom"}},
      {"fds", "FDS", {"FDS"}, {".fds", ".zip", ".7z"}, {}},
      {"gb", "GB", {"GB", "Game Boy"}, {".gb", ".zip", ".7z"}, {"gameboy", "dmg"}},
      {"gbc", "GBC", {"GBC", "gbc", "Game Boy Color"}, {".gbc", ".zip", ".7z"},
       {"gbcolor", "gameboycolor"}},
      {"gba", "GBA", {"GBA", "Game Boy Advance"}, {".gba", ".zip", ".7z"},
       {"gameboyadvance"}},
      {"gg", "GG", {"GG"}, {".gg", ".zip", ".7z"}, {"gamegear"}},
      {"gw", "GW", {"GW"}, {".mgw", ".zip", ".7z"}, {"gameandwatch"}},
      {"hbmame", "H.Brew", {"HBMAME"}, {".zip", ".7z"}, {}},
      {"lynx", "LYNX", {"LYNX"}, {".lnx", ".zip", ".7z"}, {"atarilynx"}},
      {"mame", "MAME",
       {"MAME", "MAME ACT", "MAME ETC", "MAME FLY", "MAME FLY V",
        "MAME FTG", "MAME FTG hack", "MAME RAC", "MAME SPO",
        "MAME STG", "MAME STG V"},
       {".zip", ".7z"}, {}},
      {"md", "MD", {"MD", "Genesis", "MD hack", "MD hack(picodrive)"},
       {".md", ".smd", ".gen", ".bin", ".zip", ".7z"},
       {"megadrive", "genesis"}},
      {"mdcd", "MDCD", {"MDCD", "Sega CD", "MD-CD"}, {".cue", ".iso", ".chd", ".m3u",
                                                        ".sg", ".zip", ".7z"},
       {"segacd", "megacd"}},
      {"msx", "MSX", {"MSX"}, {".rom", ".ri", ".mx1", ".mx2", ".col", ".dsk",
                                  ".cas", ".sg", ".sc", ".m3u", ".zip", ".7z"}, {}},
      {"n64", "N64", {"N64"}, {".n64", ".v64", ".z64", ".bin", ".rom", ".zip", ".7z"},
       {}},
      {"naomi", "NAOMI", {"NAOMI"}, {".zip", ".7z", ".chd"}, {}},
      {"neocd", "NEOCD", {"NEOCD", "NEOGEO-CD"},
       {".cue", ".chd", ".iso", ".zip", ".7z"}, {}},
      {"neogeo", "NEOGEO", {"NEOGEO"}, {".zip", ".7z"}, {}},
      {"ngp", "NGP", {"NGP", "NGPC"}, {".ngp", ".ngc", ".zip", ".7z"}, {"ngpc"}},
      {"ons", "ONS", {"ONS"}, {".dat", ".txt", ".nt", ".nt2", ".nt3", ".ons",
                                 ".zip", ".7z"}, {"onscripter"}},
      {"pce", "PCE", {"PCE"}, {".pce", ".chd", ".img", ".cue", ".ccd", ".iso",
                                  ".bin", ".zip", ".7z"}, {"pcengine"}},
      {"pcecd", "PCECD", {"PCECD", "PCE-CD"}, {".cue", ".ccd", ".chd", ".toc", ".m3u"},
       {"pcenginecd"}},
      {"pgm2", "PGM2", {"PGM2"}, {".zip", ".7z"}, {}},
      {"pico", "PICO", {"PICO"}, {".p8", ".png"}, {"pico8"}},
      {"poke", "POKE", {"POKE"}, {".min", ".zip", ".7z"}, {"pokemini"}},
      {"ps", "PS", {"PS", "PlayStation", "PS1", "PS1 hack"},
       {".cue", ".chd", ".pbp", ".m3u", ".ccd", ".iso", ".bin", ".img", ".mdf", ".toc"},
       {"psx", "playstation"}},
      {"saturn", "SATURN", {"SATURN", "SS"}, {".bin", ".cue", ".iso", ".mds", ".ccd",
                                          ".chd", ".rar", ".m3u"}, {"ss"}},
      {"scummvm", "SCUMMVM", {"SCUMMVM"}, {".scummvm", ".zip"}, {}},
      {"sega32x", "SEGA32X", {"SEGA32X", "MD-32X"}, {".32x", ".md", ".smd", ".bin",
                                                       ".zip", ".7z"}, {"32x"}},
      {"sfc", "SFC", {"SFC", "SNES", "SFC hack", "SFC-MSU1"},
       {".sfc", ".smc", ".zip", ".7z"}, {"snes"}},
      {"sms", "SMS", {"SMS"}, {".sms", ".zip", ".7z"}, {"mastersystem"}},
      {"varcade", "VARCADE", {"VARCADE"}, {".zip", ".7z"}, {}},
      {"vb", "VB", {"VB"}, {".vb", ".zip", ".7z"}, {"virtualboy"}},
      {"vic20", "VIC20", {"VIC20"}, {".a0", ".20", ".b0", ".d6", ".d7", ".d8",
                                       ".g4", ".g6", ".gz", ".x6", ".t64", ".tap",
                                       ".prg", ".p00", ".crt", ".bin", ".cmd",
                                       ".m3u", ".vsf", ".nib", ".nbz", ".zip", ".7z"},
       {}},
      {"ws", "WS", {"WS", "WSWAN", "WSC"}, {".ws", ".wsc", ".zip", ".7z"},
       {"wonderswan", "wswan"}},
  };

  std::vector<Platform> platforms;
  platforms.reserve(specs.size() + 5);
  int sort_order = 10;
  for (const PlatformSpec &spec : specs) {
    Platform platform = spec.id == std::string("saturn")
                            ? StandalonePlatform(options, spec.id, spec.display_name,
                                                 spec.directory_names, spec.extensions,
                                                 "h700-standalone-saturn",
                                                 {options.saturn_launcher},
                                                 sort_order)
                            : RetroArchPlatform(options, spec.id, spec.display_name,
                                                spec.directory_names, spec.extensions,
                                                sort_order);
    if (spec.id == std::string("saturn") && !options.enable_saturn) {
      platform.launchable = false;
      platform.rom_directories.clear();
      platform.diagnostics.push_back("disabled: Saturn/SS launch is pending H700 validation");
    }
    platform.platform_aliases = spec.aliases;
    if (UsesArcadeNameDatabase(platform.id)) {
      platform.arcade_name_database_path = options.arcade_name_database;
    }
    platforms.push_back(std::move(platform));
    sort_order += 10;
    if (spec.id == std::string("hbmame")) {
      platforms.push_back(StandalonePlatform(options, "java", "JAVA", {"JAVA"},
                                             {".jar"}, "h700-standalone-java",
                                             {options.java_launcher}, sort_order));
      sort_order += 10;
    }
    if (spec.id == std::string("n64")) {
      platforms.push_back(StandalonePlatform(options, "nds", "NDS", {"NDS"},
                                             {".nds", ".zip", ".7z"},
                                             "h700-standalone-nds",
                                             {options.nds_launcher}, sort_order));
      sort_order += 10;
    }
    if (spec.id == std::string("ps")) {
      platforms.push_back(StandalonePlatform(options, "psp", "PSP", {"PSP"},
                                             {".iso", ".cso", ".pbp"},
                                             "h700-standalone-psp",
                                             {options.psp_launcher}, sort_order));
      sort_order += 10;
      platforms.push_back(StandalonePlatform(options, "openbor", "OPENBOR", {"OPENBOR"},
                                             {".pak"}, "h700-standalone-openbor",
                                             {options.openbor_setup_script,
                                              options.openbor_launcher},
                                             sort_order));
      sort_order += 10;
      platforms.push_back(StandalonePlatform(options, "ports", "PORTS", {"PORTS"},
                                             {".sh"}, "h700-standalone-ports",
                                             {options.ports_shell}, sort_order));
      sort_order += 10;
    }
  }
  return platforms;
}

}  // namespace mpl

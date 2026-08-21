#include "catalog/library_builder.h"
#include "devices/h700/platform_registry.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using namespace mpl;

namespace {

const Game *FindByTitle(const std::vector<Game> &games, const std::string &title) {
  const auto found = std::find_if(games.begin(), games.end(), [&](const Game &game) {
    return game.title == title;
  });
  return found == games.end() ? nullptr : &*found;
}

int CountByTitle(const std::vector<Game> &games, const std::string &title) {
  return static_cast<int>(std::count_if(games.begin(), games.end(), [&](const Game &game) {
    return game.title == title;
  }));
}

const Platform *FindPlatformById(const std::vector<Platform> &platforms, const std::string &id) {
  const auto found = std::find_if(platforms.begin(), platforms.end(),
                                  [&](const Platform &platform) { return platform.id == id; });
  return found == platforms.end() ? nullptr : &*found;
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "multiplatform_launcher_p1_test";
  fs::remove_all(root);

  const fs::path card = root / "mnt" / "sdcard";
  const fs::path gb = card / "Roms" / "GB";
  const fs::path gbc_alias = card / "Roms" / "Game Boy Color";
  const fs::path fc = card / "Roms" / "FC";
  const fs::path fc_hack = card / "Roms" / "FC hack";
  const fs::path fc_hd = card / "Roms" / "FC-HD";
  const fs::path md_picodrive = card / "Roms" / "MD hack(picodrive)";
  const fs::path nds = card / "Roms" / "NDS";
  const fs::path psp = card / "Roms" / "PSP";
  const fs::path openbor = card / "Roms" / "OPENBOR";
  const fs::path ports = card / "Roms" / "PORTS";
  const fs::path java = card / "Roms" / "JAVA" / "240x320";
  const fs::path saturn_ss = card / "Roms" / "SS";
  const fs::path pico = card / "Roms" / "PICO";
  const fs::path fbneo_act = card / "Roms" / "FBNEO ACT";
  const fs::path fbneo_fly = card / "Roms" / "FBNEO FLY";
  const fs::path mame_fly = card / "Roms" / "MAME FLY";
  const fs::path naomi = card / "Roms" / "NAOMI";
  const fs::path system = root / "system";
  fs::create_directories(gb / "media");
  fs::create_directories(gb / "videos");
  fs::create_directories(gbc_alias);
  fs::create_directories(fc);
  fs::create_directories(fc_hack);
  fs::create_directories(fc_hd);
  fs::create_directories(md_picodrive);
  fs::create_directories(nds);
  fs::create_directories(psp);
  fs::create_directories(openbor);
  fs::create_directories(ports / "Balatro");
  fs::create_directories(ports / "Imgs");
  fs::create_directories(java / "Imgs");
  fs::create_directories(saturn_ss);
  fs::create_directories(pico / "Imgs");
  fs::create_directories(fbneo_act);
  fs::create_directories(fbneo_fly / "media" / "1944");
  fs::create_directories(mame_fly);
  fs::create_directories(naomi / "ikaruga");
  fs::create_directories(system);
  std::ofstream(system / "RA_launch.sh") << "#!/bin/sh\n";
  std::ofstream(system / "setNDS.sh") << "#!/bin/sh\n";
  std::ofstream(system / "PPSSPPSDL") << "#!/bin/sh\n";
  std::ofstream(system / "openbor.sh") << "#!/bin/sh\n";
  std::ofstream(system / "OpenBOR.dge") << "#!/bin/sh\n";
  std::ofstream(system / "bash") << "#!/bin/sh\n";
  std::ofstream(system / "launch_java.sh") << "#!/bin/sh\n";
  std::ofstream(system / "setSaturn.sh") << "#!/bin/sh\n";
  std::ofstream(system / "yabasanshiro") << "#!/bin/sh\n";
  std::ofstream(system / "saturn_bios.bin") << "bios";
  std::ofstream(gb / "Tetris.gb") << "gb-rom";
  std::ofstream(gb / "Kirby.gb") << "gb-rom";
  std::ofstream(gb / "Metroid.gb") << "gb-rom-not-in-es";
  std::ofstream(gb / "media" / "tetris.png") << "png";
  std::ofstream(gb / "media" / "tetris-logo.png") << "logo";
  std::ofstream(gb / "media" / "tetris-video.mp4") << "video";
  std::ofstream(gb / "media" / "kirby.png") << "png";
  std::ofstream(gb / "videos" / "Kirby.mkv") << "video";
  std::ofstream(gbc_alias / "Oracle.gbc") << "gbc-rom";
  std::ofstream(gbc_alias / "._Oracle.gbc") << "macos-sidecar";
  fs::create_directories(gbc_alias / "__MACOSX");
  std::ofstream(gbc_alias / "__MACOSX" / "Bad.gbc") << "macos-dir";
  std::ofstream(fc / "Contra.zip") << "fc-rom";
  std::ofstream(fc_hack / "Contra.zip") << "fc-hack-rom";
  std::ofstream(fc_hd / "HD.nes") << "fc-hd-rom";
  std::ofstream(md_picodrive / "Streets.zip") << "md-picodrive-rom";
  std::ofstream(nds / "Ys.nds") << "nds-rom";
  std::ofstream(psp / "Ridge.iso") << "psp-rom";
  std::ofstream(openbor / "Final Fight.pak") << "openbor-rom";
  std::ofstream(ports / "Balatro.sh") << "#!/bin/bash\nSHDIR=\"$(cd $(dirname \"$0\"); pwd)\"\nGAMEDIR=$SHDIR/Balatro\n";
  std::ofstream(ports / "小丑牌.sh") << "#!/bin/bash\nSHDIR=\"$(cd $(dirname \"$0\"); pwd)\"\nGAMEDIR=\"$SHDIR/Balatro\"\n";
  std::ofstream(ports / "Balatro" / "Balatro.love") << "ports-resource";
  std::ofstream(ports / "Balatro" / "Balatro.gptk") << "ports-controls";
  std::ofstream(ports / "Imgs" / "Balatro.png") << "ports-cover";
  std::ofstream(ports / "Imgs" / "小丑牌.png") << "ports-cover-cn";
  std::ofstream(java / "DoomRPG.jar") << "java-rom";
  std::ofstream(java / "Imgs" / "DoomRPG.png") << "java-cover";
  std::ofstream(saturn_ss / "001.chd") << "saturn-rom";
  std::ofstream(pico / "PicoGame.png") << "pico-cart";
  std::ofstream(pico / "Imgs" / "PicoGame.png") << "pico-cover";
  std::ofstream(fbneo_act / "dino.zip") << "fbneo-action-rom";
  std::ofstream(fbneo_act / "orphan.zip") << "fbneo-extra-rom";
  std::ofstream(fbneo_fly / "1944.zip") << "fbneo-rom";
  std::ofstream(fbneo_fly / "._1944.zip") << "macos-sidecar";
  std::ofstream(fbneo_fly / "1944u.zip") << "fbneo-rom-us";
  std::ofstream(fbneo_fly / "media" / "1944" / "boxFront.jpg") << "jpg";
  std::ofstream(fbneo_fly / "media" / "1944" / "logo.png") << "logo";
  std::ofstream(fbneo_fly / "media" / "1944" / "video.mp4") << "video";
  std::ofstream(mame_fly / "aerofgt.zip") << "mame-rom";
  std::ofstream(naomi / "ikaruga.zip") << "naomi-rom";
  std::ofstream(naomi / "ikaruga" / "gdl-0010.chd") << "naomi-chd-sidecar";
  {
    std::ofstream gamelist(gb / "gamelist.xml");
    gamelist << "<gameList>\n";
    gamelist << "  <game><path>./Tetris.gb</path><name>Tetris DX Metadata</name>";
    gamelist << "<desc>Stack blocks</desc><developer>Nintendo</developer>";
    gamelist << "<image>./media/tetris.png</image>";
    gamelist << "<marquee>./media/tetris-logo.png</marquee>";
    gamelist << "<video>./media/tetris-video.mp4</video></game>\n";
    gamelist << "  <game><path>./Kirby.gb</path><name>Kirby Metadata</name>";
    gamelist << "<image>./media/kirby.png</image>";
    gamelist << "<video>./video/Kirby.mp4</video></game>\n";
    gamelist << "</gameList>\n";
  }
  {
    std::ofstream metadata(fc / "metadata.pegasus.txt");
    metadata << "collection: FC\n\n";
    metadata << "game: Contra\n";
    metadata << "file: Contra.zip\n";
    metadata << "sort-by: 0100\n";
  }
  {
    std::ofstream metadata(fc_hack / "metadata.pegasus.txt");
    metadata << "collection: FC hack\n\n";
    metadata << "game: Contra Hack\n";
    metadata << "file: Contra.zip\n";
    metadata << "sort-by: 0101\n";
  }
  {
    std::ofstream metadata(fc_hd / "metadata.pegasus.txt");
    metadata << "collection: FC-HD\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture\n";
    metadata << "  -e LIBRETRO /data/data/com.retroarch.aarch64/cores/mesen_libretro_android.so\n\n";
    metadata << "game: HD Pack\n";
    metadata << "file: HD.nes\n";
    metadata << "sort-by: 0102\n";
  }
  {
    std::ofstream metadata(md_picodrive / "metadata.pegasus.txt");
    metadata << "collection: MD hack\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture\n";
    metadata << "  -e LIBRETRO /data/data/com.retroarch.aarch64/cores/picodrive_libretro_android.so\n\n";
    metadata << "game: 怒之铁拳 Picodrive\n";
    metadata << "file: Streets.zip\n";
    metadata << "sort-by: 0103\n";
  }
  {
    std::ofstream metadata(nds / "metadata.pegasus.txt");
    metadata << "collection: NDS\n";
    metadata << "launch: /mnt/vendor/deep/drastic/drastic \"{file.path}\"\n\n";
    metadata << "game: 伊苏1\n";
    metadata << "file: Ys.nds\n";
    metadata << "sort-by: 0104\n";
  }
  {
    std::ofstream metadata(psp / "metadata.pegasus.txt");
    metadata << "collection: PSP\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -n org.ppsspp.ppsspp/.PpssppActivity\n";
    metadata << "  -a android.intent.action.VIEW\n";
    metadata << "  -d {file.path}\n\n";
    metadata << "game: 山脊赛车\n";
    metadata << "file: Ridge.iso\n";
    metadata << "sort-by: 0105\n";
  }
  {
    std::ofstream metadata(saturn_ss / "metadata.pegasus.txt");
    metadata << "collection: SS\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture\n";
    metadata << "  -e LIBRETRO /data/data/com.retroarch.aarch64/cores/mednafen_saturn_libretro_android.so\n\n";
    metadata << "game: 月下夜想曲\n";
    metadata << "file: 001.chd\n";
    metadata << "sort-by: 041\n";
  }
  {
    std::ofstream metadata(fbneo_act / "metadata.pegasus.txt");
    metadata << "collection: 动作街机\n\n";
    metadata << "game: 恐龙快打\n";
    metadata << "file: dino.zip\n";
    metadata << "sort-by: 0001\n";
  }
  {
    std::ofstream metadata(fbneo_fly / "metadata.pegasus.txt");
    metadata << "collection: 飞机街机\n";
    metadata << "extensions: zip\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture\n";
    metadata << "  -e LIBRETRO /data/data/com.retroarch.aarch64/cores/fbneo_plus_libretro.so\n\n";
    metadata << "game: 1944 循环的征服者\n";
    metadata << "files:\n";
    metadata << "  1944.zip\n";
    metadata << "  1944u.zip\n";
    metadata << "sort-by: 0006\n";
    metadata << "publisher: Capcom\n";
    metadata << "genre: 射击-飞机竖版\n";
    metadata << "release: 1988\n";
    metadata << "x-id: FBNEO FLY\n";
    metadata << "description: Arcade shooter\n";
  }
  {
    std::ofstream metadata(mame_fly / "metadata.pegasus.txt");
    metadata << "collection: 飞机街机\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -n com.retroarch.aarch64/com.retroarch.browser.retroactivity.RetroActivityFuture\n";
    metadata << "  -e LIBRETRO /data/data/com.retroarch.aarch64/cores/mamearcade_libretro_android.so\n\n";
    metadata << "game: 音速战机\n";
    metadata << "file: aerofgt.zip\n";
    metadata << "sort-by: 0007\n";
  }
  {
    std::ofstream metadata(naomi / "metadata.pegasus.txt");
    metadata << "collection: NAOMI\n";
    metadata << "launch: am start --user 0\n";
    metadata << "  -e LIBRETRO /data/data/com.retroarch.aarch64/cores/flycast_libretro_android.so\n\n";
    metadata << "game: 斑鸠\n";
    metadata << "file: ikaruga.zip\n";
    metadata << "sort-by: 001\n";
    metadata << "publisher: Treasure\n";
  }

  H700RegistryOptions options;
  options.card_roots = {card.u8string()};
  options.retroarch_launcher = (system / "RA_launch.sh").u8string();
  options.nds_launcher = (system / "setNDS.sh").u8string();
  options.psp_launcher = (system / "PPSSPPSDL").u8string();
  options.openbor_setup_script = (system / "openbor.sh").u8string();
  options.openbor_launcher = (system / "OpenBOR.dge").u8string();
  options.ports_shell = (system / "bash").u8string();
  options.java_launcher = (system / "launch_java.sh").u8string();
  options.saturn_launcher = (system / "setSaturn.sh").u8string();
  options.saturn_emulator = (system / "yabasanshiro").u8string();
  options.saturn_bios = (system / "saturn_bios.bin").u8string();
  std::vector<Platform> platforms = LoadH700Platforms(options);
  const Platform *gb_platform = FindPlatformById(platforms, "gb");
  const Platform *gbc_platform = FindPlatformById(platforms, "gbc");
  assert(gb_platform);
  assert(gbc_platform);
  assert(gb_platform->directory_aliases.size() == 2);
  const Platform *fbneo_platform = FindPlatformById(platforms, "fbneo");
  assert(fbneo_platform);
  assert(gbc_platform->directory_aliases.size() == 3);
  assert(std::find(fbneo_platform->directory_aliases.begin(),
                   fbneo_platform->directory_aliases.end(),
                   "FBNEO FLY") != fbneo_platform->directory_aliases.end());
  assert(std::find(fbneo_platform->directory_aliases.begin(),
                   fbneo_platform->directory_aliases.end(),
                   "FBNEO ACT") != fbneo_platform->directory_aliases.end());
  const Platform *mame_platform = FindPlatformById(platforms, "mame");
  assert(mame_platform);
  assert(std::find(mame_platform->directory_aliases.begin(),
                   mame_platform->directory_aliases.end(),
                   "MAME FLY") != mame_platform->directory_aliases.end());
  const Platform *fc_hd_platform = FindPlatformById(platforms, "fc_hd");
  assert(fc_hd_platform);
  assert(fc_hd_platform->display_name == "FC-HD");
  assert(fc_hd_platform->launcher_id == "h700-retroarch-fc_hd");
  const Platform *md_platform = FindPlatformById(platforms, "md");
  assert(md_platform);
  assert(std::find(md_platform->directory_aliases.begin(),
                   md_platform->directory_aliases.end(),
                   "MD hack(picodrive)") != md_platform->directory_aliases.end());
  const Platform *nds_platform = FindPlatformById(platforms, "nds");
  assert(nds_platform);
  assert(nds_platform->display_name == "NDS");
  assert(nds_platform->launcher_id == "h700-standalone-nds");
  assert(nds_platform->launcher_kind == LauncherKind::Standalone);
  const Platform *psp_platform = FindPlatformById(platforms, "psp");
  assert(psp_platform);
  assert(psp_platform->display_name == "PSP");
  assert(psp_platform->launcher_id == "h700-standalone-psp");
  assert(psp_platform->launcher_kind == LauncherKind::Standalone);
  const Platform *openbor_platform = FindPlatformById(platforms, "openbor");
  assert(openbor_platform);
  assert(openbor_platform->display_name == "OPENBOR");
  assert(openbor_platform->launcher_id == "h700-standalone-openbor");
  assert(openbor_platform->launcher_kind == LauncherKind::Standalone);
  const Platform *ports_platform = FindPlatformById(platforms, "ports");
  assert(ports_platform);
  assert(ports_platform->display_name == "PORTS");
  assert(ports_platform->launcher_id == "h700-standalone-ports");
  assert(ports_platform->launcher_kind == LauncherKind::Standalone);
  const Platform *java_platform = FindPlatformById(platforms, "java");
  assert(java_platform);
  assert(java_platform->display_name == "JAVA");
  assert(java_platform->launcher_id == "h700-standalone-java");
  assert(java_platform->launcher_kind == LauncherKind::Standalone);
  const Platform *saturn_platform = FindPlatformById(platforms, "saturn");
  assert(saturn_platform);
  assert(saturn_platform->launcher_id == "h700-standalone-saturn");
  assert(saturn_platform->launcher_kind == LauncherKind::Standalone);
  assert(saturn_platform->launchable);
  assert(!saturn_platform->rom_directories.empty());
  assert(std::find(saturn_platform->directory_aliases.begin(),
                   saturn_platform->directory_aliases.end(),
                   "SATURN") != saturn_platform->directory_aliases.end());
  assert(std::find(saturn_platform->directory_aliases.begin(),
                   saturn_platform->directory_aliases.end(),
                   "SS") != saturn_platform->directory_aliases.end());

  const fs::path cache = root / "app-data" / "library" / "scan_cache.tsv";
  LibraryBuildReport first = LibraryBuilder().Build(platforms, cache.u8string());
  assert(first.rom_directory_games == 6);
  assert(first.emulationstation_games == 2);
  assert(first.anbernic_games == 4);
  assert(first.pegasus_games == 11);
  assert(first.merged_duplicates == 5);
  assert(first.cache_records_written > 0);
  assert(first.library.games.size() == 18);

  const Game *tetris = FindByTitle(first.library.games, "Tetris DX Metadata");
  const Game *kirby = FindByTitle(first.library.games, "Kirby Metadata");
  const Game *oracle = FindByTitle(first.library.games, "Oracle");
  const Game *contra = FindByTitle(first.library.games, "Contra");
  const Game *contra_hack = FindByTitle(first.library.games, "Contra Hack");
  const Game *hd_pack = FindByTitle(first.library.games, "HD Pack");
  const Game *md_picodrive_game = FindByTitle(first.library.games, "怒之铁拳 Picodrive");
  const Game *nds_game = FindByTitle(first.library.games, "伊苏1");
  const Game *psp_game = FindByTitle(first.library.games, "山脊赛车");
  const Game *openbor_game = FindByTitle(first.library.games, "Final Fight");
  const Game *ports_game = FindByTitle(first.library.games, "小丑牌");
  const Game *java_game = FindByTitle(first.library.games, "DoomRPG");
  const Game *pico_game = FindByTitle(first.library.games, "PicoGame");
  const Game *saturn_game = FindByTitle(first.library.games, "月下夜想曲");
  const Game *dino = FindByTitle(first.library.games, "恐龙快打");
  const Game *flyer = FindByTitle(first.library.games, "1944 循环的征服者");
  const Game *aerofgt = FindByTitle(first.library.games, "音速战机");
  const Game *ikaruga = FindByTitle(first.library.games, "斑鸠");
  assert(tetris);
  assert(kirby);
  assert(oracle);
  assert(contra);
  assert(contra_hack);
  assert(hd_pack);
  assert(md_picodrive_game);
  assert(nds_game);
  assert(psp_game);
  assert(openbor_game);
  assert(ports_game);
  assert(java_game);
  assert(pico_game);
  assert(saturn_game);
  assert(dino);
  assert(flyer);
  assert(aerofgt);
  assert(ikaruga);
  assert(!FindByTitle(first.library.games, "._Oracle"));
  assert(!FindByTitle(first.library.games, "._1944"));
  assert(!FindByTitle(first.library.games, "orphan"));
  assert(CountByTitle(first.library.games, "PicoGame") == 1);
  assert(!FindByTitle(first.library.games, "Balatro"));
  assert(CountByTitle(first.library.games, "小丑牌") == 1);
  assert(!FindByTitle(first.library.games, "Balatro.love"));
  assert(!FindByTitle(first.library.games, "Balatro.gptk"));
  assert(!FindByTitle(first.library.games, "Metroid"));
  assert(!FindByTitle(first.library.games, "Bad"));
  assert(!FindByTitle(first.library.games, "gdl-0010"));
  assert(tetris->source == "emulationstation");
  assert(tetris->developer == "Nintendo");
  assert(!tetris->media.cover.empty());
  assert(!tetris->media.logo.empty());
  assert(!tetris->media.video.empty());
  assert(kirby->source == "emulationstation");
  assert(kirby->media.video.find("videos") != std::string::npos);
  assert(kirby->media.video.find("Kirby.mkv") != std::string::npos);
  assert(!tetris->fingerprint.sample_hash.empty());
  assert(oracle->platform_id == "gbc");
  assert(contra->platform_id == "fc");
  assert(contra->collection_title == "FC");
  assert(contra_hack->platform_id == "fc");
  assert(contra_hack->collection_title == "FC hack");
  assert(contra->id != contra_hack->id);
  assert(contra->primary_target.path != contra_hack->primary_target.path);
  assert(hd_pack->platform_id == "fc_hd");
  assert(hd_pack->collection_title == "FC-HD");
  assert(hd_pack->collection_id == "collection:FC-HD");
  assert(hd_pack->launch_hint.platform_hint == "fc_hd");
  assert(hd_pack->launch_hint.core_hint == "mesen_libretro.so");
  assert(md_picodrive_game->platform_id == "md");
  assert(md_picodrive_game->collection_title == "MD hack");
  assert(md_picodrive_game->collection_id == "collection:MD hack");
  assert(md_picodrive_game->launch_hint.platform_hint == "md");
  assert(md_picodrive_game->launch_hint.core_hint == "picodrive_libretro.so");
  assert(nds_game->platform_id == "nds");
  assert(nds_game->collection_title == "NDS");
  assert(nds_game->collection_id == "collection:NDS");
  assert(nds_game->launch_hint.platform_hint == "nds");
  assert(nds_game->launch_hint.launcher_alias == "drastic");
  assert(psp_game->platform_id == "psp");
  assert(psp_game->collection_title == "PSP");
  assert(psp_game->collection_id == "collection:PSP");
  assert(psp_game->launch_hint.platform_hint == "psp");
  assert(psp_game->launch_hint.launcher_alias == "ppsspp");
  assert(openbor_game->platform_id == "openbor");
  assert(openbor_game->source == "rom-directory");
  assert(openbor_game->primary_target.path.find("Final Fight.pak") != std::string::npos);
  assert(ports_game->platform_id == "ports");
  assert(ports_game->source == "anbernic");
  assert(ports_game->primary_target.path.find("小丑牌.sh") != std::string::npos);
  assert(ports_game->alternate_targets.size() == 1);
  assert(ports_game->alternate_targets[0].path.find("Balatro.sh") != std::string::npos);
  assert(!ports_game->media.cover.empty());
  assert(ports_game->media.cover.find("Imgs") != std::string::npos);
  assert(java_game->platform_id == "java");
  assert(java_game->source == "anbernic");
  assert(java_game->primary_target.path.find("240x320") != std::string::npos);
  assert(!java_game->media.cover.empty());
  assert(java_game->media.cover.find("240x320") != std::string::npos);
  assert(java_game->media.cover.find("Imgs") != std::string::npos);
  assert(saturn_game->platform_id == "saturn");
  assert(saturn_game->collection_title == "SS");
  assert(saturn_game->collection_id == "collection:SS");
  assert(saturn_game->launch_hint.platform_hint == "saturn");
  assert(saturn_game->launch_hint.core_hint == "mednafen_saturn_libretro.so");
  assert(pico_game->platform_id == "pico");
  assert(pico_game->source == "anbernic");
  assert(!pico_game->media.cover.empty());
  assert(pico_game->media.cover.find("Imgs") != std::string::npos);
  assert(dino->platform_id == "fbneo");
  assert(dino->collection_title == "动作街机");
  assert(dino->collection_id == "collection:动作街机");
  assert(flyer->platform_id == "fbneo");
  assert(flyer->collection_title == "飞机街机");
  assert(flyer->collection_id == "collection:飞机街机");
  assert(flyer->source == "pegasus");
  assert(flyer->publisher == "Capcom");
  assert(flyer->genre == "射击-飞机竖版");
  assert(flyer->release == "1988");
  assert(flyer->external_id == "FBNEO FLY");
  assert(flyer->alternate_targets.size() == 1);
  assert(!flyer->media.cover.empty());
  assert(!flyer->media.logo.empty());
  assert(!flyer->media.video.empty());
  assert(flyer->launch_hint.platform_hint == "fbneo");
  assert(aerofgt->platform_id == "mame");
  assert(aerofgt->collection_title == "飞机街机");
  assert(aerofgt->collection_id == "collection:飞机街机");
  assert(aerofgt->launch_hint.platform_hint == "mame");
  assert(aerofgt->launch_hint.core_hint == "mame2022xtreme_libretro.so");
  assert(ikaruga->platform_id == "naomi");
  assert(ikaruga->collection_title == "NAOMI");
  assert(ikaruga->collection_id == "collection:NAOMI");
  assert(ikaruga->source == "pegasus");
  assert(ikaruga->publisher == "Treasure");
  for (const Game &game : first.library.games) {
    if (game.platform_id == "dreamcast" || game.platform_id == "fbneo" ||
        game.platform_id == "mame" || game.platform_id == "naomi") {
      assert(!game.collection_id.empty());
    }
  }

  LibraryBuildReport second = LibraryBuilder().Build(platforms, cache.u8string());
  assert(second.skipped_roots > 0);
  assert(second.cached_games == 18);
  assert(second.library.games.size() == 18);
  const Game *cached_tetris = FindByTitle(second.library.games, "Tetris DX Metadata");
  assert(cached_tetris);
  assert(cached_tetris->source == "emulationstation");
  assert(cached_tetris->developer == "Nintendo");
  assert(cached_tetris->description == "Stack blocks");
  assert(cached_tetris->media.cover == tetris->media.cover);
  assert(cached_tetris->media.video == tetris->media.video);
  const Game *cached_kirby = FindByTitle(second.library.games, "Kirby Metadata");
  assert(cached_kirby);
  assert(cached_kirby->media.video == kirby->media.video);
  const Game *cached_flyer = FindByTitle(second.library.games, "1944 循环的征服者");
  assert(cached_flyer);
  assert(cached_flyer->collection_title == "飞机街机");
  assert(cached_flyer->primary_target.label == "1944.zip");
  assert(cached_flyer->alternate_targets.size() == 1);
  const Game *cached_aerofgt = FindByTitle(second.library.games, "音速战机");
  assert(cached_aerofgt);
  assert(cached_aerofgt->collection_title == "飞机街机");
  assert(cached_aerofgt->launch_hint.core_hint == "mame2022xtreme_libretro.so");
  assert(!FindByTitle(second.library.games, "orphan"));
  const Game *cached_ikaruga = FindByTitle(second.library.games, "斑鸠");
  assert(cached_ikaruga);
  assert(cached_ikaruga->collection_title == "NAOMI");
  const Game *cached_hd_pack = FindByTitle(second.library.games, "HD Pack");
  assert(cached_hd_pack);
  assert(cached_hd_pack->platform_id == "fc_hd");
  assert(cached_hd_pack->launch_hint.core_hint == "mesen_libretro.so");
  const Game *cached_md_picodrive_game = FindByTitle(second.library.games, "怒之铁拳 Picodrive");
  assert(cached_md_picodrive_game);
  assert(cached_md_picodrive_game->platform_id == "md");
  assert(cached_md_picodrive_game->launch_hint.core_hint == "picodrive_libretro.so");
  const Game *cached_nds_game = FindByTitle(second.library.games, "伊苏1");
  assert(cached_nds_game);
  assert(cached_nds_game->platform_id == "nds");
  assert(cached_nds_game->launch_hint.launcher_alias == "drastic");
  const Game *cached_psp_game = FindByTitle(second.library.games, "山脊赛车");
  assert(cached_psp_game);
  assert(cached_psp_game->platform_id == "psp");
  assert(cached_psp_game->launch_hint.launcher_alias == "ppsspp");
  const Game *cached_openbor_game = FindByTitle(second.library.games, "Final Fight");
  assert(cached_openbor_game);
  assert(cached_openbor_game->platform_id == "openbor");
  const Game *cached_ports_game = FindByTitle(second.library.games, "小丑牌");
  assert(cached_ports_game);
  assert(cached_ports_game->platform_id == "ports");
  assert(cached_ports_game->alternate_targets.size() == 1);
  const Game *cached_java_game = FindByTitle(second.library.games, "DoomRPG");
  assert(cached_java_game);
  assert(cached_java_game->platform_id == "java");
  assert(cached_java_game->media.cover == java_game->media.cover);
  const Game *cached_saturn_game = FindByTitle(second.library.games, "月下夜想曲");
  assert(cached_saturn_game);
  assert(cached_saturn_game->platform_id == "saturn");
  assert(cached_saturn_game->launch_hint.core_hint == "mednafen_saturn_libretro.so");
  const Game *cached_pico_game = FindByTitle(second.library.games, "PicoGame");
  assert(cached_pico_game);
  assert(cached_pico_game->platform_id == "pico");
  assert(cached_pico_game->media.cover == pico_game->media.cover);
  assert(CountByTitle(second.library.games, "PicoGame") == 1);

  const fs::path ps_root = root / "disc" / "PS";
  fs::create_directories(ps_root);
  std::ofstream(ps_root / "Ridge Racer.cue") << "FILE \"track01.bin\" BINARY\n";
  std::ofstream(ps_root / "track01.bin") << "track";
  Platform ps;
  ps.id = "ps";
  ps.display_name = "PS";
  ps.rom_directories = {ps_root.u8string()};
  ps.extensions = {".cue", ".chd", ".m3u", ".bin"};
  ps.launcher_id = "fixture-standalone-ps";
  ps.launcher_kind = LauncherKind::Standalone;
  ps.launchable = true;

  LibraryBuildReport disc = LibraryBuilder().Build({ps});
  assert(disc.rom_directory_games == 1);
  assert(disc.library.games.size() == 1);
  assert(disc.library.games[0].title == "Ridge Racer");
  assert(disc.library.games[0].multi_file_entry);

  const fs::path saturn_image_root = root / "disc" / "SATURN";
  fs::create_directories(saturn_image_root);
  std::ofstream(saturn_image_root / "Solo.iso") << "saturn-iso";
  std::ofstream(saturn_image_root / "Standalone.bin") << "saturn-bin";
  std::ofstream(saturn_image_root / "Cue Game.cue") << "FILE \"track01.bin\" BINARY\n";
  std::ofstream(saturn_image_root / "track01.bin") << "track";
  Platform saturn;
  saturn.id = "saturn";
  saturn.display_name = "SATURN";
  saturn.rom_directories = {saturn_image_root.u8string()};
  saturn.extensions = {".cue", ".chd", ".iso", ".bin"};
  saturn.launcher_id = "fixture-standalone-saturn";
  saturn.launcher_kind = LauncherKind::Standalone;
  saturn.launchable = true;

  LibraryBuildReport saturn_disc = LibraryBuilder().Build({saturn});
  assert(saturn_disc.rom_directory_games == 3);
  assert(saturn_disc.library.games.size() == 3);
  assert(FindByTitle(saturn_disc.library.games, "Solo"));
  assert(FindByTitle(saturn_disc.library.games, "Standalone"));
  assert(FindByTitle(saturn_disc.library.games, "Cue Game"));
  assert(!FindByTitle(saturn_disc.library.games, "track01"));

  fs::remove_all(root);
  return 0;
}

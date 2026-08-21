#include "app/demo_library.h"

namespace mpl {

namespace {

Platform DemoPlatform(const std::string &id, const std::string &name, int sort_order) {
  Platform platform;
  platform.id = id;
  platform.display_name = name;
  platform.sort_order = sort_order;
  platform.launcher_id = "demo-retroarch-" + id;
  platform.launchable = true;
  return platform;
}

Game DemoGame(const std::string &id, const std::string &platform_id, const std::string &title,
              const std::string &developer) {
  Game game;
  game.id = id;
  game.platform_id = platform_id;
  game.title = title;
  game.sort_key = title;
  game.developer = developer;
  game.description = "Demo library entry";
  game.primary_target.path = "/demo/Roms/" + platform_id + "/" + title;
  game.primary_target.label = title;
  game.source = "demo";
  return game;
}

}  // namespace

Library BuildDemoLibrary() {
  Library library;
  library.platforms = {
      DemoPlatform("gb", "GB", 10),
      DemoPlatform("gbc", "GBC", 20),
      DemoPlatform("gba", "GBA", 30),
  };
  library.games = {
      DemoGame("gb-tetris", "gb", "Tetris", "Nintendo"),
      DemoGame("gbc-zelda", "gbc", "Zelda Oracle", "Capcom"),
      DemoGame("gba-metroid", "gba", "Metroid Fusion", "Nintendo"),
      DemoGame("gba-castlevania", "gba", "Castlevania Aria", "Konami"),
  };
  library.games[0].favorite = true;
  library.games[2].recent_order = 2;
  library.games[1].recent_order = 1;
  return library;
}

}  // namespace mpl

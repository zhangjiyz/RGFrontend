#include "catalog/arcade_name_database.h"

#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

#include "catalog/path.h"

namespace fs = std::filesystem;

namespace mpl {

namespace {

bool ParseCsvRow(const std::string &line, std::vector<std::string> *fields) {
  fields->clear();
  std::string field;
  bool quoted = false;
  for (std::size_t index = 0; index < line.size(); ++index) {
    const char ch = line[index];
    if (ch == '"') {
      if (quoted && index + 1 < line.size() && line[index + 1] == '"') {
        field.push_back('"');
        ++index;
      } else {
        quoted = !quoted;
      }
    } else if (ch == ',' && !quoted) {
      fields->push_back(std::move(field));
      field.clear();
    } else if (ch != '\r') {
      field.push_back(ch);
    }
  }
  fields->push_back(std::move(field));
  return !quoted;
}

void RemoveUtf8Bom(std::string *value) {
  if (value->size() >= 3 && static_cast<unsigned char>((*value)[0]) == 0xef &&
      static_cast<unsigned char>((*value)[1]) == 0xbb &&
      static_cast<unsigned char>((*value)[2]) == 0xbf) {
    value->erase(0, 3);
  }
}

}  // namespace

bool ArcadeNameDatabase::Load(const std::string &path) {
  titles_.clear();
  std::ifstream input(fs::u8path(path));
  if (!input) return false;

  std::string line;
  std::vector<std::string> fields;
  while (std::getline(input, line)) {
    if (!ParseCsvRow(line, &fields) || fields.size() < 2) continue;
    RemoveUtf8Bom(&fields[0]);
    const std::string key = LowerAscii(fields[0]);
    if (key.empty()) continue;
    std::string title = fields[1];
    if (title.empty() && fields.size() >= 3) title = fields[2];
    if (title.empty()) continue;
    titles_.emplace(key, std::move(title));
  }
  return !titles_.empty();
}

bool ArcadeNameDatabase::Apply(Game *game) const {
  if (!game || game->primary_target.path.empty()) return false;
  const std::string stem = fs::u8path(game->primary_target.path).stem().u8string();
  if (game->title != stem) return false;
  const auto found = titles_.find(LowerAscii(stem));
  if (found == titles_.end()) return false;
  if (game->sort_key.empty() || game->sort_key == game->title) {
    game->sort_key = found->second;
  }
  game->title = found->second;
  return true;
}

}  // namespace mpl

#include "catalog/ports_alias.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "catalog/path.h"

namespace fs = std::filesystem;

namespace mpl {
namespace {

std::string Trim(std::string value) {
  std::size_t begin = 0;
  while (begin < value.size() &&
         std::isspace(static_cast<unsigned char>(value[begin]))) {
    ++begin;
  }
  std::size_t end = value.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(begin, end - begin);
}

std::string ReadPrefix(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return {};
  constexpr std::size_t kMaxBytes = 64 * 1024;
  std::string text(kMaxBytes, '\0');
  input.read(&text[0], static_cast<std::streamsize>(text.size()));
  text.resize(static_cast<std::size_t>(input.gcount()));
  return text;
}

std::string AssignmentValue(const std::string &line, const std::string &key) {
  std::string text = Trim(line);
  constexpr const char *kExport = "export ";
  if (text.rfind(kExport, 0) == 0) {
    text = Trim(text.substr(std::char_traits<char>::length(kExport)));
  }
  const std::size_t equals = text.find('=');
  if (equals == std::string::npos) return {};
  if (Trim(text.substr(0, equals)) != key) return {};
  return Trim(text.substr(equals + 1));
}

std::string FirstShellToken(std::string value) {
  value = Trim(std::move(value));
  if (value.empty()) return {};
  if (value.front() == '"' || value.front() == '\'') {
    const char quote = value.front();
    std::string token;
    for (std::size_t index = 1; index < value.size(); ++index) {
      if (value[index] == quote) return token;
      token.push_back(value[index]);
    }
    return token;
  }
  std::size_t end = 0;
  while (end < value.size() &&
         !std::isspace(static_cast<unsigned char>(value[end])) &&
         value[end] != '#' && value[end] != ';') {
    ++end;
  }
  return value.substr(0, end);
}

void ReplaceAll(std::string *text, const std::string &from, const std::string &to) {
  if (!text || from.empty()) return;
  std::size_t position = 0;
  while ((position = text->find(from, position)) != std::string::npos) {
    text->replace(position, from.size(), to);
    position += to.size();
  }
}

std::string ResolveGamedirToken(const fs::path &root, const fs::path &script_path,
                                std::string token) {
  if (token.empty()) return {};
  const std::string script_dir = NormalizedPath(script_path.parent_path()).u8string();
  ReplaceAll(&token, "${SHDIR}", script_dir);
  ReplaceAll(&token, "$SHDIR", script_dir);
  if (token.find('$') != std::string::npos) return {};

  fs::path candidate = fs::u8path(token);
  if (!candidate.is_absolute()) candidate = script_path.parent_path() / candidate;
  candidate = NormalizedPath(candidate);
  std::error_code error;
  if (!fs::is_directory(candidate, error)) return {};
  if (!PathIsInside(candidate, root)) return {};
  const std::string relative = RelativeIdentityPath(candidate, root);
  return relative.empty() ? std::string() : relative;
}

}  // namespace

std::string PortsEntryIdentityPath(const fs::path &root, const fs::path &script_path) {
  const std::string text = ReadPrefix(script_path);
  std::size_t begin = 0;
  while (begin <= text.size()) {
    const std::size_t end = text.find('\n', begin);
    const std::string line = text.substr(begin, end == std::string::npos
                                                    ? std::string::npos
                                                    : end - begin);
    const std::string value = AssignmentValue(line, "GAMEDIR");
    const std::string token = FirstShellToken(value);
    const std::string identity = ResolveGamedirToken(root, script_path, token);
    if (!identity.empty()) return identity;
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  return {};
}

}  // namespace mpl

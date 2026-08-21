#pragma once

#include <filesystem>
#include <string>

namespace mpl {

std::string PortsEntryIdentityPath(const std::filesystem::path &root,
                                   const std::filesystem::path &script_path);

}  // namespace mpl

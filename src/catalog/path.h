#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace mpl {

std::string LowerAscii(std::string value);
std::string StableId(std::string_view text);
std::filesystem::path NormalizedPath(const std::filesystem::path &path);
bool HasAllowedExtension(const std::filesystem::path &path,
                         const std::vector<std::string> &extensions);
bool IsLikelySidecarFile(const std::filesystem::path &path);
bool IsIgnoredDataPath(const std::filesystem::path &path);
bool PathIsInside(const std::filesystem::path &path, const std::filesystem::path &root);
std::string RelativeIdentityPath(const std::filesystem::path &path,
                                 const std::filesystem::path &root);
std::int64_t PortableModifiedTime(const std::filesystem::path &path);
std::string SampleFingerprint(const std::filesystem::path &path);

}  // namespace mpl

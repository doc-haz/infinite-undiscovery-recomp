#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace iu {

enum class ContentProfileId {
  kUsa,
  kUsaUndub,
  kEurope,
  kJapan,
  kAsia
};

struct ContentProfileInfo {
  ContentProfileId id;
  std::string folder_name;        // Official folder: "USA", "USA-UNDUB", "EUROPE", "JAPAN", "ASIA"
  std::wstring display_name;      // UI name: L"USA", L"USA UNDUB (Japanese Voices)", L"Europe", L"Japan", L"Asia (English)"
  std::string edition_code;       // Internal edition code: "USA", "USA-UNDUB", "EUROPE", "JAPAN", "ASIA"
  std::string legacy_folder_name; // Legacy directory name if any: "NTSC-U", "NTSC-U-UNDUB", "PAL", etc.

  std::filesystem::path disc1_root(const std::filesystem::path& profile_root) const {
    return profile_root / "assets" / "disc1";
  }
  std::filesystem::path disc2_root(const std::filesystem::path& profile_root) const {
    return profile_root / "assets" / "disc2";
  }
  std::filesystem::path dlc_root(const std::filesystem::path& profile_root) const {
    return profile_root / "assets" / "dlc";
  }
  std::filesystem::path shader_root(const std::filesystem::path& profile_root) const {
    return profile_root / "shaders";
  }
  std::filesystem::path cache_root(const std::filesystem::path& profile_root) const {
    return profile_root / "cache";
  }
  std::filesystem::path saves_root(const std::filesystem::path& profile_root) const {
    return profile_root / "saves";
  }
  std::filesystem::path logs_root(const std::filesystem::path& profile_root) const {
    return profile_root / "logs";
  }
};

inline const std::vector<ContentProfileInfo>& GetAllProfiles() {
  static const std::vector<ContentProfileInfo> kProfiles = {
    {ContentProfileId::kUsa, "USA", L"USA", "USA", "NTSC-U"},
    {ContentProfileId::kUsaUndub, "USA-UNDUB", L"USA UNDUB (Japanese Voices)", "USA-UNDUB", "NTSC-U-UNDUB"},
    {ContentProfileId::kEurope, "EUROPE", L"Europe", "EUROPE", "PAL"},
    {ContentProfileId::kJapan, "JAPAN", L"Japan", "JAPAN", ""},
    {ContentProfileId::kAsia, "ASIA", L"Asia (English)", "ASIA", ""}
  };
  return kProfiles;
}

inline const ContentProfileInfo* FindProfileById(ContentProfileId id) {
  for (const auto& p : GetAllProfiles()) {
    if (p.id == id) return &p;
  }
  return nullptr;
}

inline const ContentProfileInfo* FindProfileByFolder(std::string_view folder) {
  for (const auto& p : GetAllProfiles()) {
    if (p.folder_name == folder) return &p;
  }
  // Check legacy names
  for (const auto& p : GetAllProfiles()) {
    if (!p.legacy_folder_name.empty() && p.legacy_folder_name == folder) return &p;
  }
  return nullptr;
}

inline const ContentProfileInfo* FindProfileByEdition(std::string_view edition) {
  for (const auto& p : GetAllProfiles()) {
    if (p.edition_code == edition) return &p;
  }
  // Also tolerate legacy edition tokens like "PAL" -> "EUROPE"
  if (edition == "PAL") {
    return FindProfileByEdition("EUROPE");
  }
  return nullptr;
}

inline std::string ProfileFolderFromEdition(std::string_view edition) {
  const auto* p = FindProfileByEdition(edition);
  return p ? p->folder_name : std::string(edition);
}

} // namespace iu

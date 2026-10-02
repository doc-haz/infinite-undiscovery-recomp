#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include "asset_dlc.h"

namespace iu::assets {
namespace fs = std::filesystem;
struct File { std::string name; uint64_t offset = 0, size = 0; fs::path host; };
struct Disc {
  fs::path source;
  bool image = false;
  unsigned number = 0;
  uint32_t title = 0, region = 0;
  std::string edition, media_id, xex_sha256;
  uint32_t version = 0, base_version = 0;
  std::array<std::string, 2> multidisc_ids;
  std::vector<File> files;
};
// Throws on malformed/foreign/unsupported data. Folder names are never identity.
Disc Inspect(const fs::path& source);
void ValidatePair(const Disc& first, const std::optional<Disc>& second);
bool Ready(const fs::path& root, std::string* reason = nullptr);
bool CanLaunch(const Disc& disc);
using Progress = std::function<void(uint64_t, uint64_t)>;
fs::path Install(const Disc& first, const std::optional<Disc>& second,
                 const fs::path& parent, const Progress& progress,
                 const std::function<bool()>& cancelled,
                 const std::vector<iu::dlc::Package>& dlc = {});
std::optional<fs::path> Pick(bool folder, void* owner = nullptr);
std::vector<fs::path> PickPackages(void* owner = nullptr);
// Windows native UI; nullopt means cancelled. Does not start the runtime.
std::optional<fs::path> Wizard(const fs::path& suggested);
}

#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
namespace rex::system { class KernelState; }
namespace iu::dlc {
struct Entry { std::string name, sha256; uint32_t size = 0; };
struct Package {
  std::filesystem::path source;
  std::string filename, content_id, sha256;
  std::wstring display_name;
  uint32_t license_mask = 0;
  uint64_t size = 0;
  std::vector<Entry> entries;
};
// Read-only inspection. Current decoder accepts small, read-only STFS packages.
// Unsupported structures are rejected explicitly; no filename-based identity.
Package Inspect(const std::filesystem::path& source);
void ValidateSelection(const std::vector<Package>& packages);
// Uses the SDK importer with an isolated manager, then verifies and publishes.
// Never replaces an existing package or enumeration header with different data.
void InstallPending(const std::filesystem::path& packages,
                    rex::system::KernelState* kernel,
                    const std::filesystem::path& user_root);
uint32_t GetInstalledCount();
}

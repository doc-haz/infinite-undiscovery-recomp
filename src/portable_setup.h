#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <windows.h>

#include "asset_setup.h"
#include "asset_dlc.h"

namespace iu::portable {
namespace fs = std::filesystem;
struct Paths { fs::path root, assets, saves, shaders, cache, logs, config; };
Paths Layout(const fs::path& exe, const std::string& region);
std::optional<fs::path> Select(const fs::path& exe, bool maintenance);
void Initialize(const fs::path& exe);
void SaveLanguage();
bool Spanish();
void SetSpanish(bool value);
// Mods (persistidos en setup.json; los lee el hook en runtime).  EsTranslation/
// EsTextures devuelven el valor EFECTIVO (mod activado && opcion activada).
bool EsTranslation();
bool EsTextures();
bool EsSelftest();
void SetEsTranslation(bool value);
void SetEsTextures(bool value);
void SetEsSelftest(bool value);
bool IsUndubSubtitleWarningDismissed();
void SetUndubSubtitleWarningDismissed(bool value);
std::string Text(const std::string& spanish);
std::wstring Wide(const std::string& utf8);
std::string Utf8(const std::wstring& wide);
std::wstring Text(const std::wstring& spanish);
struct Button { int id; std::wstring label; };
int Dialog(const std::wstring& heading, const std::wstring& text,
           const std::vector<Button>& buttons, std::function<bool(int)> onAction = {},
           std::function<std::wstring()> poll = {});
std::optional<fs::path> RunWizard(const fs::path& exe,
                                  const std::optional<std::string>& target_edition = std::nullopt);
std::optional<fs::path> RunWizard(const fs::path& exe,
                                  const std::optional<iu::assets::Disc>& initial_d1,
                                  const std::optional<iu::assets::Disc>& initial_d2,
                                  const std::vector<iu::dlc::Package>& initial_dlc,
                                  const std::optional<std::string>& target_edition = std::nullopt);
std::optional<fs::path> RunProfileManager(const fs::path& exe);
void Error(const std::string& message);

std::string GetActiveProfile();
void SetActiveProfile(const std::string& profile_folder);
void MigrateLegacyProfiles(const fs::path& exe);
}

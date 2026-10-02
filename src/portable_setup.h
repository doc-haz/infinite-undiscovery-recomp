#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <windows.h>

namespace iu::portable {
namespace fs = std::filesystem;
struct Paths { fs::path root, assets, saves, shaders, cache, logs, config; };
Paths Layout(const fs::path& exe, const std::string& region);
std::optional<fs::path> Select(const fs::path& exe, bool maintenance);
void Initialize(const fs::path& exe);
void SaveLanguage();
bool Spanish();
void SetSpanish(bool value);
std::string Text(const std::string& spanish);
std::wstring Wide(const std::string& utf8);
std::wstring Text(const std::wstring& spanish);
struct Button { int id; std::wstring label; };
int Dialog(const std::wstring& heading, const std::wstring& text,
           const std::vector<Button>& buttons, std::function<bool(int)> onAction = {},
           std::function<std::wstring()> poll = {});
std::optional<fs::path> RunWizard(const fs::path& exe);
void Error(const std::string& message);
}

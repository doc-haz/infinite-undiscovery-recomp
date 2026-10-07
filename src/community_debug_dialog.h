#pragma once

#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <imgui.h>
#include <functional>
#include <rex/cvar.h>
#include <rex/filesystem.h>
#if REX_HAS_D3D12
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "save_anywhere.h"
#include "recovery_move.h"
#include "portable_setup.h"
#include "content_profile.h"
#include "disc_swap.h"
#include "asset_dlc.h"
#include "version.h"
#include "iu_trace.h"

REXCVAR_DECLARE(bool, pso_prewarm);
REXCVAR_DECLARE(bool, pso_telemetry);

namespace iu::ui {

// Shared UI-thread actions: buttons and direct keybinds use the same guards,
// guest request queues and feedback. Guest-side validation remains authoritative.
struct CommunityDebugActions {
  std::string last_feedback;
  uint64_t feedback_time_ms{0};
  bool feedback_success{true};

  void SetFeedback(bool success, const std::string& msg) {
    feedback_success = success;
    last_feedback = msg;
    feedback_time_ms = GetTickCount64();
  }

  bool SaveAnywhere() {
    const bool es = iu::portable::Spanish();
    if (iu::save_anywhere::IsActive()) {
      SetFeedback(false, es ? "Guardar en Cualquier Lugar: El dialogo ya esta activo; completelo o cancelelo primero"
                            : "Save Anywhere: Dialog already active; complete or cancel it first");
      return false;
    }
    iu::save_anywhere::Request();
    SetFeedback(true, es ? "Guardar en Cualquier Lugar: Solicitud iniciada"
                         : "Save Anywhere: Request initiated");
    return true;
  }

  void SafeStep() {
    const bool es = iu::portable::Spanish();
    if (iu::save_anywhere::IsActive()) {
      SetFeedback(false, es ? "Movimiento rechazado: Guardar en Cualquier Lugar o dialogo modal nativo esta activo"
                            : "Movement rejected: Save Anywhere or native modal dialog is active");
      return;
    }
    iu::recovery_move::RequestSafeStep(300.0f);
    SetFeedback(true, es ? "Paso Adelante Seguro: Solicitud en cola (300 unidades hacia adelante)"
                         : "Safe Step: Request queued (300 units forward)");
  }

  void Undo() {
    const bool es = iu::portable::Spanish();
    if (iu::save_anywhere::IsActive()) {
      SetFeedback(false, es ? "Deshacer rechazado: Guardar en Cualquier Lugar o dialogo modal nativo esta activo"
                            : "Undo rejected: Save Anywhere or native modal dialog is active");
      return;
    }
    if (!iu::recovery_move::CanUndo()) {
      SetFeedback(false, es ? "Deshacer: No hay movimientos previos en la escena/contexto actual"
                            : "Undo: No prior debug movement in current scene/context");
      return;
    }
    iu::recovery_move::RequestUndo();
    SetFeedback(true, es ? "Deshacer Movimiento: Restauracion de posicion en cola"
                         : "Undo Move: Position restore queued");
  }

  std::function<void()> on_relaunch_profile_manager;
  void RelaunchProfileManager() {
    if (on_relaunch_profile_manager) {
      on_relaunch_profile_manager();
    }
  }

  std::function<void()> on_quit_game;
  void QuitGame() {
    if (on_quit_game) {
      on_quit_game();
    }
  }
};

inline std::string ActionLabel(const char* action, const char* bind) {
  const auto key = rex::cvar::GetFlagByName(bind);
  return std::string(action) + " (" + (key.empty() ? "Unbound" : key) + ")";
}

inline std::string GetGpuAdapterName() {
#if REX_HAS_D3D12
  Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
  if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    std::string cvar_idx = rex::cvar::GetFlagByName("d3d12_adapter");
    UINT index = cvar_idx.empty() ? 0 : static_cast<UINT>(std::strtoul(cvar_idx.c_str(), nullptr, 10));
    if (SUCCEEDED(factory->EnumAdapters1(index, &adapter))) {
      DXGI_ADAPTER_DESC1 desc{};
      if (SUCCEEDED(adapter->GetDesc1(&desc))) {
        char name[256]{};
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), nullptr, nullptr);
        return name;
      }
    }
  }
#endif
  return "Direct3D 12 Device";
}

inline std::string GetOsVersionString() {
  typedef LONG(NTAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
  HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
  if (hNtdll) {
    auto fn = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(hNtdll, "RtlGetVersion"));
    if (fn) {
      RTL_OSVERSIONINFOW rovi{sizeof(rovi)};
      if (fn(&rovi) == 0) {
        std::ostringstream ss;
        if (rovi.dwMajorVersion == 10 && rovi.dwBuildNumber >= 22000) {
          ss << "Windows 11 (Build " << rovi.dwBuildNumber << ")";
        } else if (rovi.dwMajorVersion == 10) {
          ss << "Windows 10 (Build " << rovi.dwBuildNumber << ")";
        } else {
          ss << "Windows NT " << rovi.dwMajorVersion << "." << rovi.dwMinorVersion << " (Build " << rovi.dwBuildNumber << ")";
        }
        return ss.str();
      }
    }
  }
  return "Windows (Unknown Version)";
}

inline std::string GetMemoryString() {
  MEMORYSTATUSEX ms{sizeof(ms)};
  if (GlobalMemoryStatusEx(&ms)) {
    uint64_t total_mb = ms.ullTotalPhys / (1024 * 1024);
    uint64_t avail_mb = ms.ullAvailPhys / (1024 * 1024);
    std::ostringstream ss;
    ss << total_mb << " MB Total, " << avail_mb << " MB Available";
    return ss.str();
  }
  return "Unknown";
}

inline std::string GetCpuInfoString() {
  unsigned int cores = std::thread::hardware_concurrency();
  std::ostringstream ss;
  ss << cores << " logical cores";
  return ss.str();
}

inline std::string GetCompilerString() {
#if defined(__clang__)
  return "Clang " + std::string(__clang_version__);
#elif defined(_MSC_VER)
  return "MSVC " + std::to_string(_MSC_VER);
#elif defined(__GNUC__)
  return "GCC " + std::string(__VERSION__);
#else
  return "Unknown Compiler";
#endif
}

inline std::string GetBuildTypeString() {
#if defined(NDEBUG)
  return "Release";
#else
  return "Debug";
#endif
}

inline std::string SanitizePathString(const std::string& input) {
  std::string output = input;
  wchar_t username[256]{};
  DWORD size = 256;
  if (GetUserNameW(username, &size) && size > 1) {
    int n = WideCharToMultiByte(CP_UTF8, 0, username, -1, nullptr, 0, nullptr, nullptr);
    if (n > 1) {
      std::string u(n - 1, 0);
      WideCharToMultiByte(CP_UTF8, 0, username, -1, u.data(), n - 1, nullptr, nullptr);
      std::string patterns[] = {
        "\\Users\\" + u,
        "/Users/" + u,
        "\\users\\" + u,
        "/users/" + u
      };
      for (const auto& pat : patterns) {
        size_t pos = 0;
        while (pos < output.size()) {
          auto it = std::search(output.begin() + pos, output.end(),
                                pat.begin(), pat.end(),
                                [](char a, char b) { return tolower(static_cast<unsigned char>(a)) == tolower(static_cast<unsigned char>(b)); });
          if (it == output.end()) break;
          size_t idx = std::distance(output.begin(), it);
          std::string replacement = (pat[0] == '/' ? "/Users/<USER>" : "\\Users\\<USER>");
          output.replace(idx, pat.size(), replacement);
          pos = idx + replacement.size();
        }
      }
    }
  }
  return output;
}

inline std::string BuildDiagnosticReportText(bool sanitize = true) {
  auto exe = rex::filesystem::GetExecutableFolder();
  auto profile = iu::portable::GetActiveProfile();
  if (profile.empty()) profile = "USA";
  auto layout = iu::portable::Layout(exe, profile);
  const auto* pinfo = iu::FindProfileByFolder(profile);
  std::string display_name = pinfo ? iu::portable::Utf8(pinfo->display_name) : profile;
  uint32_t current_disc = iu::disc_swap::GetCurrentDisc();
  uint32_t dlc_count = iu::dlc::GetInstalledCount();
  bool d1_present = std::filesystem::is_directory(layout.assets / "disc1");
  bool d2_present = std::filesystem::is_directory(layout.assets / "disc2");
  std::string mount = (current_disc == 2) ? "\\Device\\IUDisc2" : "\\Device\\Harddisk0\\Partition1";

  SYSTEMTIME st;
  GetLocalTime(&st);
  char date_buf[64];
  std::snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d %02d:%02d:%02d",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

  std::ostringstream ss;
  ss << "### Infinite Undiscovery Recomp - Diagnostic Report\n";
  ss << "Generated: " << date_buf << "\n\n";

  ss << "#### Game\n";
  ss << "- **Active Profile**: " << profile << "\n";
  ss << "- **Display Name**: " << display_name << "\n";
  ss << "- **Current Disc**: Disc " << current_disc << "\n";
  ss << "- **Title ID**: 0x535107DB\n";
  ss << "- **Region / Edition**: " << (pinfo ? pinfo->edition_code : profile) << "\n";
  ss << "- **UI Language**: " << (iu::portable::Spanish() ? "es" : "en") << "\n";
  ss << "- **DLC Packages**: " << dlc_count << "\n\n";

  ss << "#### Build\n";
  ss << "- **Version**: " << iu::kProjectVersion << "\n";
  ss << "- **Build Type**: " << GetBuildTypeString() << "\n";
  ss << "- **Build Timestamp**: " << __DATE__ << " " << __TIME__ << "\n";
  ss << "- **Compiler**: " << GetCompilerString() << "\n\n";

  ss << "#### Runtime\n";
  ss << "- **ReXGlue SDK**: 0.10.0\n";
  ss << "- **Disc Mount**: " << mount << "\n";
  ss << "- **Disc 1 Status**: " << (d1_present ? "Present (Ready)" : "Missing") << "\n";
  ss << "- **Disc 2 Status**: " << (d2_present ? "Present (Ready)" : "Not installed / Missing") << "\n\n";

  ss << "#### Graphics\n";
  ss << "- **Backend**: D3D12\n";
  ss << "- **Adapter**: " << GetGpuAdapterName() << "\n";
  ImGuiIO& io = ImGui::GetIO();
  ss << "- **Resolution**: " << static_cast<int>(io.DisplaySize.x) << "x" << static_cast<int>(io.DisplaySize.y) << "\n\n";

  ss << "#### System\n";
  ss << "- **OS**: " << GetOsVersionString() << "\n";
  ss << "- **CPU**: " << GetCpuInfoString() << "\n";
  ss << "- **Memory**: " << GetMemoryString() << "\n\n";

  auto sanitize_fn = [&](const std::filesystem::path& p) {
    std::string s = p.string();
    return sanitize ? SanitizePathString(s) : s;
  };

  ss << "#### Paths (" << (sanitize ? "Sanitized" : "Raw") << ")\n";
  ss << "- **Executable Root**: " << sanitize_fn(exe) << "\n";
  ss << "- **Profile Root**: " << sanitize_fn(layout.root) << "\n";
  ss << "- **Assets Root**: " << sanitize_fn(layout.assets) << "\n";
  ss << "- **Disc 1 Root**: " << sanitize_fn(layout.assets / "disc1") << "\n";
  ss << "- **Disc 2 Root**: " << sanitize_fn(layout.assets / "disc2") << "\n";
  ss << "- **DLC Root**: " << sanitize_fn(layout.assets / "dlc") << "\n";
  ss << "- **Saves Root**: " << sanitize_fn(layout.saves) << "\n";
  ss << "- **Shaders Root**: " << sanitize_fn(layout.shaders) << "\n";
  ss << "- **Cache Root**: " << sanitize_fn(layout.cache) << "\n";
  ss << "- **Logs Root**: " << sanitize_fn(layout.logs) << "\n\n";

  ss << "#### Diagnostics\n";
  ss << "- **Trace Recorder**: " << (iu::trace::IsRecording() ? "Active (Recording)" : "Inactive (OFF)") << "\n";
  ss << "- **Safe Step**: Ready (Forward 300 units)\n";
  ss << "- **Undo Move**: " << (iu::recovery_move::CanUndo() ? "Ready (Undo available)" : "Idle (No moves to undo)") << "\n";
  ss << "- **Save Anywhere**: " << (iu::save_anywhere::IsActive() ? "Active (Dialog open)" : "Ready") << "\n";
  ss << "- **PSO Prewarm**: " << (REXCVAR_GET(pso_prewarm) ? "Enabled" : "Disabled") << "\n";
  ss << "- **PSO Telemetry**: " << (REXCVAR_GET(pso_telemetry) ? "Enabled" : "Disabled") << "\n";

  return ss.str();
}

inline std::string SaveDiagnosticReportToFile() {
  auto exe = rex::filesystem::GetExecutableFolder();
  auto profile = iu::portable::GetActiveProfile();
  if (profile.empty()) profile = "USA";
  auto layout = iu::portable::Layout(exe, profile);
  std::filesystem::create_directories(layout.logs);

  SYSTEMTIME st;
  GetLocalTime(&st);
  char filename[128];
  std::snprintf(filename, sizeof(filename), "IU_Diagnostic_%04d-%02d-%02d_%02d-%02d-%02d.txt",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

  auto file_path = layout.logs / filename;
  std::string report = BuildDiagnosticReportText(true);

  std::ofstream out(file_path);
  if (out) {
    out << report;
    out.close();
    return file_path.string();
  }
  return {};
}

class CommunityDebugDialog : public rex::ui::ImGuiDialog {
 public:
  using CloseCallback = std::function<void()>;

  explicit CommunityDebugDialog(rex::ui::ImGuiDrawer* drawer, CommunityDebugActions& actions, CloseCallback on_close = {})
      : rex::ui::ImGuiDialog(drawer), actions_(actions), on_close_(std::move(on_close)) {}
  ~CommunityDebugDialog() override = default;

 protected:
  void OnDraw(ImGuiIO& io) override {
    const bool is_es = iu::portable::Spanish();

    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
    bool open = true;
    const char* window_title = is_es ? "Infinite Undiscovery — Menú de Juego###IUGameMenu"
                                     : "Infinite Undiscovery — Game Menu###IUGameMenu";
    if (ImGui::Begin(window_title, &open, ImGuiWindowFlags_None)) {
      // Header Section
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[MENÚ DE JUEGO]" : "[GAME MENU]");
      ImGui::SameLine();
      ImGui::TextDisabled("v%s", iu::kProjectVersion);

      auto profile = iu::portable::GetActiveProfile();
      if (profile.empty()) profile = "USA";
      const uint32_t current_disc = iu::disc_swap::GetCurrentDisc();
      const uint32_t dlc_count = iu::dlc::GetInstalledCount();

      ImGui::TextColored(ImVec4(0.85f, 0.90f, 1.0f, 1.0f),
                         is_es ? "Perfil: %s | Disco: %u | DLC: %u"
                               : "Profile: %s | Disc: %u | DLC: %u",
                         profile.c_str(), current_disc, dlc_count);

      // Language Switcher inline
      ImGui::Text(is_es ? "Idioma:" : "Language:");
      ImGui::SameLine();
      if (!is_es) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
      }
      if (ImGui::SmallButton("English")) {
        if (is_es) {
          iu::portable::SetSpanish(false);
          iu::portable::SaveLanguage();
        }
      }
      if (!is_es) {
        ImGui::PopStyleColor();
      }
      ImGui::SameLine();
      if (is_es) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
      }
      if (ImGui::SmallButton("Español")) {
        if (!is_es) {
          iu::portable::SetSpanish(true);
          iu::portable::SaveLanguage();
        }
      }
      if (is_es) {
        ImGui::PopStyleColor();
      }

      ImGui::Spacing();
      if (ImGui::Button(is_es ? "Información de Sesión / Sistema" : "Session / System Info", ImVec2(-1, 0))) {
        show_system_info_ = true;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(is_es ? "Muestra información detallada del sistema, runtime y diagnósticos"
                                : "Display detailed system, runtime and diagnostic details");
      }

      ImGui::Separator();
      ImGui::Spacing();

      // Compact Operation Feedback Display
      if (!actions_.last_feedback.empty()) {
        const uint64_t now = GetTickCount64();
        if (now - actions_.feedback_time_ms < 10000) { // Show for 10 seconds
          ImGui::PushStyleColor(ImGuiCol_Text, actions_.feedback_success ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
          ImGui::TextWrapped("%s", actions_.last_feedback.c_str());
          ImGui::PopStyleColor();
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
        }
      }

      // SECTION 1: Game / Juego
      ImGui::Text(is_es ? "Juego:" : "Game:");
      ImGui::Spacing();
      if (ImGui::Button(ActionLabel(is_es ? "Volver al Gestor de Perfiles" : "Return to Profile Manager", "bind_iu_profile_manager").c_str(), ImVec2(-1, 0))) {
        actions_.RelaunchProfileManager();
        open = false;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(is_es ? "Muestra confirmación para cerrar la sesión actual y abrir el Gestor de Perfiles (F10)"
                                : "Prompt confirmation to exit current session and open Content Profile Manager (F10)");
      }
      if (ImGui::Button(ActionLabel(is_es ? "Salir del Juego" : "Quit Game", "bind_iu_quit_game").c_str(), ImVec2(-1, 0))) {
        actions_.QuitGame();
        open = false;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(is_es ? "Muestra confirmación para salir de forma segura de Infinite Undiscovery (F12)"
                                : "Prompt confirmation to safely exit Infinite Undiscovery (F12)");
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // SECTION 2: Recovery Tools / Herramientas de Recuperación
      ImGui::Text(is_es ? "Herramientas de Recuperación:" : "Recovery Tools:");
      ImGui::Spacing();

      // Save Anywhere
      const bool sa_active = iu::save_anywhere::IsActive();
      if (sa_active) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                           is_es ? "Diálogo de guardado activo: Handle 0x%08X" : "Save Dialog Active: Handle 0x%08X",
                           iu::save_anywhere::GetDialogHandle());
        ImGui::BeginDisabled(true);
        if (ImGui::Button(ActionLabel(is_es ? "Guardar en Cualquier Lugar [Activo]" : "Save Anywhere [Active]", "bind_iu_save_anywhere").c_str(), ImVec2(-1, 0))) {
          // Double-activation prevented
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
          ImGui::SetTooltip(is_es ? "Ya hay un diálogo de guardado activo. Complétalo o cancélalo primero."
                                  : "A Save Anywhere dialog is already active. Complete or cancel it first.");
        }
        if (ImGui::SmallButton(is_es ? "Resetear Guardado (Limpieza segura)" : "Reset Save Anywhere (Safe Cleanup)")) {
          iu::save_anywhere::ResetActive();
          SetFeedback(true, is_es ? "Guardar en Cualquier Lugar: Limpieza solicitada (modal e input restaurados)"
                                  : "Save Anywhere: Cleanup requested (modal/input safely restored)");
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(is_es ? "Ejecuta la limpieza segura del estado modal y restaura el control del personaje"
                                  : "Executes authoritative modal state cleanup and restores player actor input");
        }
      } else {
        if (ImGui::Button(ActionLabel(is_es ? "Guardar en Cualquier Lugar" : "Save Anywhere", "bind_iu_save_anywhere").c_str(), ImVec2(-1, 0))) {
          if (actions_.SaveAnywhere()) open = false;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(is_es ? "Abre directamente el menú nativo de guardado (sub_82475670)"
                                  : "Directly trigger native Save UI dialog (sub_82475670)");
        }
      }

      ImGui::Spacing();

      const bool move_blocked = sa_active;
      if (move_blocked) {
        ImGui::BeginDisabled(true);
      }
      if (ImGui::Button(ActionLabel(is_es ? "Paso Adelante Seguro (300 unidades)" : "Safe Step Forward (300 units)", "bind_iu_safe_step").c_str(), ImVec2(-1, 0))) {
        actions_.SafeStep();
      }
      if (move_blocked) {
        ImGui::EndDisabled();
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip(move_blocked ? (is_es ? "Movimiento rechazado: el diálogo de guardado o modal está activo"
                                                : "Movement rejected: Save Anywhere or native modal dialog is active")
                                       : (is_es ? "Avanza el personaje 300 unidades en su dirección frontal (rescate de colisión)"
                                                : "Safely step character forward 300 units along facing direction (Geometry/Collision rescue)"));
      }

      const bool can_undo = iu::recovery_move::CanUndo();
      const bool undo_disabled = !can_undo || move_blocked;
      if (undo_disabled) {
        ImGui::BeginDisabled(true);
      }
      if (ImGui::Button(ActionLabel(is_es ? "Deshacer Movimiento de Depuración" : "Undo Debug Move", "bind_iu_undo_move").c_str(), ImVec2(-1, 0))) {
        actions_.Undo();
      }
      if (undo_disabled) {
        ImGui::EndDisabled();
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (move_blocked) {
          ImGui::SetTooltip(is_es ? "Deshacer rechazado: el diálogo de guardado o modal está activo"
                                  : "Undo rejected: Save Anywhere or native modal dialog is active");
        } else if (!can_undo) {
          ImGui::SetTooltip(is_es ? "Deshacer: No hay movimientos previos en la escena actual"
                                  : "Undo: No prior debug movement in current scene/context");
        } else {
          ImGui::SetTooltip(is_es ? "Restaura la posición del personaje a las coordenadas previas al paso"
                                  : "Revert character position to pre-step coordinates");
        }
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // SECTION 3: Developer / Debug
      ImGui::Text(is_es ? "Desarrollador / Depuración:" : "Developer / Debug:");
      ImGui::Spacing();

      const bool is_recording = iu::trace::IsRecording();
      if (is_recording) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), is_es ? "Grabación activa:" : "Recording Active:");
        ImGui::TextWrapped("%s", iu::trace::GetCurrentSessionFilename());
        const double dur = iu::trace::GetSessionDurationSeconds();
        const uint64_t drops = iu::trace::GetDroppedCount();
        ImGui::TextDisabled(is_es ? "Duración: %.1f s | Descartes: %llu" : "Duration: %.1f s | Drops: %llu", dur, drops);
        ImGui::Spacing();

        if (ImGui::Button(is_es ? "Detener Traza" : "Stop Trace", ImVec2(180, 0))) {
          iu::trace::StopRecording();
          SetFeedback(true, is_es ? "Grabador de traza: Detenido" : "Trace Recorder: Stopped");
        }
        ImGui::SameLine();
        if (ImGui::Button(is_es ? "Añadir Marcador" : "Add Marker", ImVec2(180, 0))) {
          iu::trace::AddMarker(nullptr);
          SetFeedback(true, is_es ? "Grabador de traza: Marcador añadido" : "Trace Recorder: Marker added");
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(is_es ? "Inserta una línea [MARKER] en el log de traza activo"
                                  : "Emits a prominent [MARKER] line into the active trace log");
        }
      } else {
        ImGui::TextDisabled(is_es ? "Estado del grabador: Inactivo (OFF)" : "Recorder Status: Inactive (OFF)");
        if (ImGui::Button(is_es ? "Iniciar Traza (Todas las categorías)" : "Start Trace (All Categories)", ImVec2(-1, 0))) {
          iu::trace::StartRecording(iu::trace::kAll);
          SetFeedback(true, is_es ? "Grabador de traza: Iniciado (Todas las categorías)"
                                  : "Trace Recorder: Started (All categories)");
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(is_es ? "Crea un nuevo log de traza con marca de tiempo para todas las categorías"
                                  : "Creates a new timestamped trace log and enables tracing for all categories");
        }
        if (ImGui::Button(is_es ? "Iniciar Traza (Solo SAVEPOINT)" : "Start Trace (SAVEPOINT Only)", ImVec2(-1, 0))) {
          iu::trace::StartRecording(iu::trace::kSavePoint);
          SetFeedback(true, is_es ? "Grabador de traza: Iniciado (Solo SAVEPOINT)"
                                  : "Trace Recorder: Started (SAVEPOINT only)");
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(is_es ? "Crea un nuevo log de traza y registra únicamente operaciones SavePoint"
                                  : "Creates a new timestamped trace log and enables tracing only for SavePoint operations");
        }
      }

#if defined(IU_DEVELOPER_PREVIEWS)
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::TextDisabled("Developer Placeholders (Non-functional):");
      ImGui::Spacing();

      ImGui::BeginDisabled(true);
      if (ImGui::Button("Step Up", ImVec2(-1, 0))) {}
      if (ImGui::Button("Vesplume Override", ImVec2(-1, 0))) {}
      ImGui::EndDisabled();
#endif

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // SECTION 4: Close
      if (ImGui::Button(is_es ? "Cerrar (F5)" : "Close (F5)", ImVec2(-1, 0))) {
        open = false;
      }
    }
    ImGui::End();

    // Session / System Info Modal Window
    if (show_system_info_) {
      DrawSystemInfoModal(io, is_es);
    }

    if (!open && on_close_) {
      on_close_();
    }
  }

 private:
  void DrawSystemInfoModal(ImGuiIO& io, bool is_es) {
    ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
    const char* title = is_es ? "Información de Sesión / Sistema###IUSystemInfo"
                              : "Session / System Info###IUSystemInfo";
    if (ImGui::Begin(title, &show_system_info_, ImGuiWindowFlags_None)) {
      auto exe = rex::filesystem::GetExecutableFolder();
      auto profile = iu::portable::GetActiveProfile();
      if (profile.empty()) profile = "USA";
      auto layout = iu::portable::Layout(exe, profile);
      const auto* pinfo = iu::FindProfileByFolder(profile);
      std::string display_name = pinfo ? iu::portable::Utf8(pinfo->display_name) : profile;
      uint32_t current_disc = iu::disc_swap::GetCurrentDisc();
      uint32_t dlc_count = iu::dlc::GetInstalledCount();
      bool d1_present = std::filesystem::is_directory(layout.assets / "disc1");
      bool d2_present = std::filesystem::is_directory(layout.assets / "disc2");
      std::string mount = (current_disc == 2) ? "\\Device\\IUDisc2" : "\\Device\\Harddisk0\\Partition1";

      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Juego]" : "[Game]");
      ImGui::BulletText("%s: %s", is_es ? "Perfil activo" : "Active Profile", profile.c_str());
      ImGui::BulletText("%s: %s", is_es ? "Nombre visible" : "Display Name", display_name.c_str());
      ImGui::BulletText("%s: Disco %u", is_es ? "Disco actual" : "Current Disc", current_disc);
      ImGui::BulletText("Title ID: 0x535107DB");
      ImGui::BulletText("%s: %s", is_es ? "Región / Edición" : "Region / Edition", pinfo ? pinfo->edition_code.c_str() : profile.c_str());
      ImGui::BulletText("%s: %s", is_es ? "Idioma interfaz" : "UI Language", is_es ? "Español (es)" : "English (en)");
      ImGui::BulletText("%s: %u", is_es ? "Paquetes DLC" : "DLC Packages", dlc_count);

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Compilación]" : "[Build]");
      ImGui::BulletText("%s: %s", is_es ? "Versión" : "Version", iu::kProjectVersion);
      ImGui::BulletText("%s: %s", is_es ? "Tipo de compilación" : "Build Type", GetBuildTypeString().c_str());
      ImGui::BulletText("%s: %s %s", is_es ? "Fecha/Hora" : "Build Date/Time", __DATE__, __TIME__);
      ImGui::BulletText("%s: %s", is_es ? "Compilador" : "Compiler", GetCompilerString().c_str());

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Ejecución]" : "[Runtime]");
      ImGui::BulletText("ReXGlue SDK: 0.10.0");
      ImGui::BulletText("%s: %s", is_es ? "Punto de montaje de disco" : "Disc Mount", mount.c_str());
      ImGui::BulletText("Disco 1: %s", d1_present ? (is_es ? "Presente (Listo)" : "Present (Ready)") : (is_es ? "Ausente" : "Missing"));
      ImGui::BulletText("Disco 2: %s", d2_present ? (is_es ? "Presente (Listo)" : "Present (Ready)") : (is_es ? "No instalado" : "Not installed"));

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Gráficos]" : "[Graphics]");
      ImGui::BulletText("Backend: D3D12");
      ImGui::BulletText("%s: %s", is_es ? "Adaptador GPU" : "GPU Adapter", GetGpuAdapterName().c_str());
      ImGui::BulletText("%s: %dx%d", is_es ? "Resolución ventana" : "Window Resolution",
                        static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Sistema]" : "[System]");
      ImGui::BulletText("OS: %s", GetOsVersionString().c_str());
      ImGui::BulletText("CPU: %s", GetCpuInfoString().c_str());
      ImGui::BulletText("%s: %s", is_es ? "Memoria" : "Memory", GetMemoryString().c_str());

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Rutas Sanitizadas]" : "[Sanitized Paths]");
      ImGui::BulletText("Root: %s", SanitizePathString(layout.root.string()).c_str());
      ImGui::BulletText("Assets: %s", SanitizePathString(layout.assets.string()).c_str());
      ImGui::BulletText("Saves: %s", SanitizePathString(layout.saves.string()).c_str());
      ImGui::BulletText("Shaders: %s", SanitizePathString(layout.shaders.string()).c_str());
      ImGui::BulletText("Logs: %s", SanitizePathString(layout.logs.string()).c_str());

      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), is_es ? "[Diagnósticos]" : "[Diagnostics]");
      ImGui::BulletText("%s: %s", is_es ? "Grabador de trazas" : "Trace Recorder",
                        iu::trace::IsRecording() ? (is_es ? "Activo (Grabando)" : "Active (Recording)") : (is_es ? "Inactivo (OFF)" : "Inactive (OFF)"));
      ImGui::BulletText("%s: %s", is_es ? "Guardar en Cualquier Lugar" : "Save Anywhere",
                        iu::save_anywhere::IsActive() ? (is_es ? "Activo (Diálogo abierto)" : "Active (Dialog open)") : (is_es ? "Listo" : "Ready"));
      ImGui::BulletText("%s: %s", is_es ? "Paso seguro (300 unidades)" : "Safe Step (300 units)", is_es ? "Listo" : "Ready");
      ImGui::BulletText("%s: %s", is_es ? "Deshacer movimiento" : "Undo Move",
                        iu::recovery_move::CanUndo() ? (is_es ? "Disponible" : "Ready") : (is_es ? "Sin historial" : "Idle"));

      ImGui::Separator();
      ImGui::Spacing();

      if (!report_feedback_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
        ImGui::TextWrapped("%s", report_feedback_.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
      }

      if (ImGui::Button(is_es ? "Copiar Todo al Portapapeles" : "Copy All to Clipboard", ImVec2(240, 0))) {
        std::string report = BuildDiagnosticReportText(true);
        ImGui::SetClipboardText(report.c_str());
        report_feedback_ = is_es ? "¡Reporte copiado al portapapeles con éxito!"
                                 : "Diagnostic report copied to clipboard successfully!";
      }
      ImGui::SameLine();
      if (ImGui::Button(is_es ? "Guardar Reporte de Diagnóstico" : "Save Diagnostic Report", ImVec2(240, 0))) {
        std::string path = SaveDiagnosticReportToFile();
        if (!path.empty()) {
          report_feedback_ = (is_es ? "Reporte guardado en:\n" : "Diagnostic report saved to:\n") + SanitizePathString(path);
        } else {
          report_feedback_ = is_es ? "Error al guardar el reporte" : "Failed to save diagnostic report";
        }
      }

      ImGui::Spacing();
      if (ImGui::Button(is_es ? "Cerrar" : "Close", ImVec2(-1, 0))) {
        show_system_info_ = false;
        report_feedback_.clear();
      }
    }
    ImGui::End();
  }

  void SetFeedback(bool success, const std::string& msg) {
    actions_.SetFeedback(success, msg);
  }

  CommunityDebugActions& actions_;
  CloseCallback on_close_;
  bool show_system_info_{false};
  std::string report_feedback_;
};

}  // namespace iu::ui

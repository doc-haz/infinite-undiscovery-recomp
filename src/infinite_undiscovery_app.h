// infinite_undiscovery - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/filesystem.h>
#if REX_HAS_D3D12
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif
#include "vblank_diag.h"
#include "pso_diag.h"
#include "asset_setup.h"
#include "asset_dlc.h"
#include "portable_setup.h"
#include "version.h"
#include <rex/ui/keybinds.h>
#include "community_debug_dialog.h"
#include "disc_swap.h"
#include "disc2_io_trace.h"
#include "disc2_package_trace.h"

REXCVAR_DECLARE(bool, asset_setup);
REXCVAR_DECLARE(bool, profile_manager);
REXCVAR_DECLARE(bool, community_debug);
REXCVAR_DECLARE(bool, pso_prewarm);
REXCVAR_DECLARE(bool, pso_telemetry);

class InfiniteUndiscoveryApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<InfiniteUndiscoveryApp>(new InfiniteUndiscoveryApp(ctx, "infinite_undiscovery",
        PPCImageConfig));
  }

  iu::ui::CommunityDebugActions community_debug_actions_;
  std::unique_ptr<iu::ui::CommunityDebugDialog> community_debug_overlay_;

  void ToggleCommunityDebug() {
    if (!REXCVAR_GET(community_debug)) {
      if (community_debug_overlay_) {
        community_debug_overlay_.reset();
      }
      return;
    }

    if (community_debug_overlay_) {
      community_debug_overlay_.reset();
    } else if (imgui_drawer()) {
      community_debug_overlay_ = std::make_unique<iu::ui::CommunityDebugDialog>(
          imgui_drawer(), community_debug_actions_, [this] {
            app_context().CallInUIThreadDeferred([this] {
              community_debug_overlay_.reset();
            });
          });
    }
  }

  void OnPostInitLogging() override {
#if REX_HAS_D3D12
    // Preserve adapter choices from CLI, environment and configuration.
    if (rex::cvar::GetFlagSource("d3d12_adapter") !=
        rex::cvar::Source::kDefault) {
      return;
    }
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
      return;
    }
    Microsoft::WRL::ComPtr<IDXGIAdapter1> preferred;
    if (FAILED(factory->EnumAdapterByGpuPreference(
            0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&preferred)))) {
      return;
    }
    DXGI_ADAPTER_DESC1 preferred_desc{};
    if (FAILED(preferred->GetDesc1(&preferred_desc)) ||
        (preferred_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
      return;
    }
    // ReXGlue takes the normal DXGI index, not the preference-order index.
    for (UINT index = 0; ; ++index) {
      Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
      if (FAILED(factory->EnumAdapters1(index, &adapter))) {
        return;
      }
      DXGI_ADAPTER_DESC1 desc{};
      if (FAILED(adapter->GetDesc1(&desc))) {
        continue;
      }
      if (desc.AdapterLuid.HighPart == preferred_desc.AdapterLuid.HighPart &&
          desc.AdapterLuid.LowPart == preferred_desc.AdapterLuid.LowPart) {
        rex::cvar::SetFlagByName("d3d12_adapter", std::to_string(index));
        return;
      }
    }
#endif
  }


  // Override virtual hooks for customization:
  void OnPreSetup(rex::RuntimeConfig& config) override {
    config.gpu_plugin = "xenos";
    vb::log("PRE_SETUP gpu_plugin=xenos synthetic_ticker=absent");
    pso_diag::pre_setup();
  }
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  void OnPostSetup() override {
    vb::host_runtime(runtime(), "POST_SETUP");
    pso_diag::install(runtime());

    const std::string profile_name = portable_root_.filename().string();
    const auto layout = iu::portable::Layout(rex::filesystem::GetExecutableFolder(), profile_name);
    REXLOG_INFO("CONTENT_PROFILE={}", profile_name);
    REXLOG_INFO("DISC1_ROOT={}", (layout.assets / "disc1").string());
    REXLOG_INFO("DISC2_ROOT={}", (layout.assets / "disc2").string());
    REXLOG_INFO("DLC_ROOT={}", (layout.assets / "dlc").string());
    REXLOG_INFO("CACHE_ROOT={}", layout.shaders.string());
    iu::trace::emit(iu::trace::kGeneral, "CONTENT_PROFILE=%s", profile_name.c_str());
    iu::trace::emit(iu::trace::kGeneral, "DISC1_ROOT=%s", (layout.assets / "disc1").string().c_str());
    iu::trace::emit(iu::trace::kGeneral, "DISC2_ROOT=%s", (layout.assets / "disc2").string().c_str());
    iu::trace::emit(iu::trace::kGeneral, "DLC_ROOT=%s", (layout.assets / "dlc").string().c_str());
    iu::trace::emit(iu::trace::kGeneral, "CACHE_ROOT=%s", layout.shaders.string().c_str());

    // Base ConstructRuntime calls OnPostSetup before LaunchModule. Preserve
    // Disc 1 and prepare the optional permanent Disc 2 device before the guest.
    if (!iu::disc_swap::Install(runtime(), portable_root_))
      throw std::runtime_error("IU permanent disc mounts could not be prepared");
    iu::disc2_io::Install(runtime());
  }

  bool pending_relaunch_profile_manager_{false};

  void PromptReturnToProfileManager() {
    int choice = iu::portable::Dialog(
        iu::portable::Text(L"Gestor de perfiles de contenido"),
        iu::portable::Text(L"¿Volver al Gestor de Perfiles? Se cerrara la sesion de juego actual."),
        {{IDYES, iu::portable::Text(L"Si")}, {IDNO, iu::portable::Text(L"No")}});
    if (choice == IDYES) {
      pending_relaunch_profile_manager_ = true;
      if (window()) {
        window()->RequestClose();
      } else {
        app_context().RequestDeferredQuit();
      }
    }
  }

  void PromptQuitGame() {
    int choice = iu::portable::Dialog(
        L"Infinite Undiscovery",
        iu::portable::Text(L"¿Salir de Infinite Undiscovery?"),
        {{IDYES, iu::portable::Text(L"Si")}, {IDNO, iu::portable::Text(L"No")}});
    if (choice == IDYES) {
      pending_relaunch_profile_manager_ = false;
      if (window()) {
        window()->RequestClose();
      } else {
        app_context().RequestDeferredQuit();
      }
    }
  }

  void OnClosing(rex::ui::UIEvent& e) override {
    (void)e;
    REXLOG_INFO("[Shutdown] Window closing, terminating title...");
    if (runtime() && runtime()->kernel_state()) {
      runtime()->kernel_state()->TerminateTitle();
    }
    REXLOG_INFO("[Shutdown] Title terminated; flushing logs.");
    rex::FlushLogging();

    if (pending_relaunch_profile_manager_) {
      wchar_t exe_path[MAX_PATH]{};
      GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
      std::wstring cmd = L"\"" + std::wstring(exe_path) + L"\" --profile_manager";
      STARTUPINFOW si{sizeof(si)};
      PROCESS_INFORMATION pi{};
      if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
      } else {
        REXLOG_ERROR("[ProfileManager] Failed to spawn profile manager process");
      }
    }

    std::_Exit(0);
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    pso_diag::install_presenter(runtime());
    community_debug_actions_.on_relaunch_profile_manager = [this] {
      PromptReturnToProfileManager();
    };
    community_debug_actions_.on_quit_game = [this] {
      PromptQuitGame();
    };
    rex::ui::RegisterBind("bind_iu_community_debug", "F5",
                          "Toggle Infinite Undiscovery Game Menu overlay",
                          [this] { ToggleCommunityDebug(); });
    // F7 belongs to ReXGlue Achievements. These defaults are otherwise unused.
    rex::ui::RegisterBind("bind_iu_save_anywhere", "F6", "Save Anywhere",
                          [this] {
                            if (!REXCVAR_GET(community_debug)) return;
                            if (community_debug_actions_.SaveAnywhere())
                              community_debug_overlay_.reset();
                          });
    rex::ui::RegisterBind("bind_iu_safe_step", "F8", "Safe Step Forward (300 units)",
                          [this] {
                            if (REXCVAR_GET(community_debug)) community_debug_actions_.SafeStep();
                          });
    rex::ui::RegisterBind("bind_iu_undo_move", "F9", "Undo Debug Move",
                          [this] {
                            if (REXCVAR_GET(community_debug)) community_debug_actions_.Undo();
                          });
    rex::ui::RegisterBind("bind_iu_profile_manager", "F10", "Return to Profile Manager",
                          [this] { PromptReturnToProfileManager(); });
    rex::ui::RegisterBind("bind_iu_quit_game", "F12", "Quit Game",
                          [this] { PromptQuitGame(); });
  }
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  void OnShutdown() override {
    iu::disc2_io::Shutdown();
    rex::ui::UnregisterBind("bind_iu_community_debug");
    rex::ui::UnregisterBind("bind_iu_save_anywhere");
    rex::ui::UnregisterBind("bind_iu_safe_step");
    rex::ui::UnregisterBind("bind_iu_undo_move");
    rex::ui::UnregisterBind("bind_iu_profile_manager");
    rex::ui::UnregisterBind("bind_iu_quit_game");
    community_debug_overlay_.reset();
    pso_diag::shutdown();
  }
  std::filesystem::path portable_root_;
  bool SetupEnvironment() override {
    try {
      const auto exe=rex::filesystem::GetExecutableFolder();
      iu::portable::Initialize(exe);
      const bool maintenance = REXCVAR_GET(asset_setup) || REXCVAR_GET(profile_manager);
      auto selected=iu::portable::Select(exe, maintenance);
      if(!selected) return false;
      portable_root_=*selected;
      const std::string profile_name = portable_root_.filename().string();
      if (!maintenance && profile_name == "USA-UNDUB" && !iu::portable::IsUndubSubtitleWarningDismissed()) {
        int choice = iu::portable::Dialog(
            L"USA UNDUB",
            iu::portable::Text(L"USA UNDUB utiliza voces en japones.\n\nPara una mejor experiencia, activa:\nOpciones -> Mensajes de evento -> Voz y subtitulos."),
            {{IDOK, iu::portable::Text(L"Aceptar")}, {101, iu::portable::Text(L"No volver a mostrar")}});
        if (choice == 101) {
          iu::portable::SetUndubSubtitleWarningDismissed(true);
          iu::portable::SaveLanguage();
        }
      }
      const auto layout=iu::portable::Layout(exe,profile_name);
      // Pin before base SetupEnvironment: no user-profile default or legacy config.
      rex::cvar::SetFlagByName("user_data_root",layout.saves.string());
      rex::cvar::SetFlagByName("game_data_root",(layout.assets/"disc1").string());
      rex::cvar::SetFlagByName("cache_root",layout.shaders.string());
      rex::cvar::SetFlagByName("log_file",(layout.logs/"runtime.log").string());
      // Existing diagnostics remain unchanged; optional output is local too.
      if(std::getenv("IU_DIAG_TRACE_PATH")) _putenv_s("IU_DIAG_TRACE_PATH",(layout.logs/"diagnostic.log").string().c_str());
      if(std::getenv("IU_PERF_TRACE_PATH")) _putenv_s("IU_PERF_TRACE_PATH",(layout.logs/"performance.log").string().c_str());
      if(std::getenv("IU_VESPLUME_TRACE_PATH")) _putenv_s("IU_VESPLUME_TRACE_PATH",(layout.logs/"vesplume_trace.log").string().c_str());
      _putenv_s("IU_TRACE_DIR",layout.logs.string().c_str());

      const auto disc1_root = layout.assets / "disc1";
      const auto disc2_root = layout.assets / "disc2";
      const auto dlc_root = layout.assets / "dlc";
      const auto cache_root = layout.shaders;

      std::printf("[STARTUP] CONTENT_PROFILE=%s\n", profile_name.c_str());
      std::printf("[STARTUP] DISC1_ROOT=%s\n", disc1_root.string().c_str());
      std::printf("[STARTUP] DISC2_ROOT=%s\n", disc2_root.string().c_str());
      std::printf("[STARTUP] DLC_ROOT=%s\n", dlc_root.string().c_str());
      std::printf("[STARTUP] CACHE_ROOT=%s\n", cache_root.string().c_str());
      std::fflush(stdout);

      REXLOG_INFO("CONTENT_PROFILE={}", profile_name);
      REXLOG_INFO("DISC1_ROOT={}", disc1_root.string());
      REXLOG_INFO("DISC2_ROOT={}", disc2_root.string());
      REXLOG_INFO("DLC_ROOT={}", dlc_root.string());
      REXLOG_INFO("CACHE_ROOT={}", cache_root.string());
      vb::log("CONTENT_PROFILE=%s DISC1_ROOT=%s DISC2_ROOT=%s DLC_ROOT=%s CACHE_ROOT=%s",
              profile_name.c_str(), disc1_root.string().c_str(), disc2_root.string().c_str(),
              dlc_root.string().c_str(), cache_root.string().c_str());

      return rex::ReXApp::SetupEnvironment();
    } catch(const std::exception& e) {iu::portable::Error(e.what());return false;}
  }
  void OnConfigurePaths(rex::PathConfig& paths) override {
    paths.game_data_root=portable_root_/"assets"/"disc1";
    paths.user_data_root=portable_root_/"saves";
    paths.update_data_root=portable_root_/"assets"/"updates";
    paths.cache_root=portable_root_/"shaders";
    paths.metadata_root=portable_root_/"cache"/"metadata";
    paths.config_path=portable_root_/"cache"/"runtime.toml";
  }
  std::optional<rex::PathConfig> OnFinalizePaths(const rex::PathConfig& defaults,
      std::function<void(rex::PathConfig)>) override {return defaults;}

  bool ConstructRuntime(const rex::PathConfig& paths) override {
    if (!rex::ReXApp::ConstructRuntime(paths)) return false;
    // Original setup/diagnostic hooks run unchanged in the base construction.
    // The XEX and title are now loaded, but no guest thread has been launched.
    const auto packages = paths.game_data_root.parent_path() / "dlc";
    if (paths.game_data_root.filename() != "disc1" || !std::filesystem::is_directory(packages)) return true;
    for (;;) {
      try {
        iu::dlc::InstallPending(packages, runtime()->kernel_state(), paths.user_data_root);
        REXLOG_INFO("[AssetSetup] DLC validated and installed before guest launch");
        return true;
      } catch (const std::exception& error) {
        REXLOG_ERROR("[AssetSetup] DLC installation stopped: {}", error.what());
        int choice=iu::portable::Dialog(L"DLC",iu::portable::Wide(error.what()),
          {{IDABORT,L"Cerrar"},{IDRETRY,L"Reintentar"},{IDIGNORE,L"Continuar sin DLC"}});
        if (choice == IDIGNORE) return true;
        if (choice != IDRETRY) return false;
      }
    }
  }

  void OnPostLaunchModule(rex::system::XThread* thread) override {
    pso_diag::prewarm_wait();
    rex::ReXApp::OnPostLaunchModule(thread);
  }
};

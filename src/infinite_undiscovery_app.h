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

REXCVAR_DECLARE(bool, asset_setup);

class InfiniteUndiscoveryApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<InfiniteUndiscoveryApp>(new InfiniteUndiscoveryApp(ctx, "infinite_undiscovery",
        PPCImageConfig));
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
  }
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    pso_diag::install_presenter(runtime());
  }
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  void OnShutdown() override {
    pso_diag::shutdown();
  }
  std::filesystem::path portable_root_;
  bool SetupEnvironment() override {
    try {
      const auto exe=rex::filesystem::GetExecutableFolder();
      iu::portable::Initialize(exe);
      auto selected=iu::portable::Select(exe, REXCVAR_GET(asset_setup));
      if(!selected) return false;
      portable_root_=*selected;
      const auto layout=iu::portable::Layout(exe,portable_root_.filename().string());
      // Pin before base SetupEnvironment: no user-profile default or legacy config.
      rex::cvar::SetFlagByName("user_data_root",layout.saves.string());
      rex::cvar::SetFlagByName("game_data_root",(layout.assets/"disc1").string());
      rex::cvar::SetFlagByName("cache_root",layout.shaders.string());
      rex::cvar::SetFlagByName("log_file",(layout.logs/"runtime.log").string());
      // Existing diagnostics remain unchanged; optional output is local too.
      if(std::getenv("IU_DIAG_TRACE_PATH")) _putenv_s("IU_DIAG_TRACE_PATH",(layout.logs/"diagnostic.log").string().c_str());
      if(std::getenv("IU_PERF_TRACE_PATH")) _putenv_s("IU_PERF_TRACE_PATH",(layout.logs/"performance.log").string().c_str());
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
};

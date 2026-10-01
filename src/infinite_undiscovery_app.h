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
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      const auto exe_dir = rex::filesystem::GetExecutableFolder();
      const auto local_assets = (exe_dir / "assets").lexically_normal();
      const auto dev_assets = (exe_dir / ".." / ".." / ".." / "assets").lexically_normal();
      if (std::filesystem::is_directory(local_assets)) {
        paths.game_data_root = local_assets;
      } else if (std::filesystem::is_directory(dev_assets)) {
        paths.game_data_root = dev_assets;
      } else {
        paths.game_data_root = local_assets;
      }
    }
  }
};

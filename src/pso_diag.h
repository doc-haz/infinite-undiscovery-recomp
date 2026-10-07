#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <thread>

#include <rex/runtime.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/graphics/graphics_system.h>
#include <rex/ui/presenter.h>
#include <rex/ui/d3d12/d3d12_provider.h>

REXCVAR_DECLARE(bool, pso_prewarm);
REXCVAR_DECLARE(bool, pso_telemetry);

namespace pso_diag {

enum EventType : uint8_t {
  EVENT_NONE = 0,
  EVENT_PSO = 1,
  EVENT_FRAME = 2,
  EVENT_PRESENT_BEGIN = 3,
  EVENT_PRESENT_END = 4,
};

struct TraceEvent {
  uint8_t type;
  uint8_t pad[3];
  uint32_t seq;
  uint32_t thread_id;
  uint32_t hr;
  uint64_t t_begin;
  uint64_t t_end;
  union {
    struct {
      uint64_t vs_hash;
      uint64_t ps_hash;
    } pso;
    struct {
      uint64_t between_frames_ticks;
      uint64_t wrapper_ticks;
      uint64_t pre_present_ticks;
      uint64_t dxgi_present_ticks;
      uint64_t post_present_ticks;
      uint64_t waitable_ticks;
    } frame;
    struct {
      uint32_t sync_interval;
      uint32_t flags;
    } present;
  };
};

static constexpr size_t kEventBufferSize = 262144;
inline TraceEvent s_events[kEventBufferSize];
inline std::atomic<uint64_t> s_write_idx{0};
inline uint64_t s_read_idx{0};

inline LARGE_INTEGER s_qpc_freq{0};
inline LARGE_INTEGER s_trace_start_qpc{0};
inline std::atomic<uint64_t> s_prev_wrapper_end_qpc{0};

inline std::atomic<uint32_t> s_pso_sequence{0};
inline std::atomic<uint32_t> s_frame_sequence{0};
inline std::atomic<uint32_t> s_present_sequence{0};

inline std::atomic<bool> s_running{false};
inline std::thread s_worker_thread;
inline FILE* s_trace_file = nullptr;

inline std::atomic<uint32_t> s_pso_active_compilations{0};
inline std::atomic<uint32_t> s_pso_total_created{0};
inline std::atomic<uint64_t> s_last_pso_end_qpc{0};
inline std::atomic<bool> s_telemetry_enabled{false};

struct FramePsoStats {
  std::atomic<uint32_t> count{0};
  std::atomic<uint64_t> total_ticks{0};
  std::atomic<uint64_t> max_ticks{0};
};
inline FramePsoStats s_frame_pso;

struct SessionPsoStats {
  std::atomic<uint32_t> total_created{0};
  std::atomic<uint64_t> total_ticks{0};
  std::atomic<uint64_t> worst_single_ticks{0};
  std::atomic<uint32_t> worst_frame{0};
  std::atomic<uint64_t> worst_frame_ticks{0};
};
inline SessionPsoStats s_session_pso;

inline bool is_telemetry_enabled() {
  const char* env = std::getenv("IU_PSO_TELEMETRY");
  if (env) {
    return (std::strcmp(env, "1") == 0 || std::strcmp(env, "true") == 0 || std::strcmp(env, "on") == 0);
  }
  const char* path = std::getenv("IU_PERF_TRACE_PATH");
  if (path && path[0] != '\0') {
    return true;
  }
  return REXCVAR_GET(pso_telemetry);
}

inline bool is_prewarm_enabled() {
  const char* env = std::getenv("IU_PSO_PREWARM");
  if (env) {
    return !(std::strcmp(env, "0") == 0 || std::strcmp(env, "false") == 0 || std::strcmp(env, "off") == 0);
  }
  return REXCVAR_GET(pso_prewarm);
}

// Function pointers for original vtable methods
using PFN_CreateGraphicsPipelineState = HRESULT(STDMETHODCALLTYPE*)(
    ID3D12Device* This,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState);

using PFN_CreateSwapChain = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory* This,
    IUnknown* pDevice,
    DXGI_SWAP_CHAIN_DESC* pDesc,
    IDXGISwapChain** ppSwapChain);

using PFN_CreateSwapChainForHwnd = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2* This,
    IUnknown* pDevice,
    HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain);

using PFN_Present = HRESULT(STDMETHODCALLTYPE*)(
    IDXGISwapChain* This,
    UINT SyncInterval,
    UINT Flags);

using PFN_Present1 = HRESULT(STDMETHODCALLTYPE*)(
    IDXGISwapChain1* This,
    UINT SyncInterval,
    UINT Flags,
    const DXGI_PRESENT_PARAMETERS* pPresentParameters);

struct PresenterProbe : rex::ui::Presenter {
  using PaintResult = rex::ui::Presenter::PaintResult;
};

using PFN_PaintAndPresentImpl = PresenterProbe::PaintResult(STDMETHODCALLTYPE*)(
    rex::ui::Presenter* This,
    bool execute_ui_drawers);

using PFN_ResizeBuffers = HRESULT(STDMETHODCALLTYPE*)(
    IDXGISwapChain* This,
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags);

inline PFN_CreateGraphicsPipelineState s_orig_CreateGraphicsPipelineState = nullptr;
inline PFN_CreateSwapChain s_orig_CreateSwapChain = nullptr;
inline PFN_CreateSwapChainForHwnd s_orig_CreateSwapChainForHwnd = nullptr;
inline PFN_Present s_orig_Present = nullptr;
inline PFN_Present1 s_orig_Present1 = nullptr;
inline PFN_PaintAndPresentImpl s_orig_PaintAndPresentImpl = nullptr;
inline PFN_ResizeBuffers s_orig_ResizeBuffers = nullptr;

using PFN_GetProcAddress = FARPROC(WINAPI*)(HMODULE, LPCSTR);
inline PFN_GetProcAddress s_orig_GetProcAddress = nullptr;

using PFN_CreateDXGIFactory = HRESULT(WINAPI*)(REFIID, void**);
inline PFN_CreateDXGIFactory s_orig_CreateDXGIFactory = nullptr;

using PFN_CreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID, void**);
inline PFN_CreateDXGIFactory1 s_orig_CreateDXGIFactory1 = nullptr;

using PFN_CreateDXGIFactory2 = HRESULT(WINAPI*)(UINT, REFIID, void**);
inline PFN_CreateDXGIFactory2 s_orig_CreateDXGIFactory2 = nullptr;

inline std::atomic<HANDLE> s_frame_latency_waitable_object{nullptr};

inline bool IsWaitableExperimentEnabled() {
  const char* env = std::getenv("IU_EXPERIMENT_WAITABLE");
  if (env && (std::strcmp(env, "0") == 0 || std::strcmp(env, "false") == 0 || std::strcmp(env, "off") == 0)) {
    return false;
  }
  return true; // Enabled by default for this experiment
}

inline std::atomic<bool> s_swapchain_hooked{false};
inline thread_local bool s_in_present_hook = false;

struct CurrentFrameTimings {
  uint64_t t_dxgi_begin = 0;
  uint64_t t_dxgi_end = 0;
  uint32_t sync_interval = 0;
  uint32_t flags = 0;
  uint32_t present_hr = 0;
  bool present_called = false;
};
inline thread_local CurrentFrameTimings s_current_frame_timings;

inline void WorkerThreadFunc() {
  while (s_running.load(std::memory_order_relaxed) || s_read_idx < s_write_idx.load(std::memory_order_relaxed)) {
    uint64_t write_idx = s_write_idx.load(std::memory_order_acquire);
    if (s_read_idx < write_idx) {
      double freq = (double)s_qpc_freq.QuadPart;
      uint64_t start_qpc = (uint64_t)s_trace_start_qpc.QuadPart;

      while (s_read_idx < write_idx) {
        const TraceEvent& ev = s_events[s_read_idx % kEventBufferSize];
        if (ev.type == EVENT_PSO) {
          double elapsed_ms = (double)(ev.t_end - ev.t_begin) * 1000.0 / freq;
          double t_begin_ms = (double)(ev.t_begin - start_qpc) * 1000.0 / freq;
          double t_end_ms = (double)(ev.t_end - start_qpc) * 1000.0 / freq;

          std::fprintf(s_trace_file,
                       "[IU-PERF] pso_seq=%u type=PSO vs=%016llX ps=%016llX elapsed_ms=%.3f "
                       "t_begin_ms=%.3f t_end_ms=%.3f hr=%08X tid=%u\n",
                       ev.seq,
                       (unsigned long long)ev.pso.vs_hash,
                       (unsigned long long)ev.pso.ps_hash,
                       elapsed_ms, t_begin_ms, t_end_ms,
                       ev.hr, ev.thread_id);
        } else if (ev.type == EVENT_PRESENT_BEGIN) {
          double t_ms = (double)(ev.t_begin - start_qpc) * 1000.0 / freq;
          std::fprintf(s_trace_file,
                       "[PRESENT_BEGIN] seq=%u t_ms=%.3f tid=%u\n",
                       ev.seq, t_ms, ev.thread_id);
        } else if (ev.type == EVENT_PRESENT_END) {
          double elapsed_ms = (double)(ev.t_end - ev.t_begin) * 1000.0 / freq;
          double t_ms = (double)(ev.t_end - start_qpc) * 1000.0 / freq;
          std::fprintf(s_trace_file,
                       "[PRESENT_END] seq=%u t_ms=%.3f elapsed_ms=%.3f hr=%08X sync=%u flags=%u tid=%u\n",
                       ev.seq, t_ms, elapsed_ms, ev.hr, ev.present.sync_interval, ev.present.flags, ev.thread_id);
        } else if (ev.type == EVENT_FRAME) {
          double total_ms = (double)(ev.frame.between_frames_ticks + ev.frame.waitable_ticks + ev.frame.wrapper_ticks) * 1000.0 / freq;
          double between_ms = (double)ev.frame.between_frames_ticks * 1000.0 / freq;
          double waitable_ms = (double)ev.frame.waitable_ticks * 1000.0 / freq;
          double wrapper_ms = (double)ev.frame.wrapper_ticks * 1000.0 / freq;
          double pre_ms = (double)ev.frame.pre_present_ticks * 1000.0 / freq;
          double dxgi_ms = (double)ev.frame.dxgi_present_ticks * 1000.0 / freq;
          double post_ms = (double)ev.frame.post_present_ticks * 1000.0 / freq;
          double t_begin_ms = (double)(ev.t_begin - start_qpc) * 1000.0 / freq;
          double t_end_ms = (double)(ev.t_end - start_qpc) * 1000.0 / freq;

          std::fprintf(s_trace_file,
                       "[FRAME] frame_seq=%u total_ms=%.3f between_ms=%.3f waitable_ms=%.3f wrapper_ms=%.3f "
                       "pre_present_ms=%.3f dxgi_present_ms=%.3f post_present_ms=%.3f "
                       "t_begin_ms=%.3f t_end_ms=%.3f hr=%08X tid=%u\n",
                       ev.seq, total_ms, between_ms, waitable_ms, wrapper_ms,
                       pre_ms, dxgi_ms, post_ms,
                       t_begin_ms, t_end_ms, ev.hr, ev.thread_id);
        }
        s_read_idx++;
      }
      std::fflush(s_trace_file);
    } else {
      Sleep(2);
    }
  }
  if (s_trace_file) {
    std::fflush(s_trace_file);
  }
}

inline HRESULT STDMETHODCALLTYPE Hook_CreateGraphicsPipelineState(
    ID3D12Device* This,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
    REFIID riid,
    void** ppPipelineState) {
  s_pso_active_compilations.fetch_add(1, std::memory_order_relaxed);

  LARGE_INTEGER t0, t1;
  bool timing_needed = s_telemetry_enabled.load(std::memory_order_relaxed) ||
                       s_running.load(std::memory_order_relaxed);

  if (timing_needed) {
    QueryPerformanceCounter(&t0);
  }

  HRESULT hr = s_orig_CreateGraphicsPipelineState(This, pDesc, riid, ppPipelineState);

  if (timing_needed) {
    QueryPerformanceCounter(&t1);
    s_last_pso_end_qpc.store(static_cast<uint64_t>(t1.QuadPart), std::memory_order_release);
  } else {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    s_last_pso_end_qpc.store(static_cast<uint64_t>(now.QuadPart), std::memory_order_release);
  }

  s_pso_active_compilations.fetch_sub(1, std::memory_order_release);
  s_pso_total_created.fetch_add(1, std::memory_order_relaxed);

  if (timing_needed) {
    uint64_t vs_hash = 0, ps_hash = 0;
    if (pDesc && pDesc->VS.pShaderBytecode && pDesc->VS.BytecodeLength > 0) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(pDesc->VS.pShaderBytecode);
      uint64_t h = 14695981039346656037ULL;
      for (size_t i = 0; i < pDesc->VS.BytecodeLength; ++i) {
        h = (h ^ p[i]) * 1099511628211ULL;
      }
      vs_hash = h;
    }
    if (pDesc && pDesc->PS.pShaderBytecode && pDesc->PS.BytecodeLength > 0) {
      const uint8_t* p = reinterpret_cast<const uint8_t*>(pDesc->PS.pShaderBytecode);
      uint64_t h = 14695981039346656037ULL;
      for (size_t i = 0; i < pDesc->PS.BytecodeLength; ++i) {
        h = (h ^ p[i]) * 1099511628211ULL;
      }
      ps_hash = h;
    }

    uint64_t elapsed_ticks = static_cast<uint64_t>(t1.QuadPart - t0.QuadPart);
    double freq = (double)s_qpc_freq.QuadPart;
    if (freq <= 0.0) freq = 10000000.0;
    double elapsed_ms = (double)elapsed_ticks * 1000.0 / freq;
    uint32_t current_frame = s_frame_sequence.load(std::memory_order_relaxed);

    if (s_telemetry_enabled.load(std::memory_order_relaxed)) {
      REXLOG_INFO("PIPELINE_CREATE frame={} vs={:016X} ps={:016X} time_ms={:.3f}",
                  current_frame, vs_hash, ps_hash, elapsed_ms);
      if (s_trace_file) {
        std::fprintf(s_trace_file, "PIPELINE_CREATE frame=%u vs=%016llX ps=%016llX time_ms=%.3f\n",
                     current_frame, (unsigned long long)vs_hash, (unsigned long long)ps_hash, elapsed_ms);
      }

      s_frame_pso.count.fetch_add(1, std::memory_order_relaxed);
      s_frame_pso.total_ticks.fetch_add(elapsed_ticks, std::memory_order_relaxed);
      uint64_t cur_max = s_frame_pso.max_ticks.load(std::memory_order_relaxed);
      while (elapsed_ticks > cur_max &&
             !s_frame_pso.max_ticks.compare_exchange_weak(cur_max, elapsed_ticks, std::memory_order_relaxed)) {}

      s_session_pso.total_created.fetch_add(1, std::memory_order_relaxed);
      s_session_pso.total_ticks.fetch_add(elapsed_ticks, std::memory_order_relaxed);
      uint64_t cur_worst_single = s_session_pso.worst_single_ticks.load(std::memory_order_relaxed);
      while (elapsed_ticks > cur_worst_single &&
             !s_session_pso.worst_single_ticks.compare_exchange_weak(cur_worst_single, elapsed_ticks, std::memory_order_relaxed)) {}
    }

    if (s_running.load(std::memory_order_relaxed)) {
      uint32_t seq = ++s_pso_sequence;
      uint64_t idx = s_write_idx.fetch_add(1, std::memory_order_relaxed);
      TraceEvent& ev = s_events[idx % kEventBufferSize];
      ev.type = EVENT_PSO;
      ev.seq = seq;
      ev.thread_id = GetCurrentThreadId();
      ev.hr = static_cast<uint32_t>(hr);
      ev.t_begin = static_cast<uint64_t>(t0.QuadPart);
      ev.t_end = static_cast<uint64_t>(t1.QuadPart);
      ev.pso.vs_hash = vs_hash;
      ev.pso.ps_hash = ps_hash;
    }
  }

  return hr;
}

inline HRESULT STDMETHODCALLTYPE Hook_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags) {
  if (s_in_present_hook) {
    return s_orig_Present(This, SyncInterval, Flags);
  }
  s_in_present_hook = true;

  LARGE_INTEGER t0, t1;
  QueryPerformanceCounter(&t0);

  uint32_t seq = ++s_present_sequence;
  uint32_t tid = GetCurrentThreadId();

  if (s_running.load(std::memory_order_relaxed)) {
    uint64_t idx0 = s_write_idx.fetch_add(1, std::memory_order_relaxed);
    TraceEvent& ev0 = s_events[idx0 % kEventBufferSize];
    ev0.type = EVENT_PRESENT_BEGIN;
    ev0.seq = seq;
    ev0.thread_id = tid;
    ev0.hr = 0;
    ev0.t_begin = static_cast<uint64_t>(t0.QuadPart);
    ev0.t_end = static_cast<uint64_t>(t0.QuadPart);
    ev0.present.sync_interval = SyncInterval;
    ev0.present.flags = Flags;
  }

  s_current_frame_timings.t_dxgi_begin = static_cast<uint64_t>(t0.QuadPart);
  s_current_frame_timings.sync_interval = SyncInterval;
  s_current_frame_timings.flags = Flags;

  HRESULT hr = s_orig_Present(This, SyncInterval, Flags);

  QueryPerformanceCounter(&t1);
  s_current_frame_timings.t_dxgi_end = static_cast<uint64_t>(t1.QuadPart);
  s_current_frame_timings.present_hr = static_cast<uint32_t>(hr);
  s_current_frame_timings.present_called = true;

  if (s_running.load(std::memory_order_relaxed)) {
    uint64_t idx1 = s_write_idx.fetch_add(1, std::memory_order_relaxed);
    TraceEvent& ev1 = s_events[idx1 % kEventBufferSize];
    ev1.type = EVENT_PRESENT_END;
    ev1.seq = seq;
    ev1.thread_id = tid;
    ev1.hr = static_cast<uint32_t>(hr);
    ev1.t_begin = static_cast<uint64_t>(t0.QuadPart);
    ev1.t_end = static_cast<uint64_t>(t1.QuadPart);
    ev1.present.sync_interval = SyncInterval;
    ev1.present.flags = Flags;
  }

  s_in_present_hook = false;
  return hr;
}

inline HRESULT STDMETHODCALLTYPE Hook_Present1(
    IDXGISwapChain1* This, UINT SyncInterval, UINT Flags,
    const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
  if (s_in_present_hook) {
    return s_orig_Present1(This, SyncInterval, Flags, pPresentParameters);
  }
  s_in_present_hook = true;

  LARGE_INTEGER t0, t1;
  QueryPerformanceCounter(&t0);

  uint32_t seq = ++s_present_sequence;
  uint32_t tid = GetCurrentThreadId();

  if (s_running.load(std::memory_order_relaxed)) {
    uint64_t idx0 = s_write_idx.fetch_add(1, std::memory_order_relaxed);
    TraceEvent& ev0 = s_events[idx0 % kEventBufferSize];
    ev0.type = EVENT_PRESENT_BEGIN;
    ev0.seq = seq;
    ev0.thread_id = tid;
    ev0.hr = 0;
    ev0.t_begin = static_cast<uint64_t>(t0.QuadPart);
    ev0.t_end = static_cast<uint64_t>(t0.QuadPart);
    ev0.present.sync_interval = SyncInterval;
    ev0.present.flags = Flags;
  }

  s_current_frame_timings.t_dxgi_begin = static_cast<uint64_t>(t0.QuadPart);
  s_current_frame_timings.sync_interval = SyncInterval;
  s_current_frame_timings.flags = Flags;

  HRESULT hr = s_orig_Present1(This, SyncInterval, Flags, pPresentParameters);

  QueryPerformanceCounter(&t1);
  s_current_frame_timings.t_dxgi_end = static_cast<uint64_t>(t1.QuadPart);
  s_current_frame_timings.present_hr = static_cast<uint32_t>(hr);
  s_current_frame_timings.present_called = true;

  if (s_running.load(std::memory_order_relaxed)) {
    uint64_t idx1 = s_write_idx.fetch_add(1, std::memory_order_relaxed);
    TraceEvent& ev1 = s_events[idx1 % kEventBufferSize];
    ev1.type = EVENT_PRESENT_END;
    ev1.seq = seq;
    ev1.thread_id = tid;
    ev1.hr = static_cast<uint32_t>(hr);
    ev1.t_begin = static_cast<uint64_t>(t0.QuadPart);
    ev1.t_end = static_cast<uint64_t>(t1.QuadPart);
    ev1.present.sync_interval = SyncInterval;
    ev1.present.flags = Flags;
  }

  s_in_present_hook = false;
  return hr;
}

inline HRESULT STDMETHODCALLTYPE Hook_ResizeBuffers(
    IDXGISwapChain* This,
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags) {
  if (IsWaitableExperimentEnabled()) {
    SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  }
  // Safeguard: clear handle so no concurrent thread waits on a stale or releasing handle during resize
  s_frame_latency_waitable_object.store(nullptr, std::memory_order_release);

  HRESULT hr = s_orig_ResizeBuffers(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);
  if (SUCCEEDED(hr) && IsWaitableExperimentEnabled()) {
    IDXGISwapChain2* sc2 = nullptr;
    if (SUCCEEDED(This->QueryInterface(IID_PPV_ARGS(&sc2)))) {
      sc2->SetMaximumFrameLatency(1);
      HANDLE hWait = sc2->GetFrameLatencyWaitableObject();
      if (hWait && hWait != INVALID_HANDLE_VALUE) {
        s_frame_latency_waitable_object.store(hWait, std::memory_order_release);
        if (s_trace_file) {
          std::fprintf(s_trace_file,
                       "# [IU-PERF] WAITABLE_OBJECT_RESIZED sc2=%p handle=%p max_latency=1\n",
                       (void*)sc2, (void*)hWait);
          std::fflush(s_trace_file);
        }
      }
      sc2->Release();
    }
  }
  return hr;
}

inline void HookSwapChain(IDXGISwapChain* sc) {
  if (!sc) return;
  if (s_swapchain_hooked.exchange(true)) return;

  void** vtbl = *reinterpret_cast<void***>(sc);
  if (!vtbl) return;

  DWORD old_protect;
  if (VirtualProtect(&vtbl[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
    s_orig_Present = reinterpret_cast<PFN_Present>(vtbl[8]);
    vtbl[8] = reinterpret_cast<void*>(&Hook_Present);
    VirtualProtect(&vtbl[8], sizeof(void*), old_protect, &old_protect);
  }

  if (VirtualProtect(&vtbl[13], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
    s_orig_ResizeBuffers = reinterpret_cast<PFN_ResizeBuffers>(vtbl[13]);
    vtbl[13] = reinterpret_cast<void*>(&Hook_ResizeBuffers);
    VirtualProtect(&vtbl[13], sizeof(void*), old_protect, &old_protect);
  }

  if (VirtualProtect(&vtbl[22], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
    s_orig_Present1 = reinterpret_cast<PFN_Present1>(vtbl[22]);
    vtbl[22] = reinterpret_cast<void*>(&Hook_Present1);
    VirtualProtect(&vtbl[22], sizeof(void*), old_protect, &old_protect);
  }

  if (s_trace_file) {
    std::fprintf(s_trace_file, "# [IU-PERF] SWAPCHAIN_HOOKED sc=%p orig_present=%p orig_present1=%p orig_resize=%p\n",
                 (void*)sc, (void*)s_orig_Present, (void*)s_orig_Present1, (void*)s_orig_ResizeBuffers);
    std::fflush(s_trace_file);
  }

  if (IsWaitableExperimentEnabled() && s_frame_latency_waitable_object.load(std::memory_order_relaxed) == nullptr) {
    IDXGISwapChain2* sc2 = nullptr;
    if (SUCCEEDED(sc->QueryInterface(IID_PPV_ARGS(&sc2)))) {
      sc2->SetMaximumFrameLatency(1);
      HANDLE hWait = sc2->GetFrameLatencyWaitableObject();
      if (hWait && hWait != INVALID_HANDLE_VALUE) {
        s_frame_latency_waitable_object.store(hWait, std::memory_order_release);
        if (s_trace_file) {
          std::fprintf(s_trace_file, "# [IU-PERF] WAITABLE_OBJECT_INITIALIZED_IN_HOOK sc2=%p handle=%p max_latency=1\n",
                       (void*)sc2, (void*)hWait);
          std::fflush(s_trace_file);
        }
      }
      sc2->Release();
    }
  }
}

inline HRESULT STDMETHODCALLTYPE Hook_CreateSwapChain(
    IDXGIFactory* This,
    IUnknown* pDevice,
    DXGI_SWAP_CHAIN_DESC* pDesc,
    IDXGISwapChain** ppSwapChain) {
  HRESULT hr = s_orig_CreateSwapChain(This, pDevice, pDesc, ppSwapChain);
  if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain) {
    HookSwapChain(*ppSwapChain);
  }
  return hr;
}

inline HRESULT STDMETHODCALLTYPE Hook_CreateSwapChainForHwnd(
    IDXGIFactory2* This,
    IUnknown* pDevice,
    HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput,
    IDXGISwapChain1** ppSwapChain) {
  DXGI_SWAP_CHAIN_DESC1 modified_desc{};
  const DXGI_SWAP_CHAIN_DESC1* pDescToUse = pDesc;

  bool waitable_enabled = IsWaitableExperimentEnabled();
  if (pDesc) {
    modified_desc = *pDesc;
    if (waitable_enabled) {
      modified_desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    }
    pDescToUse = &modified_desc;

    if (s_trace_file) {
      std::fprintf(s_trace_file,
                   "# [IU-PERF] SWAPCHAIN_DESC1 w=%u h=%u format=%u buffer_count=%u "
                   "swap_effect=%u orig_flags=0x%X final_flags=0x%X waitable_exp=%u\n",
                   pDesc->Width, pDesc->Height, pDesc->Format, pDesc->BufferCount,
                   pDesc->SwapEffect, pDesc->Flags, modified_desc.Flags, waitable_enabled ? 1 : 0);
      std::fflush(s_trace_file);
    }
  }

  HRESULT hr = s_orig_CreateSwapChainForHwnd(
      This, pDevice, hWnd, pDescToUse, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

  if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain) {
    if (waitable_enabled) {
      IDXGISwapChain2* sc2 = nullptr;
      if (SUCCEEDED((*ppSwapChain)->QueryInterface(IID_PPV_ARGS(&sc2)))) {
        HRESULT hr_lat = sc2->SetMaximumFrameLatency(1);
        HANDLE hWait = sc2->GetFrameLatencyWaitableObject();
        if (hWait && hWait != INVALID_HANDLE_VALUE) {
          s_frame_latency_waitable_object.store(hWait, std::memory_order_release);
          if (s_trace_file) {
            std::fprintf(s_trace_file,
                         "# [IU-PERF] WAITABLE_OBJECT_INITIALIZED sc2=%p hr_lat=%08X handle=%p max_latency=1\n",
                         (void*)sc2, (unsigned int)hr_lat, (void*)hWait);
            std::fflush(s_trace_file);
          }
        }
        sc2->Release();
      }
    }
    HookSwapChain(reinterpret_cast<IDXGISwapChain*>(*ppSwapChain));
  }
  return hr;
}

inline PresenterProbe::PaintResult STDMETHODCALLTYPE Hook_PaintAndPresentImpl(
    rex::ui::Presenter* This,
    bool execute_ui_drawers) {
  if (!s_swapchain_hooked.load(std::memory_order_relaxed)) {
    IDXGISwapChain* sc = *reinterpret_cast<IDXGISwapChain**>(reinterpret_cast<char*>(This) + 0x490);
    if (sc) {
      HookSwapChain(sc);
    }
  }

  uint64_t waitable_ticks = 0;
  HANDLE hWait = s_frame_latency_waitable_object.load(std::memory_order_acquire);
  if (hWait != nullptr && hWait != INVALID_HANDLE_VALUE && IsWaitableExperimentEnabled()) {
    LARGE_INTEGER w0, w1;
    QueryPerformanceCounter(&w0);
    DWORD wait_res = WaitForSingleObjectEx(hWait, 1000, TRUE);
    QueryPerformanceCounter(&w1);
    if (wait_res == WAIT_OBJECT_0) {
      waitable_ticks = static_cast<uint64_t>(w1.QuadPart - w0.QuadPart);
    } else if (wait_res == WAIT_TIMEOUT) {
      waitable_ticks = static_cast<uint64_t>(w1.QuadPart - w0.QuadPart);
      static std::atomic<uint32_t> s_timeout_logged{0};
      if (s_timeout_logged.fetch_add(1, std::memory_order_relaxed) < 10 && s_trace_file) {
        std::fprintf(s_trace_file, "# [IU-PERF] WAITABLE_OBJECT_TIMEOUT! waited=1000ms\n");
        std::fflush(s_trace_file);
      }
    } else if (wait_res == WAIT_FAILED) {
      DWORD err = GetLastError();
      static std::atomic<uint32_t> s_failed_logged{0};
      if (s_failed_logged.fetch_add(1, std::memory_order_relaxed) < 10 && s_trace_file) {
        std::fprintf(s_trace_file, "# [IU-PERF] WAITABLE_OBJECT_WAIT_FAILED err=%lu\n", (unsigned long)err);
        std::fflush(s_trace_file);
      }
    }
  }

  LARGE_INTEGER t0, t1;
  QueryPerformanceCounter(&t0);

  s_current_frame_timings.present_called = false;
  s_current_frame_timings.t_dxgi_begin = 0;
  s_current_frame_timings.t_dxgi_end = 0;

  PresenterProbe::PaintResult res = s_orig_PaintAndPresentImpl(This, execute_ui_drawers);

  QueryPerformanceCounter(&t1);

  if (s_running.load(std::memory_order_relaxed)) {
    uint64_t prev_end = s_prev_wrapper_end_qpc.load(std::memory_order_relaxed);
    uint64_t between_ticks = (prev_end > 0 && (uint64_t)t0.QuadPart >= prev_end)
                                 ? ((uint64_t)t0.QuadPart - prev_end)
                                 : 0;
    s_prev_wrapper_end_qpc.store(static_cast<uint64_t>(t1.QuadPart), std::memory_order_relaxed);

    uint64_t wrapper_ticks = static_cast<uint64_t>(t1.QuadPart - t0.QuadPart);
    uint64_t pre_present_ticks = 0;
    uint64_t dxgi_present_ticks = 0;
    uint64_t post_present_ticks = 0;

    if (s_current_frame_timings.present_called) {
      pre_present_ticks = s_current_frame_timings.t_dxgi_begin - static_cast<uint64_t>(t0.QuadPart);
      dxgi_present_ticks = s_current_frame_timings.t_dxgi_end - s_current_frame_timings.t_dxgi_begin;
      post_present_ticks = static_cast<uint64_t>(t1.QuadPart) - s_current_frame_timings.t_dxgi_end;
    } else {
      pre_present_ticks = wrapper_ticks;
    }

    uint32_t seq = ++s_frame_sequence;

    if (s_telemetry_enabled.load(std::memory_order_relaxed)) {
      uint32_t count = s_frame_pso.count.exchange(0, std::memory_order_relaxed);
      if (count > 0) {
        uint64_t total_ticks = s_frame_pso.total_ticks.exchange(0, std::memory_order_relaxed);
        uint64_t max_ticks = s_frame_pso.max_ticks.exchange(0, std::memory_order_relaxed);
        double freq = (double)s_qpc_freq.QuadPart;
        if (freq <= 0.0) freq = 10000000.0;
        double total_ms = (double)total_ticks * 1000.0 / freq;
        double max_ms = (double)max_ticks * 1000.0 / freq;

        REXLOG_INFO("PIPELINE_MISSES_FRAME frame={} count={} total_ms={:.3f} max_ms={:.3f}",
                    seq, count, total_ms, max_ms);
        if (s_trace_file) {
          std::fprintf(s_trace_file, "PIPELINE_MISSES_FRAME frame=%u count=%u total_ms=%.3f max_ms=%.3f\n",
                       seq, count, total_ms, max_ms);
        }

        uint64_t cur_worst_frame_ticks = s_session_pso.worst_frame_ticks.load(std::memory_order_relaxed);
        while (total_ticks > cur_worst_frame_ticks &&
               !s_session_pso.worst_frame_ticks.compare_exchange_weak(cur_worst_frame_ticks, total_ticks, std::memory_order_relaxed)) {}
        if (total_ticks >= s_session_pso.worst_frame_ticks.load(std::memory_order_relaxed)) {
          s_session_pso.worst_frame.store(seq, std::memory_order_relaxed);
        }
      }
    }

    uint64_t idx = s_write_idx.fetch_add(1, std::memory_order_relaxed);
    TraceEvent& ev = s_events[idx % kEventBufferSize];
    ev.type = EVENT_FRAME;
    ev.seq = seq;
    ev.thread_id = GetCurrentThreadId();
    ev.hr = static_cast<uint32_t>(res);
    ev.t_begin = static_cast<uint64_t>(t0.QuadPart);
    ev.t_end = static_cast<uint64_t>(t1.QuadPart);
    ev.frame.between_frames_ticks = between_ticks;
    ev.frame.wrapper_ticks = wrapper_ticks;
    ev.frame.pre_present_ticks = pre_present_ticks;
    ev.frame.dxgi_present_ticks = dxgi_present_ticks;
    ev.frame.post_present_ticks = post_present_ticks;
    ev.frame.waitable_ticks = waitable_ticks;
  } else {
    uint32_t seq = ++s_frame_sequence;
    if (s_telemetry_enabled.load(std::memory_order_relaxed)) {
      uint32_t count = s_frame_pso.count.exchange(0, std::memory_order_relaxed);
      if (count > 0) {
        uint64_t total_ticks = s_frame_pso.total_ticks.exchange(0, std::memory_order_relaxed);
        uint64_t max_ticks = s_frame_pso.max_ticks.exchange(0, std::memory_order_relaxed);
        double freq = (double)s_qpc_freq.QuadPart;
        if (freq <= 0.0) freq = 10000000.0;
        double total_ms = (double)total_ticks * 1000.0 / freq;
        double max_ms = (double)max_ticks * 1000.0 / freq;

        REXLOG_INFO("PIPELINE_MISSES_FRAME frame={} count={} total_ms={:.3f} max_ms={:.3f}",
                    seq, count, total_ms, max_ms);
        if (s_trace_file) {
          std::fprintf(s_trace_file, "PIPELINE_MISSES_FRAME frame=%u count=%u total_ms=%.3f max_ms=%.3f\n",
                       seq, count, total_ms, max_ms);
        }

        uint64_t cur_worst_frame_ticks = s_session_pso.worst_frame_ticks.load(std::memory_order_relaxed);
        while (total_ticks > cur_worst_frame_ticks &&
               !s_session_pso.worst_frame_ticks.compare_exchange_weak(cur_worst_frame_ticks, total_ticks, std::memory_order_relaxed)) {}
        if (total_ticks >= s_session_pso.worst_frame_ticks.load(std::memory_order_relaxed)) {
          s_session_pso.worst_frame.store(seq, std::memory_order_relaxed);
        }
      }
    }
  }

  return res;
}

inline void shutdown() {
  s_frame_latency_waitable_object.store(nullptr, std::memory_order_release);

  static std::atomic<bool> summary_emitted{false};
  if (!summary_emitted.exchange(true)) {
    if (s_telemetry_enabled.load(std::memory_order_relaxed) || s_session_pso.total_created.load(std::memory_order_relaxed) > 0) {
      uint32_t total_created = s_session_pso.total_created.load(std::memory_order_relaxed);
      uint64_t total_ticks = s_session_pso.total_ticks.load(std::memory_order_relaxed);
      uint64_t worst_single_ticks = s_session_pso.worst_single_ticks.load(std::memory_order_relaxed);
      uint32_t worst_frame = s_session_pso.worst_frame.load(std::memory_order_relaxed);
      uint64_t worst_frame_ticks = s_session_pso.worst_frame_ticks.load(std::memory_order_relaxed);

      double freq = (double)s_qpc_freq.QuadPart;
      if (freq <= 0.0) freq = 10000000.0;
      double total_creation_ms = (double)total_ticks * 1000.0 / freq;
      double worst_single_ms = (double)worst_single_ticks * 1000.0 / freq;
      double worst_frame_ms = (double)worst_frame_ticks * 1000.0 / freq;

      REXLOG_INFO("PIPELINE_SESSION_SUMMARY total_created={} total_creation_ms={:.3f} worst_single_ms={:.3f} worst_frame={} worst_frame_ms={:.3f}",
                  total_created, total_creation_ms, worst_single_ms, worst_frame, worst_frame_ms);
      if (s_trace_file) {
        std::fprintf(s_trace_file, "PIPELINE_SESSION_SUMMARY total_created=%u total_creation_ms=%.3f worst_single_ms=%.3f worst_frame=%u worst_frame_ms=%.3f\n",
                     total_created, total_creation_ms, worst_single_ms, worst_frame, worst_frame_ms);
        std::fflush(s_trace_file);
      }
    }
  }

  if (s_running.exchange(false)) {
    if (s_worker_thread.joinable()) {
      s_worker_thread.join();
    }
    if (s_trace_file) {
      std::fclose(s_trace_file);
      s_trace_file = nullptr;
    }
  }
}

inline void install_presenter(rex::Runtime* rt) {
  auto* gs = rt ? rt->graphics_system() : nullptr;
  auto* pres = gs ? gs->presenter() : nullptr;
  if (!pres) return;

  void** vtbl = *reinterpret_cast<void***>(pres);
  if (!vtbl || s_orig_PaintAndPresentImpl != nullptr) return;

  DWORD old_protect;
  if (VirtualProtect(&vtbl[6], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
    s_orig_PaintAndPresentImpl = reinterpret_cast<PFN_PaintAndPresentImpl>(vtbl[6]);
    vtbl[6] = reinterpret_cast<void*>(&Hook_PaintAndPresentImpl);
    VirtualProtect(&vtbl[6], sizeof(void*), old_protect, &old_protect);

    if (s_trace_file) {
      std::fprintf(s_trace_file, "# [IU-PERF] PRESENTER_HOOKED pres=%p orig_slot6=%p\n",
                   (void*)pres, (void*)s_orig_PaintAndPresentImpl);
      std::fflush(s_trace_file);
    }
  }
}

inline void HookFactory(IDXGIFactory* factory) {
  if (!factory) return;
  void** fac_vtbl = *reinterpret_cast<void***>(factory);
  if (!fac_vtbl) return;

  DWORD old_protect;
  if (s_orig_CreateSwapChain == nullptr &&
      VirtualProtect(&fac_vtbl[10], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
    s_orig_CreateSwapChain = reinterpret_cast<PFN_CreateSwapChain>(fac_vtbl[10]);
    fac_vtbl[10] = reinterpret_cast<void*>(&Hook_CreateSwapChain);
    VirtualProtect(&fac_vtbl[10], sizeof(void*), old_protect, &old_protect);
  }
  if (s_orig_CreateSwapChainForHwnd == nullptr &&
      VirtualProtect(&fac_vtbl[15], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
    s_orig_CreateSwapChainForHwnd = reinterpret_cast<PFN_CreateSwapChainForHwnd>(fac_vtbl[15]);
    fac_vtbl[15] = reinterpret_cast<void*>(&Hook_CreateSwapChainForHwnd);
    VirtualProtect(&fac_vtbl[15], sizeof(void*), old_protect, &old_protect);
  }
  if (s_trace_file) {
    std::fprintf(s_trace_file, "# [IU-PERF] FACTORY_HOOKED factory=%p orig_CreateSwapChain=%p orig_CreateSwapChainForHwnd=%p\n",
                 (void*)factory, (void*)s_orig_CreateSwapChain, (void*)s_orig_CreateSwapChainForHwnd);
    std::fflush(s_trace_file);
  }
}

inline HRESULT WINAPI Hook_CreateDXGIFactory(REFIID riid, void** ppFactory) {
  HRESULT hr = s_orig_CreateDXGIFactory(riid, ppFactory);
  if (SUCCEEDED(hr) && ppFactory && *ppFactory) {
    HookFactory(reinterpret_cast<IDXGIFactory*>(*ppFactory));
  }
  return hr;
}

inline HRESULT WINAPI Hook_CreateDXGIFactory1(REFIID riid, void** ppFactory) {
  HRESULT hr = s_orig_CreateDXGIFactory1(riid, ppFactory);
  if (SUCCEEDED(hr) && ppFactory && *ppFactory) {
    HookFactory(reinterpret_cast<IDXGIFactory*>(*ppFactory));
  }
  return hr;
}

inline HRESULT WINAPI Hook_CreateDXGIFactory2(UINT Flags, REFIID riid, void** ppFactory) {
  HRESULT hr = s_orig_CreateDXGIFactory2(Flags, riid, ppFactory);
  if (SUCCEEDED(hr) && ppFactory && *ppFactory) {
    HookFactory(reinterpret_cast<IDXGIFactory*>(*ppFactory));
  }
  return hr;
}

inline FARPROC WINAPI Hook_GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
  FARPROC p = s_orig_GetProcAddress(hModule, lpProcName);
  if (!p || (uintptr_t)lpProcName < 0x10000) {
    return p;
  }
  if (std::strcmp(lpProcName, "CreateDXGIFactory2") == 0) {
    if (s_orig_CreateDXGIFactory2 == nullptr) {
      s_orig_CreateDXGIFactory2 = reinterpret_cast<PFN_CreateDXGIFactory2>(p);
    }
    return reinterpret_cast<FARPROC>(&Hook_CreateDXGIFactory2);
  }
  if (std::strcmp(lpProcName, "CreateDXGIFactory1") == 0) {
    if (s_orig_CreateDXGIFactory1 == nullptr) {
      s_orig_CreateDXGIFactory1 = reinterpret_cast<PFN_CreateDXGIFactory1>(p);
    }
    return reinterpret_cast<FARPROC>(&Hook_CreateDXGIFactory1);
  }
  if (std::strcmp(lpProcName, "CreateDXGIFactory") == 0) {
    if (s_orig_CreateDXGIFactory == nullptr) {
      s_orig_CreateDXGIFactory = reinterpret_cast<PFN_CreateDXGIFactory>(p);
    }
    return reinterpret_cast<FARPROC>(&Hook_CreateDXGIFactory);
  }
  return p;
}

inline void HookRexRuntimeIAT() {
  HMODULE hRex = GetModuleHandleW(L"rexruntime.dll");
  if (!hRex) return;

  auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hRex);
  auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<BYTE*>(hRex) + dos->e_lfanew);
  auto import_desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
      reinterpret_cast<BYTE*>(hRex) + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

  for (; import_desc->Name; ++import_desc) {
    const char* dll_name = reinterpret_cast<const char*>(reinterpret_cast<BYTE*>(hRex) + import_desc->Name);
    if (_stricmp(dll_name, "KERNEL32.dll") == 0) {
      auto orig_thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
          reinterpret_cast<BYTE*>(hRex) + (import_desc->OriginalFirstThunk ? import_desc->OriginalFirstThunk : import_desc->FirstThunk));
      auto first_thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
          reinterpret_cast<BYTE*>(hRex) + import_desc->FirstThunk);

      for (; orig_thunk->u1.AddressOfData; ++orig_thunk, ++first_thunk) {
        if (!IMAGE_SNAP_BY_ORDINAL(orig_thunk->u1.Ordinal)) {
          auto import_by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
              reinterpret_cast<BYTE*>(hRex) + orig_thunk->u1.AddressOfData);
          if (std::strcmp(reinterpret_cast<const char*>(import_by_name->Name), "GetProcAddress") == 0) {
            DWORD old_protect;
            if (VirtualProtect(&first_thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &old_protect)) {
              if (s_orig_GetProcAddress == nullptr) {
                s_orig_GetProcAddress = reinterpret_cast<PFN_GetProcAddress>(first_thunk->u1.Function);
                first_thunk->u1.Function = reinterpret_cast<uintptr_t>(&Hook_GetProcAddress);
                if (s_trace_file) {
                  std::fprintf(s_trace_file, "# [IU-PERF] REXRUNTIME_GETPROCADDR_HOOKED orig=%p\n", (void*)s_orig_GetProcAddress);
                  std::fflush(s_trace_file);
                }
              }
              VirtualProtect(&first_thunk->u1.Function, sizeof(void*), old_protect, &old_protect);
            }
            break;
          }
        }
      }
      break;
    }
  }
}

inline void ensure_initialized() {
  static std::atomic<bool> initialized{false};
  if (initialized.exchange(true)) return;

  QueryPerformanceFrequency(&s_qpc_freq);
  QueryPerformanceCounter(&s_trace_start_qpc);
  s_prev_wrapper_end_qpc.store(static_cast<uint64_t>(s_trace_start_qpc.QuadPart), std::memory_order_relaxed);

  if (is_telemetry_enabled()) {
    s_telemetry_enabled.store(true, std::memory_order_relaxed);
  }

  const char* path = std::getenv("IU_PERF_TRACE_PATH");
  if (!path || path[0] == '\0') {
    std::atexit(&shutdown);
    return;
  }

  s_trace_file = std::fopen(path, "w");
  if (!s_trace_file) {
    std::atexit(&shutdown);
    return;
  }
  std::fprintf(s_trace_file, "# Infinite Undiscovery Frame Pacing & Performance Trace\n");
  std::fprintf(s_trace_file, "# QPC Frequency: %llu\n", (unsigned long long)s_qpc_freq.QuadPart);
  std::fprintf(s_trace_file, "# Trace Start QPC: %llu\n", (unsigned long long)s_trace_start_qpc.QuadPart);
  std::fprintf(s_trace_file, "# Experiment Waitable Enabled: %u\n", IsWaitableExperimentEnabled() ? 1 : 0);
  std::fflush(s_trace_file);

  s_running.store(true, std::memory_order_release);
  s_worker_thread = std::thread(&WorkerThreadFunc);
  SetThreadPriority(s_worker_thread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);

  std::atexit(&shutdown);
}

inline void pre_setup() {
  ensure_initialized();
  HookRexRuntimeIAT();
}

inline void install(rex::Runtime* rt) {
  ensure_initialized();

  auto* gs = rt ? rt->graphics_system() : nullptr;
  auto* prov = gs ? static_cast<rex::ui::d3d12::D3D12Provider*>(gs->provider()) : nullptr;
  ID3D12Device* dev = prov ? prov->GetDevice() : nullptr;
  IDXGIFactory2* factory = prov ? prov->GetDXGIFactory() : nullptr;
  if (s_trace_file) {
    std::fprintf(s_trace_file, "# D3D12Device: %p, DXGIFactory: %p\n", (void*)dev, (void*)factory);
    std::fflush(s_trace_file);
  }

  if (dev) {
    void** dev_vtbl = *reinterpret_cast<void***>(dev);
    if (dev_vtbl && s_orig_CreateGraphicsPipelineState == nullptr) {
      DWORD old_protect;
      if (VirtualProtect(&dev_vtbl[10], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
        s_orig_CreateGraphicsPipelineState = reinterpret_cast<PFN_CreateGraphicsPipelineState>(dev_vtbl[10]);
        dev_vtbl[10] = reinterpret_cast<void*>(&Hook_CreateGraphicsPipelineState);
        VirtualProtect(&dev_vtbl[10], sizeof(void*), old_protect, &old_protect);
      }
    }
  }

  if (factory) {
    void** fac_vtbl = *reinterpret_cast<void***>(factory);
    if (fac_vtbl) {
      DWORD old_protect;
      if (s_orig_CreateSwapChain == nullptr &&
          VirtualProtect(&fac_vtbl[10], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
        s_orig_CreateSwapChain = reinterpret_cast<PFN_CreateSwapChain>(fac_vtbl[10]);
        fac_vtbl[10] = reinterpret_cast<void*>(&Hook_CreateSwapChain);
        VirtualProtect(&fac_vtbl[10], sizeof(void*), old_protect, &old_protect);
      }
      if (s_orig_CreateSwapChainForHwnd == nullptr &&
          VirtualProtect(&fac_vtbl[15], sizeof(void*), PAGE_EXECUTE_READWRITE, &old_protect)) {
        s_orig_CreateSwapChainForHwnd = reinterpret_cast<PFN_CreateSwapChainForHwnd>(fac_vtbl[15]);
        fac_vtbl[15] = reinterpret_cast<void*>(&Hook_CreateSwapChainForHwnd);
        VirtualProtect(&fac_vtbl[15], sizeof(void*), old_protect, &old_protect);
      }
      if (s_trace_file) {
        std::fprintf(s_trace_file, "# [IU-PERF] FACTORY_HOOKED orig_CreateSwapChain=%p orig_CreateSwapChainForHwnd=%p\n",
                     (void*)s_orig_CreateSwapChain, (void*)s_orig_CreateSwapChainForHwnd);
        std::fflush(s_trace_file);
      }
    }
  }

  install_presenter(rt);
}

inline void prewarm_wait() {
  ensure_initialized();

  if (!is_prewarm_enabled()) {
    REXLOG_INFO("[PSO_PREWARM] Disabled (skipping prewarm wait)");
    return;
  }

  LARGE_INTEGER t0, t1;
  QueryPerformanceCounter(&t0);
  double freq = (double)s_qpc_freq.QuadPart;
  if (freq <= 0.0) freq = 10000000.0;

  REXLOG_INFO("IU_PSO_PREWARM_BEGIN");
  if (s_trace_file) {
    std::fprintf(s_trace_file, "IU_PSO_PREWARM_BEGIN\n");
    std::fflush(s_trace_file);
  }

  uint32_t start_created = s_pso_total_created.load(std::memory_order_relaxed);

  const uint64_t quiet_ticks = static_cast<uint64_t>((50.0 / 1000.0) * freq);
  const uint64_t timeout_ticks = static_cast<uint64_t>((5000.0 / 1000.0) * freq);

  while (true) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    uint64_t elapsed_ticks = static_cast<uint64_t>(now.QuadPart - t0.QuadPart);
    if (elapsed_ticks >= timeout_ticks) {
      break;
    }

    uint32_t active = s_pso_active_compilations.load(std::memory_order_acquire);
    uint64_t last_end = s_last_pso_end_qpc.load(std::memory_order_acquire);

    if (active == 0) {
      if (last_end == 0 || (static_cast<uint64_t>(now.QuadPart) - last_end) >= quiet_ticks) {
        break;
      }
    }

    Sleep(2);
  }

  QueryPerformanceCounter(&t1);
  double wait_ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / freq;
  uint32_t end_created = s_pso_total_created.load(std::memory_order_relaxed);
  uint32_t pipelines_waited = end_created >= start_created ? (end_created - start_created) : 0;

  REXLOG_INFO("IU_PSO_PREWARM_END pipelines_waited={} wait_ms={:.2f}", pipelines_waited, wait_ms);
  if (s_trace_file) {
    std::fprintf(s_trace_file, "IU_PSO_PREWARM_END pipelines_waited=%u wait_ms=%.2f\n", pipelines_waited, wait_ms);
    std::fflush(s_trace_file);
  }
}

}  // namespace pso_diag

#pragma once

#include <atomic>
#include <cstdint>
#include "iu_trace.h"

// Guest declarations (declared in infinite_undiscovery_funcs.h)
DECLARE_REX_FUNC(sub_82475670);
DECLARE_REX_FUNC(sub_82147140);
DECLARE_REX_FUNC(sub_8219CAF0);
DECLARE_REX_FUNC(sub_826D7E08);
DECLARE_REX_FUNC(sub_8247CC38);
DECLARE_REX_FUNC(sub_825B8288);
DECLARE_REX_FUNC(sub_82499198);

namespace iu::save_anywhere {

struct State {
  std::atomic<bool> request_pending{false};
  std::atomic<bool> reset_pending{false};
  std::atomic<bool> is_active{false};
  std::atomic<uint32_t> dialog_handle{0};
  std::atomic<uint32_t> dialog_object{0};
  std::atomic<uint64_t> active_start_time_ms{0};
};

inline State g_state;

inline void Request() {
  using namespace iu::trace;
  if (g_state.is_active.load(std::memory_order_relaxed)) {
    emit(kSavePoint, "SAVE_ANYWHERE_REQUEST ignored: dialog already active (handle=0x%08X)",
         g_state.dialog_handle.load(std::memory_order_relaxed));
    return;
  }
  emit(kSavePoint, "SAVE_ANYWHERE_REQUEST initiated from Community Debug UI");
  g_state.request_pending.store(true, std::memory_order_release);
}

inline bool IsActive() {
  return g_state.is_active.load(std::memory_order_relaxed);
}

inline uint32_t GetDialogHandle() {
  return g_state.dialog_handle.load(std::memory_order_relaxed);
}

// Retained for diagnostic inspection / tests
inline uint32_t GetDialogObject() {
  return g_state.dialog_object.load(std::memory_order_relaxed);
}

inline void ResetActive() {
  using namespace iu::trace;
  emit(kSavePoint, "SAVE_ANYWHERE manual safe reset requested (active=%d handle=0x%08X)",
       g_state.is_active.load(std::memory_order_relaxed) ? 1 : 0,
       g_state.dialog_handle.load(std::memory_order_relaxed));
  g_state.reset_pending.store(true, std::memory_order_release);
}

inline void PerformSafeCleanup(PPCContext& ctx, uint8_t* base, uint32_t temp_sp, const char* reason) {
  using namespace iu::trace;
  const uint32_t handle = g_state.dialog_handle.load(std::memory_order_relaxed);
  emit(kSavePoint, "SAVE_ANYWHERE executing safe cleanup (%s, handle=0x%08X)", reason, handle);

  // Step A: clear modal UI state in global context *(0x82A58EB8 + 404)
  // Exactly matching sub_8263CDD0 line 20883: sub_8247CC38(0x82A58EB8 + 404, 0)
  PPCContext clean_ctx = ctx;
  clean_ctx.r1.u32 = temp_sp;
  clean_ctx.r3.s64 = 0x82A58EB8 + 404;
  clean_ctx.r4.s64 = 0;
  sub_8247CC38(clean_ctx, base);

  // Step B: restore player actor input
  // Exactly matching sub_8263CDD0 line 20888-20893:
  //   input_mgr = sub_825B8288(0x82A58EB8)
  //   sub_82499198(input_mgr, 1)
  clean_ctx.r1.u32 = temp_sp;
  clean_ctx.r3.u64 = 0x82A58EB8;
  sub_825B8288(clean_ctx, base);
  const uint32_t input_mgr = clean_ctx.r3.u32;
  if (input_mgr != 0) {
    clean_ctx.r1.u32 = temp_sp;
    clean_ctx.r3.u64 = input_mgr;
    clean_ctx.r4.s64 = 1;
    sub_82499198(clean_ctx, base);
  }

  g_state.is_active.store(false, std::memory_order_release);
  g_state.dialog_handle.store(0, std::memory_order_release);
  g_state.dialog_object.store(0, std::memory_order_release);
  g_state.active_start_time_ms.store(0, std::memory_order_release);
  g_state.reset_pending.store(false, std::memory_order_release);
}

inline void PollAndDispatch(PPCContext& ctx, uint8_t* base) {
  // Check guest memory and stack sanity for any host dispatch
  const uint32_t original_sp = ctx.r1.u32;
  const bool sp_valid = (original_sp >= 0x00010000 && original_sp < 0xF0000000);
  const uint32_t temp_sp = sp_valid ? ((original_sp - 64) & ~0xFu) : 0;
  if (sp_valid && base != nullptr) {
    REX_STORE_U32(temp_sp, original_sp);
  }

  // 1. Process pending manual safe reset
  if (g_state.reset_pending.load(std::memory_order_acquire)) {
    if (base != nullptr && sp_valid) {
      PerformSafeCleanup(ctx, base, temp_sp, "manual reset requested");
    } else {
      using namespace iu::trace;
      emit(kSavePoint, "SAVE_ANYWHERE manual reset deferred: invalid SP or null base");
    }
  }

  // 2. If active, check if native dialog is still alive or watchdog expired
  if (g_state.is_active.load(std::memory_order_relaxed)) {
    const uint32_t handle = g_state.dialog_handle.load(std::memory_order_relaxed);
    if (handle != 0 && base != nullptr && sp_valid) {
      // In sub_8263CBB8 / sub_8263CDD0: object table pointer is at *(0x82A380F8 + 24)
      const uint32_t table_ref = 0x82A380F8;
      const uint32_t table = REX_LOAD_U32(table_ref + 24);
      if (table != 0) {
        PPCContext check_ctx = ctx;
        check_ctx.r1.u32 = temp_sp;
        check_ctx.r3.u64 = table;
        check_ctx.r4.u64 = handle;
        check_ctx.r5.s64 = 7; // Dialog entity type
        sub_82147140(check_ctx, base);

        if (check_ctx.r3.u32 == 0) {
          // Dialog handle no longer resolved -> dialog was dismissed normally (saved or cancelled)
          PerformSafeCleanup(ctx, base, temp_sp, "dialog dismissed normally");
        } else {
          // Dialog still alive in object table. Check conservative watchdog.
          const uint64_t start_ms = g_state.active_start_time_ms.load(std::memory_order_relaxed);
          if (start_ms != 0) {
            const uint64_t now = GetTickCount64();
            constexpr uint64_t kWatchdogTimeoutMs = 120000; // 120s conservative ceiling
            if (now >= start_ms && (now - start_ms) > kWatchdogTimeoutMs) {
              // Safety: only trigger watchdog if NOT actively writing async save data
              const uint32_t dialog_obj = g_state.dialog_object.load(std::memory_order_relaxed);
              bool write_busy = false;
              if (dialog_obj != 0 && iu::trace::readable(base, dialog_obj, 12850)) {
                write_busy = (REX_LOAD_U8(dialog_obj + 12844) != 0);
              }
              if (!write_busy) {
                using namespace iu::trace;
                emit(kSavePoint, "SAVE_ANYWHERE watchdog expired (>120s idle), triggering safe recovery cleanup");
                PerformSafeCleanup(ctx, base, temp_sp, "watchdog timeout (>120s)");
              }
            }
          }
        }
      }
    }
  }

  // 2. Process pending request
  if (!g_state.request_pending.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  using namespace iu::trace;

  // ---------------- Conservative Precondition Safety Guards ----------------
  // Guard 1: Save Anywhere already active
  if (g_state.is_active.load(std::memory_order_relaxed)) {
    emit(kSavePoint, "SAVE_ANYWHERE_REQUEST rejected: Save Anywhere dialog already active (handle=0x%08X)",
         g_state.dialog_handle.load(std::memory_order_relaxed));
    return;
  }

  // Guard 2: Async save/write activity in progress on last dialog instance
  const uint32_t last_dialog = g_state.dialog_object.load(std::memory_order_relaxed);
  if (last_dialog != 0 && readable(base, last_dialog, 12850)) {
    const uint8_t write_busy = REX_LOAD_U8(last_dialog + 12844);
    if (write_busy != 0) {
      emit(kSavePoint, "SAVE_ANYWHERE_REQUEST rejected: async save write in progress on dialog 0x%08X (+12844=%u)",
           last_dialog, write_busy);
      return;
    }
  }

  // Guard 3: UI focus manager stack busy (native Save/Load or other modal UI already active)
  const uint32_t table_ref = 0x82A380F8;
  if (!readable(base, table_ref, 52)) {
    emit(kSavePoint, "SAVE_ANYWHERE_REQUEST rejected: engine UI table 0x%08X not readable/ready", table_ref);
    return;
  }
  const uint32_t ui_mgr = REX_LOAD_U32(table_ref + 48);
  if (ui_mgr != 0 && readable(base, ui_mgr, 220)) {
    const uint32_t active_dialogs = REX_LOAD_U32(ui_mgr + 212);
    if (active_dialogs > 0) {
      emit(kSavePoint, "SAVE_ANYWHERE_REQUEST rejected: UI focus stack already has active dialogs (count=%u)",
           active_dialogs);
      return;
    }
  }

  // Diagnostic: Log player gameplay input state (informative only, non-blocking)
  const uint32_t global_ctx = 0x82A58EA8;
  if (readable(base, global_ctx, 16)) {
    const uint32_t input_mgr = REX_LOAD_U32(global_ctx + 8);
    if (input_mgr != 0 && readable(base, input_mgr, 80)) {
      const uint8_t input_enabled = REX_LOAD_U8(input_mgr + 72);
      emit(kSavePoint, "SAVE_ANYWHERE SAFETY_STATE input_mgr=0x%08X input72=%u",
           input_mgr, input_enabled);
    }
  }

  // Guard 4: Guest stack pointer sanity check
  if (!sp_valid) {
    emit(kSavePoint, "SAVE_ANYWHERE_REQUEST rejected: invalid guest SP=0x%08X", original_sp);
    return;
  }

  // Allocate 64 bytes on the guest stack for the 8-byte holder and backchain
  const uint32_t frame_size = 64;
  const uint32_t holder_sp = (original_sp - frame_size) & ~0xFu;
  const uint32_t holder_guest = holder_sp + 16;
  REX_STORE_U32(holder_sp, original_sp);

  // Initialize holder[0] = 0, holder[1] = 0
  REX_STORE_U32(holder_guest + 0, 0);
  REX_STORE_U32(holder_guest + 4, 0);

  // Reproduce the proven normal call contract for sub_82475670:
  // r3 = guest pointer to holder
  // r6 = 0
  // r7 = 0
  // f1 = 58.0f
  // f2 = 27.0f
  PPCContext dispatch_ctx = ctx;
  dispatch_ctx.r1.u32 = holder_sp;
  dispatch_ctx.r3.u64 = holder_guest;
  dispatch_ctx.r6.u64 = 0;
  dispatch_ctx.r7.u64 = 0;
  dispatch_ctx.fpscr.disableFlushMode();
  dispatch_ctx.f1.f64 = 58.0;
  dispatch_ctx.f2.f64 = 27.0;

  emit(kSavePoint, "SAVE_ANYWHERE BEFORE sub_82475670 r3=0x%08X (holder) r6=0 r7=0 f1=58.0000 f2=27.0000 SP=0x%08X",
       holder_guest, holder_sp);

  sub_82475670(dispatch_ctx, base);

  const uint32_t h0 = REX_LOAD_U32(holder_guest + 0);
  const uint32_t h1 = REX_LOAD_U32(holder_guest + 4);

  emit(kSavePoint, "SAVE_ANYWHERE AFTER sub_82475670 ret_r3=0x%08X holder[0]=0x%08X holder[1]=0x%08X",
       dispatch_ctx.r3.u32, h0, h1);

  if (h1 == 0) {
    emit(kSavePoint, "SAVE_ANYWHERE FAILED: holder[1] handle is 0");
    return;
  }

  // In sub_8263CBB8: resolve dialog object pointer from handle using sub_82147140
  const uint32_t table = REX_LOAD_U32(table_ref + 24);
  dispatch_ctx.r1.u32 = holder_sp;
  dispatch_ctx.r3.u64 = table;
  dispatch_ctx.r4.u64 = h1;
  dispatch_ctx.r5.s64 = 7; // Dialog entity type
  sub_82147140(dispatch_ctx, base);
  const uint32_t dialog_obj = dispatch_ctx.r3.u32;

  emit(kSavePoint, "SAVE_ANYWHERE RESOLVED dialog_obj=0x%08X (h0=0x%08X table=0x%08X)",
       dialog_obj, h0, table);

  if (dialog_obj == 0) {
    emit(kSavePoint, "SAVE_ANYWHERE FAILED: dialog_obj could not be resolved from handle 0x%08X", h1);
    return;
  }

  // Exact post-sub_82475670 sequence from sub_8263CBB8 lines 22033-22063:
  // Step 1: Set mode 1 (Save mode) at offset 12592
  REX_STORE_U32(dialog_obj + 12592, 1);

  // Step 2: Activate dialog widget via sub_8219CAF0(dialog_obj, 1)
  dispatch_ctx.r1.u32 = holder_sp;
  dispatch_ctx.r3.u64 = dialog_obj;
  dispatch_ctx.r4.s64 = 1;
  sub_8219CAF0(dispatch_ctx, base);

  // Step 3: Register dialog handle onto active UI focus stack via sub_826D7E08(dialog_obj)
  dispatch_ctx.r1.u32 = holder_sp;
  dispatch_ctx.r3.u64 = dialog_obj;
  sub_826D7E08(dispatch_ctx, base);

  // Step 4: Register modal UI state on global interaction manager via sub_8247CC38(0x82A58EB8 + 404, 1)
  dispatch_ctx.r1.u32 = holder_sp;
  dispatch_ctx.r3.s64 = 0x82A58EB8 + 404;
  dispatch_ctx.r4.s64 = 1;
  sub_8247CC38(dispatch_ctx, base);

  // Step 5: Switch input focus from player actor to UI dialog via:
  //   input_mgr = sub_825B8288(0x82A58EB8)
  //   sub_82499198(input_mgr, 0)
  dispatch_ctx.r1.u32 = holder_sp;
  dispatch_ctx.r3.u64 = 0x82A58EB8;
  sub_825B8288(dispatch_ctx, base);
  const uint32_t input_mgr = dispatch_ctx.r3.u32;
  if (input_mgr != 0) {
    dispatch_ctx.r1.u32 = holder_sp;
    dispatch_ctx.r3.u64 = input_mgr;
    dispatch_ctx.r4.s64 = 0;
    sub_82499198(dispatch_ctx, base);
  }

  emit(kSavePoint, "SAVE_ANYWHERE ACTIVATED: mode=1, sub_8219CAF0(1), sub_826D7E08, modal=1, input_mgr=0x%08X disabled",
       input_mgr);

  g_state.active_start_time_ms.store(GetTickCount64(), std::memory_order_release);
  g_state.dialog_object.store(dialog_obj, std::memory_order_release);
  g_state.dialog_handle.store(h1, std::memory_order_release);
  g_state.is_active.store(true, std::memory_order_release);
}

}  // namespace iu::save_anywhere

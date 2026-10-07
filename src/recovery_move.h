#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include "iu_trace.h"

#include "save_anywhere.h"

// Guest entity and transform functions
DECLARE_REX_FUNC(sub_82147140);
DECLARE_REX_FUNC(sub_8217DBB0);

namespace iu::recovery_move {

struct UndoState {
  std::atomic<bool> valid{false};
  uint32_t player_handle{0};
  uint32_t player_obj{0};
  float orig_x{0.0f};
  float orig_y{0.0f};
  float orig_z{0.0f};
};

struct WatchState {
  bool active{false};
  uint32_t ticks_remaining{0};
  uint32_t tick_count{0};
  uint32_t player_obj{0};
  uint32_t player_handle{0};
  float orig_x{0.0f};
  float orig_y{0.0f};
  float orig_z{0.0f};
  float target_x{0.0f};
  float target_y{0.0f};
  float target_z{0.0f};
};

struct State {
  std::atomic<bool> step_requested{false};
  std::atomic<bool> undo_requested{false};
  std::atomic<float> step_distance{300.0f};
  UndoState undo;
  WatchState watch;
};

inline State g_state;

inline void RequestSafeStep(float distance = 300.0f) {
  using namespace iu::trace;
  emit(kSavePoint, "SAFE_STEP_REQUEST initiated distance=%.2f units", distance);
  g_state.step_distance.store(distance, std::memory_order_relaxed);
  g_state.step_requested.store(true, std::memory_order_release);
}

inline void RequestUndo() {
  using namespace iu::trace;
  emit(kSavePoint, "UNDO_MOVE_REQUEST initiated");
  g_state.undo_requested.store(true, std::memory_order_release);
}

inline bool CanUndo() {
  return g_state.undo.valid.load(std::memory_order_acquire);
}

inline void ClearUndo() {
  g_state.undo.valid.store(false, std::memory_order_release);
  g_state.undo.player_handle = 0;
  g_state.undo.player_obj = 0;
}

inline float LoadGuestF32(uint8_t* base, uint32_t guest_addr) {
  PPCRegister temp;
  temp.u32 = REX_LOAD_U32(guest_addr);
  return temp.f32;
}

inline void StoreGuestF32(uint8_t* base, uint32_t guest_addr, float val) {
  PPCRegister temp;
  temp.f32 = val;
  REX_STORE_U32(guest_addr, temp.u32);
}

inline void PollAndDispatch(PPCContext& ctx, uint8_t* base) {
  using namespace iu::trace;

  // 0. Process post-step observation on subsequent game loop ticks
  if (g_state.watch.active && base != nullptr) {
    const uint32_t p_obj = g_state.watch.player_obj;
    if (readable(base, p_obj, 880)) {
      const float tick_x = LoadGuestF32(base, p_obj + 856);
      const float tick_y = LoadGuestF32(base, p_obj + 860);
      const float tick_z = LoadGuestF32(base, p_obj + 864);
      g_state.watch.tick_count++;

      emit(kSavePoint, "SAFE_STEP_DIAG_5_NEXT_TICK tick=%u player=0x%08X handle=0x%08X pos=(%.4f, %.4f, %.4f) delta_from_cur=(%.4f, %.4f, %.4f) delta_from_target=(%.4f, %.4f, %.4f)",
           g_state.watch.tick_count, p_obj, g_state.watch.player_handle,
           tick_x, tick_y, tick_z,
           tick_x - g_state.watch.orig_x, tick_y - g_state.watch.orig_y, tick_z - g_state.watch.orig_z,
           tick_x - g_state.watch.target_x, tick_y - g_state.watch.target_y, tick_z - g_state.watch.target_z);

      if (g_state.watch.ticks_remaining > 0) {
        g_state.watch.ticks_remaining--;
      } else {
        g_state.watch.active = false;
      }
    } else {
      g_state.watch.active = false;
    }
  }

  // Dequeue requests prioritizing Undo if both somehow were requested
  bool do_undo = g_state.undo_requested.exchange(false, std::memory_order_acq_rel);
  bool do_step = false;
  if (!do_undo) {
    do_step = g_state.step_requested.exchange(false, std::memory_order_acq_rel);
  } else {
    // Drop simultaneous step request safely to prevent conflicting movements
    g_state.step_requested.store(false, std::memory_order_release);
  }

  if (!do_step && !do_undo) {
    return;
  }

  // 1. Validate guest memory and context
  if (base == nullptr) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: null guest memory base");
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: null guest memory base");
    return;
  }

  const uint32_t original_sp = ctx.r1.u32;
  if (original_sp < 0x00010000 || original_sp >= 0xF0000000) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: invalid guest SP=0x%08X", original_sp);
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: invalid guest SP=0x%08X", original_sp);
    return;
  }

  // Guard: Save Anywhere active check
  if (iu::save_anywhere::IsActive()) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: Save Anywhere dialog active (handle=0x%08X)",
                      iu::save_anywhere::GetDialogHandle());
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: Save Anywhere dialog active (handle=0x%08X)",
                      iu::save_anywhere::GetDialogHandle());
    return;
  }

  // Guard: UI focus stack check (modal / save-load dialog already active)
  const uint32_t table_ref = 0x82A380F8;
  if (!readable(base, table_ref, 52)) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: engine table 0x%08X not readable", table_ref);
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: engine table 0x%08X not readable", table_ref);
    return;
  }
  const uint32_t ui_mgr = REX_LOAD_U32(table_ref + 48);
  if (ui_mgr != 0 && readable(base, ui_mgr, 220)) {
    const uint32_t active_dialogs = REX_LOAD_U32(ui_mgr + 212);
    if (active_dialogs > 0) {
      if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: UI focus stack has active dialogs (count=%u)", active_dialogs);
      if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: UI focus stack has active dialogs (count=%u)", active_dialogs);
      return;
    }
  }

  // Guard: Player controllable / gameplay input check
  const uint32_t global_ctx = 0x82A58EA8;
  if (readable(base, global_ctx, 16)) {
    const uint32_t input_mgr = REX_LOAD_U32(global_ctx + 8);
    if (input_mgr != 0 && readable(base, input_mgr, 80)) {
      const uint8_t input_enabled = REX_LOAD_U8(input_mgr + 72);
      if (input_enabled == 0) {
        if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: player input is disabled (input_mgr=0x%08X input72=0)", input_mgr);
        if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: player input is disabled (input_mgr=0x%08X input72=0)", input_mgr);
        return;
      }
    }
  }

  const uint32_t entity_table = REX_LOAD_U32(table_ref + 24);
  if (entity_table == 0) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: entity table pointer is 0");
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: entity table pointer is 0");
    return;
  }

  // 3. Resolve controlled player handle from global context 0x82A58EA8
  if (!readable(base, global_ctx, 124)) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: global context 0x%08X not readable", global_ctx);
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: global context 0x%08X not readable", global_ctx);
    return;
  }
  const uint32_t player_handle = REX_LOAD_U32(global_ctx + 120);
  if (player_handle == 0) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: player handle at 0x%08X+120 is 0", global_ctx);
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: player handle at 0x%08X+120 is 0", global_ctx);
    return;
  }

  // 4. Resolve Player Actor Object (entity type 19)
  PPCContext call_ctx = ctx;
  const uint32_t temp_sp = (original_sp - 64) & ~0xFu;
  REX_STORE_U32(temp_sp, original_sp);
  call_ctx.r1.u32 = temp_sp;
  call_ctx.r3.u64 = entity_table;
  call_ctx.r4.u64 = player_handle;
  call_ctx.r5.s64 = 19;  // Actor entity type
  sub_82147140(call_ctx, base);
  const uint32_t player_obj = call_ctx.r3.u32;

  if (player_obj == 0 || !readable(base, player_obj, 880)) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: player object 0x%08X invalid/unreadable", player_obj);
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: player object 0x%08X invalid/unreadable", player_obj);
    return;
  }

  // 5. Read current position from player_obj (+856, +860, +864)
  const float cur_x = LoadGuestF32(base, player_obj + 856);
  const float cur_y = LoadGuestF32(base, player_obj + 860);
  const float cur_z = LoadGuestF32(base, player_obj + 864);

  if (std::isnan(cur_x) || std::isnan(cur_y) || std::isnan(cur_z) ||
      std::isinf(cur_x) || std::isinf(cur_y) || std::isinf(cur_z)) {
    if (do_step) emit(kSavePoint, "SAFE_STEP_REJECTED: non-finite player coordinates");
    if (do_undo) emit(kSavePoint, "UNDO_MOVE_REJECTED: non-finite player coordinates");
    return;
  }

  // Handle UNDO MOVE
  if (do_undo) {
    if (!g_state.undo.valid.load(std::memory_order_acquire)) {
      emit(kSavePoint, "UNDO_MOVE_REJECTED: no previous debug move stored");
      return;
    }
    if (g_state.undo.player_handle != player_handle) {
      emit(kSavePoint, "UNDO_MOVE_REJECTED: player handle changed (stored=0x%08X cur=0x%08X)",
           g_state.undo.player_handle, player_handle);
      ClearUndo();
      return;
    }
    if (g_state.undo.player_obj != player_obj) {
      emit(kSavePoint, "UNDO_MOVE_REJECTED: player instance changed (stored=0x%08X cur=0x%08X)",
           g_state.undo.player_obj, player_obj);
      ClearUndo();
      return;
    }

    emit(kSavePoint, "UNDO_MOVE_BEFORE player=0x%08X handle=0x%08X cur_pos=(%.4f, %.4f, %.4f) target_pos=(%.4f, %.4f, %.4f)",
         player_obj, player_handle, cur_x, cur_y, cur_z,
         g_state.undo.orig_x, g_state.undo.orig_y, g_state.undo.orig_z);

    // Restore original position via native authoritative setter sub_8217DBB0
    const uint32_t vec_sp = (original_sp - 64) & ~0xFu;
    const uint32_t vec_guest = vec_sp + 16;
    REX_STORE_U32(vec_sp, original_sp);
    StoreGuestF32(base, vec_guest + 0, g_state.undo.orig_x);
    StoreGuestF32(base, vec_guest + 4, g_state.undo.orig_y);
    StoreGuestF32(base, vec_guest + 8, g_state.undo.orig_z);

    PPCContext undo_ctx = ctx;
    undo_ctx.r1.u32 = vec_sp;
    undo_ctx.r3.u64 = player_obj;
    undo_ctx.r4.u64 = vec_guest;
    sub_8217DBB0(undo_ctx, base);

    const float restored_x = LoadGuestF32(base, player_obj + 856);
    const float restored_y = LoadGuestF32(base, player_obj + 860);
    const float restored_z = LoadGuestF32(base, player_obj + 864);

    emit(kSavePoint, "UNDO_MOVE_AFTER player=0x%08X handle=0x%08X restored_pos=(%.4f, %.4f, %.4f)",
         player_obj, player_handle, restored_x, restored_y, restored_z);

    ClearUndo();
    return;
  }

  // Handle SAFE STEP FORWARD
  if (do_step) {
    // 3. Raw forward (+668, +672, +676)
    const float raw_fx = LoadGuestF32(base, player_obj + 668);
    const float raw_fy = LoadGuestF32(base, player_obj + 672);
    const float raw_fz = LoadGuestF32(base, player_obj + 676);

    // 4. Horizontal forward length before normalization
    const float h_len_sq = raw_fx * raw_fx + raw_fz * raw_fz;
    const float h_len = std::sqrt(h_len_sq);

    // 1 & 2: Log player_obj and current position
    emit(kSavePoint, "SAFE_STEP_DIAG_1_POS player=0x%08X handle=0x%08X cur_pos=(%.4f, %.4f, %.4f)",
         player_obj, player_handle, cur_x, cur_y, cur_z);

    // 3 & 4: Log raw forward and horizontal length before normalization
    emit(kSavePoint, "SAFE_STEP_DIAG_2_RAW_FWD player=0x%08X raw_fwd=(%.4f, %.4f, %.4f) raw_h_len=%.6f",
         player_obj, raw_fx, raw_fy, raw_fz, h_len);

    if (h_len_sq < 1e-6f || std::isnan(h_len_sq) || std::isinf(h_len_sq)) {
      emit(kSavePoint, "SAFE_STEP_REJECTED: player forward vector invalid (raw=(%.4f, %.4f, %.4f) h_len=%.6f)",
           raw_fx, raw_fy, raw_fz, h_len);
      return;
    }

    // 5. Normalized Fx/Fz
    const float inv_len = 1.0f / h_len;
    const float norm_fx = raw_fx * inv_len;
    const float norm_fz = raw_fz * inv_len;

    // 6. Configured step distance
    const float dist = g_state.step_distance.load(std::memory_order_relaxed);

    // 7. Calculated target X/Y/Z
    const float target_x = cur_x + norm_fx * dist;
    const float target_y = cur_y;  // Keep elevation unchanged
    const float target_z = cur_z + norm_fz * dist;

    // 8. Actual delta target - current
    const float delta_x = target_x - cur_x;
    const float delta_y = target_y - cur_y;
    const float delta_z = target_z - cur_z;

    emit(kSavePoint, "SAFE_STEP_DIAG_3_PLAN norm_fwd=(%.4f, %.4f) dist=%.4f target=(%.4f, %.4f, %.4f) delta=(%.4f, %.4f, %.4f)",
         norm_fx, norm_fz, dist, target_x, target_y, target_z, delta_x, delta_y, delta_z);

    emit(kSavePoint, "SAFE_STEP_BEFORE player=0x%08X handle=0x%08X pos=(%.4f, %.4f, %.4f) fwd=(%.4f, %.4f, %.4f)",
         player_obj, player_handle, cur_x, cur_y, cur_z, norm_fx, 0.0f, norm_fz);

    // Save previous state to Undo buffer before modifying position
    g_state.undo.player_handle = player_handle;
    g_state.undo.player_obj = player_obj;
    g_state.undo.orig_x = cur_x;
    g_state.undo.orig_y = cur_y;
    g_state.undo.orig_z = cur_z;
    g_state.undo.valid.store(true, std::memory_order_release);

    // Apply forward displacement in horizontal plane via native authoritative setter sub_8217DBB0
    const uint32_t vec_sp = (original_sp - 64) & ~0xFu;
    const uint32_t vec_guest = vec_sp + 16;
    REX_STORE_U32(vec_sp, original_sp);
    StoreGuestF32(base, vec_guest + 0, target_x);
    StoreGuestF32(base, vec_guest + 4, target_y);
    StoreGuestF32(base, vec_guest + 8, target_z);

    PPCContext step_ctx = ctx;
    step_ctx.r1.u32 = vec_sp;
    step_ctx.r3.u64 = player_obj;
    step_ctx.r4.u64 = vec_guest;
    sub_8217DBB0(step_ctx, base);

    // 9. Position immediately after sub_8217DBB0
    const float post_x = LoadGuestF32(base, player_obj + 856);
    const float post_y = LoadGuestF32(base, player_obj + 860);
    const float post_z = LoadGuestF32(base, player_obj + 864);

    emit(kSavePoint, "SAFE_STEP_DIAG_4_POST_COMMIT player=0x%08X post_pos=(%.4f, %.4f, %.4f) delta_from_cur=(%.4f, %.4f, %.4f) delta_from_target=(%.4f, %.4f, %.4f)",
         player_obj, post_x, post_y, post_z,
         post_x - cur_x, post_y - cur_y, post_z - cur_z,
         post_x - target_x, post_y - target_y, post_z - target_z);

    emit(kSavePoint, "SAFE_STEP_AFTER player=0x%08X handle=0x%08X pos=(%.4f, %.4f, %.4f) delta=(%.4f, %.4f, %.4f)",
         player_obj, player_handle, post_x, post_y, post_z,
         post_x - cur_x, post_y - cur_y, post_z - cur_z);

    // 10. Arm next-tick observation
    g_state.watch.active = true;
    g_state.watch.ticks_remaining = 2;
    g_state.watch.tick_count = 0;
    g_state.watch.player_obj = player_obj;
    g_state.watch.player_handle = player_handle;
    g_state.watch.orig_x = cur_x;
    g_state.watch.orig_y = cur_y;
    g_state.watch.orig_z = cur_z;
    g_state.watch.target_x = target_x;
    g_state.watch.target_y = target_y;
    g_state.watch.target_z = target_z;
  }
}

}  // namespace iu::recovery_move

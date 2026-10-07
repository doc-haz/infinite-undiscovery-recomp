#pragma once

#include <windows.h>
#include <cstdlib>
#include "save_anywhere.h"
#include "recovery_move.h"

// Local override of the mapped SDK hook. Resolve the original from the DLL
// explicitly, so both direct calls and dispatch-table calls use this wrapper.
extern "C" void rexcrt_SwitchToFiber(PPCContext& ctx, uint8_t* base) {
  static PPCFunc* const original = []() -> PPCFunc* {
    const auto module = GetModuleHandleW(L"rexruntime.dll");
    const auto proc = module ? GetProcAddress(module, "rexcrt_SwitchToFiber") : nullptr;
    if (!proc) std::abort();
    return reinterpret_cast<PPCFunc*>(proc);
  }();

  iu::save_anywhere::PollAndDispatch(ctx, base);
  iu::recovery_move::PollAndDispatch(ctx, base);

  // This activation is suspended on its own host fiber stack. Unlike shared
  // PPCContext, its saved LR belongs to this continuation (all 64 bits).
  const uint64_t continuation_lr = ctx.lr;
  original(ctx, base);
  ctx.lr = continuation_lr;
}

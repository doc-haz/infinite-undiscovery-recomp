# IU Debug Trace Framework

Reusable, category-gated, non-blocking diagnostic tracing for Infinite Undiscovery Recomp.
Purely observational: no breakpoints, no guest-state changes, pass-through when disabled.

## Files

| File | Role |
|---|---|
| `src/iu_trace.h` (new) | Framework: categories, lock-free ring + writer thread, guest memory/vtable/register helpers, `IU_TRACE*` macros. |
| `src/iu_trace_points.h` (new) | Guest trace points. Currently `sub_8263CBB8` (SAVEPOINT). |
| `src/vesplume_diag.h` (changed) | Logging rerouted through the common sink; same file, env var, header and line format. Hooks unchanged. |
| `src/main.cpp` (changed) | Includes `iu_trace_points.h`. |
| `src/infinite_undiscovery_app.h` (changed) | `SetupEnvironment` sets `IU_TRACE_DIR` to `<portable root>/logs`. |

## Enabling categories

Set `IU_TRACE` before launching (comma/semicolon separated, case-insensitive):

```
set IU_TRACE=SAVEPOINT
set IU_TRACE=SAVEPOINT,VESPLUME
set IU_TRACE=ALL
```

Unset = everything off (zero logging, pass-through hooks). `IU_VESPLUME_TRACE_PATH` still enables
the Vesplume hooks as before. Categories: `GENERAL`, `SAVEPOINT`, `VESPLUME`.

## Log location

`NTSC-U/logs/` (resolved by the app into `IU_TRACE_DIR`):

- `iu_trace.log` — all framework categories (`[ms since start] #seq tid=N CATEGORY message`).
- `vesplume_trace.log` — Vesplume diagnostics (legacy format).

Files are appended to; a session banner marks each run.

## Adding a trace category

1. In `iu_trace.h` add a bit to `enum Category` (e.g. `kMyThing = 1u << 3`).
2. Add a row to `kCategories` (`{"MYTHING", kMyThing}`).
3. Enable with `IU_TRACE=MYTHING`.

## Adding a trace point

In `src/iu_trace_points.h` (guest function `sub_XXXXXXXX` generated with `DEFINE_REX_FUNC`):

```cpp
extern "C" void __imp__sub_XXXXXXXX(PPCContext& ctx, uint8_t* base);
extern "C" void sub_XXXXXXXX(PPCContext& ctx, uint8_t* base) {
  using namespace iu::trace;
  if (!enabled(kMyThing)) { __imp__sub_XXXXXXXX(ctx, base); return; }
  const uint32_t lr = (uint32_t)ctx.lr;           // capture before the call
  emit(kMyThing, "ENTER r3=0x%08X LR=0x%08X", ctx.r3.u32, lr);
  regs(kMyThing, "ENTER", ctx);                    // r1,r3-r10,LR,CTR
  object(kMyThing, "ENTER", base, ctx.r3.u32);     // host ptr, vtable, slots
  field32(kMyThing, "ENTER", "x", base, ctx.r3.u32, 2968);
  __imp__sub_XXXXXXXX(ctx, base);                  // original body, exactly once
}
```

Only one override per guest function is possible (strong symbol wins over the weak alias).
Helpers: `host_ptr` (guest→host = `base + guest`), `readable`, `peek8/16/32`, `field32_str`.
For one-line traces from existing code: `IU_TRACE(kCat, "fmt", ...)`.

## Performance

- Disabled category: one cached mask test; no formatting or I/O.
- Enabled: `vsnprintf` into a preallocated 8192-slot ring (500 B/slot, ~4 MB), lock-free MPSC; file I/O
  only on a background thread (drains every ~50 ms).
- Ring full → records dropped and counted (`DROPPED` line); the game never blocks.
- Guest reads use `VirtualQuery` for safety; only on the enabled path. Avoid tracing very hot
  functions (per-frame/per-object) unless filtering inside the hook.
- Last ~50 ms of records may be lost on a hard crash; normal exit drains the ring.

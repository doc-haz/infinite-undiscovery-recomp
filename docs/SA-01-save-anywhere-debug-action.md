# SA-01 — "Save Anywhere" as a reusable Debug Menu action

**Status:** Static analysis + design only (`Phase SA-1`). Nothing is implemented, wired,
compiled, or runtime-validated in this phase.

**Evidence labels used in this document**

- `DEMOSTRADO` — confirmed from static source / generated code / SDK headers.
- `INFERENCIA` — reasonable interpretation of static structure, not yet proven at runtime.
- `NO DETERMINADO` — not resolved by static analysis alone; requires SA-2 instrumentation
  or live validation.

---

## 1. Objective and architectural requirements

The final feature is **not** a standalone F5 cheat. The deliverable is a reusable developer
action that a future in-game Debug Menu can invoke:

```
Debug Menu
  └── Save Anywhere
        └── invoke the game's normal save flow
```

Design constraints (fixed):

- Reuse the game's normal save flow, save UI / slot selection when feasible, and the
  existing Xbox 360 content save subsystem.
- Emit a normal, compatible Infinite Undiscovery save. No separate save format.
- No emulator-style savestates.
- No manual story/progression flag manipulation.
- No permanent coupling of the save backend to keyboard input.
- F5 is allowed **only** as a temporary Phase SA-2 validation frontend. Both F5 and the
  Debug Menu must call the same `SaveAnywhere()` action function.

---

## 2. Save backend (DEMOSTRADO)

The game saves through the Xbox 360 content API, imported in the generated IAT stubs in
`generated/default/`. This is the game's existing save system and the only backend the
feature may use.

Guest import thunks (`generated/default/infinite_undiscovery_init.cpp`):

| Import | Guest thunk address | Used via wrapper |
|---|---|---|
| `XamContentCreateEx` | `0x8296827C` | `sub_827DF970` |
| `XamContentClose` | `0x8296828C` | `sub_827DF9F8` |
| `XamContentSetThumbnail` | `0x8296829C` | `sub_827DFA00` |
| `XamContentGetCreator` | `0x829682AC` | `sub_827DFA08` |
| `XamContentCreateEnumerator` | `0x829682BC` | `sub_827DFA10` |
| `XamContentGetDeviceData` | `0x829682CC` | `sub_827DFA18` |
| `XamEnumerate` | `0x8296826C` | `sub_827DF958` |
| `XamUserGetSigninState` | `0x829682EC` | `sub_827DFC90` |
| `XamUserGetXUID` | `0x829681FC` | `sub_827DFC98` |
| `XamUserGetSigninInfo` | `0x8296830C` | `sub_827DFEA0` |
| `XamShowSigninUI` | `0x8296822C` | `sub_827DF918` |
| `XamShowDeviceSelectorUI` | `0x8296824C` | `sub_827DF930` |

SDK confirmation (`include/rex/system/xcontent.h`, `.../xam/content_manager.h`):

- `XContentType::kSavedGame = 1` is the save content type.
- `static_assert_size(XCONTENT_DATA, 0x134)` — `0x134 = 308` bytes, matching the
  308-byte slot stride used by the save list.

This confirms the design must target normal Xbox 360 saved-game content, not a custom
format.

---

## 3. Save service object (DEMOSTRADO addresses, INFERENCIA field names)

A singleton holder is referenced through `sub_825B8288`
(`generated/default/infinite_undiscovery_recomp.145.cpp:19171`), which simply returns
`[arg + 8]`.

- Holder global: `0x82A58EA8` (`lis r11,-32090; addi r3,r11,-29016`).
- Save service pointer: `[0x82A58EA8 + 8]` (the value `sub_825B8288` returns).

Observed service fields used by the save/menu code (INFERENCIA, by usage):

| Offset | Observed use | Evidence |
|---|---|---|
| `+60` | selected storage device id | `sub_824947E0`, `sub_824958A0` |
| `+64` | signed-in/ready byte (`== 1` means ready) | `sub_824943F0` |
| `+65..67` | sign-in/profile state flags | `sub_824994B8` |
| `+76` | user index passed to `Xam*` calls as `dwUserIndex` | multiple |
| `+104` | enumerator handle | `sub_82494B60`, `sub_82494C28` |
| `+108` | Xam result/status (`997` used as "pending/success" marker) | `sub_82494C28` |
| `+136` | entry count | `sub_82494B60` |
| `+140` | `XCONTENT_DATA` slot buffer (50 x 308 = 15400) | `sub_82494B60` |

The menu/save state machine caches the service pointer at menu-object offset `+17456`
(`sub_82145E08`, `sub_82494B60`, `sub_824958A0`).

---

## 4. Normal save flow (DEMOSTRADO structure)

The save subsystem is a multi-frame state machine, not a single synchronous call.

1. **Signed-in / device preflight**
   - `sub_824994B8` (`104.cpp:12088`): stores user index, calls
     `XamUserGetSigninState`; shows `XamShowSigninUI` when not signed in, otherwise
     reads `XamUserGetSigninInfo`.
   - `sub_824943F0` (`30.cpp:12522`): returns `1` when `service[+64] == 1`
     (profile ready gate).

2. **Save slot enumeration / list UI**
   - `sub_82494B60` (`28.cpp:11435`) and `sub_824947E0` (`91.cpp:14028`):
     `XamContentCreateEnumerator(content_type=1, count=50)` then
     `XamEnumerate` into a 15400-byte `XCONTENT_DATA` buffer.
   - `sub_82494C28` (`3.cpp:11643`): list state machine that drives the enumeration
     and transitions the UI state (`+15944`, `+15928`, etc.).

3. **Storage device selection**
   - `sub_82494D28` (`107.cpp:14066`): calls `XamShowDeviceSelectorUI`
     (`sub_827DF930`), then on success sets the save state.

4. **Save write / finalize**
   - `sub_824958A0` (`122.cpp:13954`): the save state machine. Statically observed to
     use `XamContentGetCreator` (`sub_827DFA08`), `XamContentSetThumbnail`
     (`sub_827DFA00`), `XamContentCreateEx` (via `sub_827DFAA0`), `XamContentClose`
     (`sub_827DF9F8`), and a 308-byte `XCONTENT_DATA` at `+17476`.
   - `sub_827DFAA0` (`118.cpp:24511`) is the create-content wrapper that forwards to
     `sub_827DF970` → `XamContentCreateEx`.

**Correction to an earlier handoff note:** `sub_8247A6D0` (`11.cpp:13674`) is a
content state machine too, but it enumerates content type `2`
(`XContentType::kMarketplaceContent`) with 10 entries — it is the DLC/marketplace
content cluster, **not** the saved-game routine. The saved-game path is the
content-type-1 cluster described above.

---

## 5. Proposed architecture

### 5.1 Reusable action API (host side)

Introduce one host-side action entry point; all frontends call only this:

```cpp
// Host-side design sketch (NOT implemented in SA-1).
enum class SaveAnywhereResult {
  kNotReady,      // profile/device/service predicate failed
  kUnsafeState,   // combat/cutscene/loading/async predicate failed
  kAlreadyBusy,   // a save/load flow is already in progress
  kInvoked,       // normal save flow accepted/invoked
};

SaveAnywhereResult SaveAnywhere();
```

`SaveAnywhere()` must:

- resolve the save service singleton from guest address `0x82A58EA8` (+8),
- evaluate the safety predicate (Section 6),
- if safe, invoke the **normal** guest save flow (Section 4), not a custom writer.

### 5.2 Guest invocation options

Two acceptable mechanisms exist in the SDK:

- **Option A — call the highest-level normal save trigger.** Preferred. Find the exact
  guest function the pause menu calls when the player selects "Save" and call it through
  `rex::ppc::GuestToHostFunction` (`include/rex/ppc/function.h`). This preserves the
  normal UI/slot-selection behavior for free.
- **Option B — drive the existing save state machine.** If the trigger is a per-frame
  state machine (`sub_824958A0`), set its start state and pump it once per game frame
  until it reaches its terminal/idle state. This is lower-level and requires locating the
  menu/save-manager object instance (open item in Section 8).

Both options must run on a guest-bound thread with a valid
`rex::runtime::ThreadState`/`current_kernel_state()`
(`include/rex/system/thread_state.h`); `GuestToHostFunction` already no-ops safely when
no thread state is bound, which is useful for guarding the temp hotkey.

### 5.3 Frontends (both call `SaveAnywhere()` only)

- **Primary:** Debug Menu entry `Debug Menu -> Save Anywhere`.
- **Temporary (Phase SA-2 only):** an F5 bind registered with
  `rex::ui::RegisterBind("debug_save_anywhere", "F5", "...", []{ SaveAnywhere(); })`
  and dispatched through `rex::ui::ProcessKeyEvent` (`include/rex/ui/keybinds.h`,
  `VirtualKey::kF5 = 0x74` in `include/rex/ui/virtual_key.h`).

The hotkey is test scaffolding; the save backend must not depend on it and the bind can
be removed without changing the Debug Menu path.

### 5.4 Async / multi-frame handling

The save flow is asynchronous in the guest. `SaveAnywhere()` is an **invocation**, not a
synchronous completion. The safety predicate must treat "save/load in progress" as
`kAlreadyBusy` so a second trigger does not overlap the first. Completion is observed via
the save state machine's own status fields (`+108`, `+15944`, `+15980`), not by waiting on
host timers.

---

## 6. Safety predicate specification

Proposed predicate:

```
CanSaveAnywhere() =
      IsServiceReady()
  AND IsDeviceReady()
  AND IsSaveIdle()
  AND IsGameStateSafe()
```

| Layer | Condition | Decision | Evidence status |
|---|---|---|---|
| Profile | signed-in profile available (`service[+64] == 1`; `XamUserGetSigninState`) | required | `DEMOSTRADO` (`sub_824943F0`, `sub_824994B8`) |
| Device | selected storage device present (`XamContentGetDeviceData` success) | required | `DEMOSTRADO` (`sub_82495348`, `sub_82495FB0`) |
| Service | save service singleton pointer and `service[+76]` user index valid | required | `DEMOSTRADO` addresses / `INFERENCIA` validity check |
| Idle | no enumerator handle (`+104 == 0`), no `997` pending state, save/menu state machine idle (`+15944`/`+15980`) | required | `INFERENCIA` (exact idle encoding needs SA-2) |
| Exploration | normal player-controlled exploration | **enable** | `NO DETERMINADO` — game-mode flag not yet located |
| Cutscene / scripted event | in a scripted sequence or cutscene | **disable** | `NO DETERMINADO` — script/scene-mode flag not yet located |
| Combat | in combat and save is unsafe | **disable** | `NO DETERMINADO` — battle-state flag not yet located |
| Map transition / loading | loading or transitioning | **disable** | `NO DETERMINADO` — load/transition flag not yet located |
| Async save/load | a save or load operation is already running | **disable** | `INFERENCIA` via save state fields |

The four `NO DETERMINADO` rows are the main remaining static gap. They must be derived
from the same state the game itself uses to grey out its normal Save option, so the Debug
Menu matches the game's own rules instead of inventing new ones.

---

## 7. Phase SA-2 implementation plan (not executed now)

1. Locate the exact guest trigger the normal pause menu uses for "Save" (resolve Option A
   vs Option B in Section 5.2).
2. Locate the save-menu manager object instance the guest functions expect (`r3`).
3. Resolve the game-state flags for cutscene/combat/loading/async and encode
   `IsGameStateSafe()`.
4. Implement `SaveAnywhere()` and the temporary F5 bind; keep the backend
   input-agnostic.
5. Validate with before/after measurements and the normal slot UI:
   - F5 during exploration opens the normal save flow;
   - F5 during a cutscene/combat/loading is a no-op (safety predicate);
   - the produced save is a normal, compatible save (loadable through the normal Load
     flow), not a custom file.

---

## 8. Open questions / risks

- **Save-menu object instance:** where the guest obtains the `r3` save/menu-manager object
  is not yet identified; it must be found before Option B can be used.
- **Normal trigger vs state machine:** if the pause menu invokes a UI handler rather than
  `sub_824958A0` directly, Option A should target that handler.
- **Exact game-mode flags:** cutscene/combat/loading predicates are `NO DETERMINADO` and
  must be sourced from the game's own state, never guessed.
- **Reentrancy:** the async save state machine must be guarded against overlapping
  invocations.
- **Generated files:** do not hand-edit `generated/default/`; code changes belong in
  host source (`src/`) or a new host-side module.

---

## 9. Non-goals

- No separate save format or savestate.
- No direct story/progression flag writes.
- No permanent F5 behavior.
- No attempt to replace the normal save UI with a host-side custom dialog unless the game
  itself has no recoverable save UI.

## Resumen (ES)

- **Solo análisis estático (SA-1); no se implementa nada.**
- Guardar reutiliza el subsistema Xbox 360 de contenido (`XamContent*`) ya importado por
  el juego.
- Se identificaron el singleton del servicio de guardado (`0x82A58EA8` + 8), el flujo
  normal de guardado (`sub_82494B60`/`sub_824947E0` para listar,
  `sub_82494D28` para elegir dispositivo y `sub_824958A0` para escribir/cerrar) y los
  predicados de perfil/dispositivo (`sub_824943F0`, `sub_82495348`).
- La acción reutilizable `SaveAnywhere()` es la única entrada; el menú Debug y una
  tecla temporal F5 (solo SA-2) llaman a la misma acción.
- Predicado de seguridad en capas: perfil listo, dispositivo presente, guardado en
  reposo y estado de juego seguro (exploración). Combate/cinemática/carga quedan como
  `NO DETERMINADO` y deben tomarse del estado que el propio juego usa para su opción
  normal de guardar.

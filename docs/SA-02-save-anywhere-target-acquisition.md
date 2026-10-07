# SA-02 — "Save Anywhere" Target Acquisition (static analysis only)

**Status:** `Phase SA-2` — static analysis only. Nothing is implemented, wired,
compiled, or runtime-validated. No source, generated, or save data was modified.

**Evidence labels**

- `VERIFIED` — proven from generated guest C++ / the decrypted flat guest image
  (`extract/disc1/default.exe`, guest base `0x82000000`) / SDK headers.
- `INFERRED` — consistent static interpretation, not yet runtime-proven.
- `UNKNOWN` — not resolvable from static analysis alone; needs SA-3 runtime capture.

This document continues directly from `docs/SA-01-save-anywhere-debug-action.md`.
SA-1 findings are reused, not reinterpreted. One SA-1 inference is upgraded below
(see A.1 / C.1).

---

## A. Exact normal Save trigger

### A.1 Singleton correction (upgrade of an SA-1 inference)

SA-1 called `[0x82A58EA8 + 8]` the "save service". That is now `VERIFIED` to be the
**18144-byte save/menu manager**:

- `sub_82497B80` (`infinite_undiscovery_recomp.98.cpp:12350-12490`) does
  `r3 = 18144`, allocates via `sub_821D1088`, constructs it with `sub_82495468`,
  and stores the result at `[holder + 8]` (`stw r11,8(r30)`).
- `sub_82495468` (`128.cpp:13727`) is the manager constructor (size 18144; initializes
  state-machine fields `+15928..+16200`, `+17440..+17472`, and two 308-byte
  `XCONTENT_DATA` buffers at `+17476` and `+17784`).
- `sub_825B8288` (`145.cpp:19171`) is literally `return [arg + 8]` and is the accessor
  used by the menu code to reach the manager.

Separately, `sub_82479600` (`71.cpp:12646`) returns `[holder + 12]`. The menu object
stores that value into its own `+264` (`sub_82482468`, `82.cpp:12656`). Therefore
`[holder + 12]` is the **content-save service object**, and `menu+264` is a cached
copy of it. SA-1's "`service +8`" wording should now read "manager at `+8`".

### A.2 The highest-level trigger the normal flow reaches

| Level | Function | Generated symbol | File / line | Role |
|---|---|---|---|---|
| Open save UI | `sub_824858E8` | `sub_824858E8` | `145.cpp:13997` | creates menu, sets mode, opens normal save UI |
| Create menu | `sub_82475670` | `sub_82475670` | `2.cpp:14167` | allocates 12856-byte menu, registers it |
| Menu init | `sub_82482468` | `sub_82482468` | `82.cpp:12656` | installs vtable `0x820390B4`, zeroes state |
| Menu dispatcher | `sub_82485188` | `sub_82485188` | `108.cpp:14464` | per-frame state machine (cases 0..17) |
| Slot execute | `sub_824803C8` | `sub_824803C8` | `99.cpp:13001` | serialize + stage actual write |
| Serialize | `sub_8247FF00` | `sub_8247FF00` | `16.cpp:13328` | builds the save metadata/payload |
| Stage | `sub_82494A18` | `sub_82494A18` | `126.cpp:13873` | copies slot/payload into manager, starts state |
| Write/close | `sub_824958A0` | `sub_824958A0` | `122.cpp:13954` | `XamContent*` create/thumbnail/close |

`sub_824858E8` is the normal "open the Save/Load window" trigger. It is **not** reached
by any direct `bl` in the recompiled C++: it is virtual (see B), so the caller is a
guest vtable dispatch, resolved at runtime through the object's vtable pointer.

Two sibling "create menu with a concrete mode" entry points also exist and are the
more likely true gameplay callers of the save window:

- `sub_826380B8` (`1.cpp:19612`) — creates the menu and sets mode from a small context
  object: `menu+12592 = 1` if `[ctx+40] != 0`, else `2`; copies `[ctx+41]` to
  `menu+12622` and `[ctx+36]` to `menu+12836`.
- `sub_8263CBB8` (`81.cpp:21945`) — hardcodes `menu+12592 = 1` (save), then
  `sub_8247CC38(holder+404, 1)`, `sub_825B8288(holder)` and `sub_82499198(manager, 0)`.

Mode `+12592 = 1` is the save mode (`INFERRED` direction, strongly supported because
`sub_8263CBB8` sets `1` and the slot-execute path it feeds is the serializer + writer).

### A.3 Whether it opens the normal slot UI and requires a save-point actor

- `VERIFIED` — the trigger opens the **normal** slot-selection UI: it allocates the same
  12856-byte window object and drives `sub_82485188`, whose cases `12` and `17` start
  save-list enumeration (manager `+15980 = 1`, `+15984 = 2048`) and case `11` starts
  device selection (`sub_824808F0`).
- `VERIFIED` — no save-point actor is required by the write path itself. The serializer
  reads live **map state** from a type-201 registry object (`sub_82147140(reg, 201, 20)`,
  then `u16 [obj+380]` = map id) and a world-state flag `[obj+5524]&1`
  (`sub_8215FB78`). A save-point actor is not dereferenced in the payload builder.
- `INFERRED` — the normal UI *callers* gate the trigger with their own scene context, but
  the trigger function itself does not enforce a save-point requirement.

---

## B. Call chain from UI / save point to backend

`VERIFIED` structure (function addresses and the only recompiled call sites):

```
[gameplay scene / task object]            (UNKNOWN exact identity)
   -> sub_824858E8                        vtable slot 7  (opens Save/Load window)
        -> sub_824D1CC0(self)             validates/guards (always returns 1)
        -> sub_82475670(holder, f1, f2, 0, 0)
             -> sub_824751B8               alloc 12856-byte window
             -> sub_8248D268               registers window in type registry
             -> sub_82482468               menu constructor (vtable 0x820390B4)
        -> menu+12592 = mode, menu+12845 = 1
        -> sub_8219CAF0(menu, 1)
        -> manager = sub_825B8288(0x82A58EA8)
        -> sub_82499198(manager, 0)        (no-op when r4==0)
        -> self+264 = 2

[per frame]
   -> sub_82485188(menu)                   dispatcher, vtable slot 25
        top gate: sub_82161940([0x82895998]) == 1 -> skip
        case 12 / 17: enumerate save list   (manager +15980=1, +15984=2048)
        case 11: sub_824808F0               (device selection start)
        slot confirmed -> sub_824803C8(menu)
             -> sub_8247FF00(menu)          serialize payload
             -> sub_82494A18(menu+264, slot, payload, key, aux, meta)
                  -> sub_82494520 / sub_82494720 / sub_82494778
                       -> sub_824958A0       XamContentGetCreator / SetThumbnail /
                                             XamContentCreateEx / XamContentClose
```

Device-selector UI entry (`sub_82494D28`, `107.cpp:14066`) is reached from
`sub_824808F0`/`sub_824958A0` (`92.cpp:604`, `21.cpp:12603`).

`VERIFIED` exclusivity: the writer entry points have **no** recompiled callers outside
this menu flow — `sub_824803C8` only at `108.cpp:14628/14840`; `sub_82494A18` only at
`99.cpp:13048`; `sub_824808F0` only at `108.cpp:15091/15204`.

---

## C. r3 object identity and lifetime

### C.1 The menu/window object (the object the dispatcher expects)

`VERIFIED`:

- The Save/Load window is a **12856-byte object**; its vtable is `0x820390B4`
  (`sub_82482468` stores `lis -32252 + addi -28492`).
- The vtable's slot `-1` (`0x820390B0`) holds typeinfo pointer `0x820F2588`; slot 25
  (`0x82039118`) is `sub_82485188` (the dispatcher).
- The window is **not** a global singleton: it is heap-allocated on demand by
  `sub_82475670`, then registered in the type registry (`0x828380F8`) via
  `sub_8248D268`. A new key/instance is produced each time the menu opens.
- RTTI class names present in the flat image (`extract/disc1/default.exe`):
  `CSaveFile`, `CSaveLoadWnd`, `CCampSaveLoadWnd`, `CMenuSaveLoadTask`,
  `CSavePointManager`, `CSavePointObject`, `CSaveTarget`.

`INFERRED` (high confidence):

- The 12856-byte window is `CSaveLoadWnd` (or its `CCampSaveLoadWnd` subclass for camp
  saves; the camp path is the `menu+12622 == 1` branch that forces map id `4460`).
- The vtable-`0x82039494` class whose slot 7 is `sub_824858E8` is a menu/task driver
  (`CMenuSaveLoadTask` or a related scene task), constructed by `sub_82485850`
  (`147.cpp:13710`). Its `r3` fields used by the trigger are:
  `+16` (data passed to a vtable call), `+40` (a registry type id, value `7`),
  `+264` (window-open state, set to `2`), `+280` (type id passed to registry lookups),
  `+332` (menu mode copied to `menu+12592`).

`UNKNOWN`:

- The exact C++ class name of the vtable-`0x82039494` object (not proven against RTTI).
- Where the live instance of that object is stored (no global pointer was found
  statically; it is reached through registry lookups of type `7` / type `201`).

### C.2 Lifetime

`VERIFIED`:

- The **manager** (`[0x82A58EA8 + 8]`, 18144 bytes) is persistent — allocated once during
  init (`sub_82497B80`) and cached for the session.
- The **window** (12856 bytes) exists only while the Save/Load UI is open; it is
  re-created per open (`sub_82475670`) and torn down by `sub_82481560`
  (`33.cpp:11062`) on exit/cancel.

`INFERRED` — the Debug Menu can obtain the manager trivially (`[0x82A58EA8 + 8]`), but
must either (a) locate the live window/task object or (b) create a window via
`sub_82475670` to invoke the normal flow.

---

## D. Required arguments / globals

`VERIFIED` for the write path (`sub_824803C8` -> `sub_82494A18`):

- `r3` = `menu+264` = content-save service (`[0x82A58EA8 + 12]`).
- `r4` = `menu+12820` = selected slot index.
- `r5` = `menu+12628` = serialized payload buffer.
- `r6` = `menu+12632` = type-201 live-map object key/type.
- `r7` = constant `0x8283D648` (`lis -32252, addi -29304`) — a static table pointer.
- `r8` = `menu+12776` = auxiliary metadata buffer (42 bytes, zeroed).
- `r9` = `menu+12648` = auxiliary metadata buffer (128 bytes, zeroed).

`sub_82494A18` then marshals (`126.cpp:13873`): `manager+15964 = slot`,
`manager+15968/17440 = payload ptr`, `manager+15972/17444 = payload size`,
`manager+15944 = 64` (unless already `16`), then `sub_82494520/4720/4778`.

Key globals (`VERIFIED` addresses):

| Global | Meaning |
|---|---|
| `0x82A58EA8` | content/save singleton holder (`+8` manager, `+12` service, `+20`, `+68`, `+88`) |
| `0x828380F8` | type registry; `+24` = table of live typed objects; `+76` = allocator arena |
| `0x82895998` | pointer to an object whose `+60 == 1` blocks the menu dispatcher |
| `0x829...` (type 201) | live world/map state object (map id at `+380`, save flag at `+5524`) |

---

## E. Native save gating / safety predicates

`VERIFIED` gates observed in the normal flow:

1. **Profile ready** — `sub_824943F0` returns `1` iff `service[+64] == 1`
   (`30.cpp:12522`). SA-1.
2. **Device ready** — `sub_82495348` (`XamContentGetDeviceData` success). SA-1.
3. **Menu dispatcher block** — `sub_82485188` top gate
   (`108.cpp:14484-14497`): if `sub_82161940([0x82895998])` returns `1`, the whole
   per-frame dispatch is skipped. `sub_82161940` (`76.cpp:202`) returns `1` iff
   `[obj + 60] == 1`. So **`[[0x82895998] + 60] == 1` means "menu disabled"**.
4. **Serializer readiness** — `sub_82498E70` (`136.cpp:12612`) is the payload-header
   builder (writes magic `0x55445356`, type `51`), guarded by `sub_826D1000(meta, 0x64000)`.
   It is a buffer/format check, **not** a game-mode gate.
5. **World-state flag** — `sub_8215FB78` (`39.cpp:177`) returns
   `[type201_obj + 5524] & 1`. This is the only "where am I" flag read by the payload
   builder; it selects which checkpoint record is copied (see F). It does **not**
   block the save.

`INFERRED` semantic mapping:

- `[[0x82895998]+60] == 1` is most likely a "UI/input blocked" state (busy, transition,
  cutscene). It is the closest existing analog to the required
  `cutscene/transition/menu` disable predicate.
- `[type201_obj + 5524] & 1` distinguishes "at a legitimate save context" from
  "elsewhere"; its exact meaning needs runtime correlation.

`UNKNOWN` (still the SA-1 `NO DETERMINADO` rows — no statically-proven flag):

- explicit **combat** flag,
- explicit **cutscene/scripted-event** flag,
- explicit **map transition/loading** flag,
- explicit **async save/load in progress** flag (the manager state fields
  `+15944/+15980/+15984` are the candidates, but their exact idle encoding is not
  proven statically).

---

## F. Position / map serialization behavior

`VERIFIED` from `sub_8247FF00` (`16.cpp:13328`):

- The payload (`menu+12628`) stores:
  - `+44` = **map id** (`u16 [type201_obj + 380]`), forced to the constant `4460`
    when `menu+12622 == 1` (camp save path).
  - `+72` = a **16-byte checkpoint/location record** copied from a **static table**
    (base `0x82A59128`, offset `+0x3C564`) when `sub_8215FB78()` is true; otherwise
    only **4 bytes** from `sub_8247A3F0(...)+8`.
  - `+68` = **progress bitmask** (loop over `sub_8219FFE0`).
  - `+16` = slot+1, `+28`/`+32` = map/checkpoint pointers, `+36` = `[0x82A59128+152]`,
    `+48` = `sub_8218AE60(holder)` (clock/time), `+96`, `+100`, `+108` = flags.
- `sub_8247A3F0` (`58.cpp:13146`) is pure pointer arithmetic
  (`r3 + r4*296 + 0x25968`) into a static, 296-byte-stride table — it is **not** a live
  coordinate read.
- `menu+12648` (128 bytes) and `menu+12776` (42 bytes) are **zeroed** and passed only as
  auxiliary metadata buffers; no float X/Y/Z is written anywhere in the builder.

Conclusion (`VERIFIED`): **no live player coordinates are serialized.** The save records
`map id` + `checkpoint/location table index` + `progress flags`.

Answer to the position question:

- `A` exact coordinates — **ruled out** (`VERIFIED`, no coordinate capture).
- `B` current map + predefined spawn/checkpoint — **most consistent** with the evidence
  (`INFERRED`, high confidence: map id is live, but the spawn record is a static table
  entry).
- `C` most recent legitimate save point — plausible for the non-save-point branch
  (`INFERRED`).
- `UNKNOWN` — whether the `sub_8215FB78()==0` branch (4-byte record) yields "map default
  spawn", "last checkpoint", or "invalid location". This is the single most important
  item to resolve in SA-3.

---

## G. Existing hidden / debug save path

- `VERIFIED` (RTTI strings): `CPlayerInput_DebugCtrl`, `CDebugOutputTask`,
  `CDebugOutputTask` exist in the guest, so a debug input/controller infrastructure
  exists.
- `VERIFIED` (call-graph): the save writer entry points (`sub_824803C8`,
  `sub_82494A18`, `sub_824808F0`) have recompiled callers **only** inside the normal
  Save/Load menu flow; `sub_824858E8` has no direct recompiled caller (vtable only).
- `INFERRED` — there is no statically-visible alternate/debug save command; any hidden
  debug save would still have to route through the same window/stage functions.
- `UNKNOWN` — whether `CPlayerInput_DebugCtrl` exposes a save command at runtime
  (no string/table evidence found statically).

---

## H. Recommended future `SaveAnywhere()` call architecture

**Recommendation: A, with one runtime precondition.**

`SaveAnywhere()` must ultimately drive the game's own window, i.e. create the
`CSaveLoadWnd` object and run `sub_82485188` per frame until the slot UI is shown.
Preferred shape:

```
SaveAnywhere():
  1. manager = sub_825B8288(0x82A58EA8)          // [0x82A58EA8 + 8]
  2. if manager == 0 -> kNotReady
  3. service = [0x82A58EA8 + 12]
  4. CanSaveAnywhere() = IsProfileReady() && IsDeviceReady()
                          && !IsMenuBlocked()      // [[0x82895998]+60] != 1
                          && !IsSaveBusy()          // manager state fields
                          && IsGameStateSafe()      // SA-3 to resolve exact flags
  5. if !safe -> kUnsafeState
  6. menu = sub_82475670(holder, f1, f2, 0, 0)      // normal window creation
  7. menu+12592 = 1 (save); menu+12845 = 1
  8. sub_8219CAF0(menu, 1)
  9. sub_82499198(manager, 0)
 10. each game frame while window open: sub_82485188(menu)   // normal dispatcher
 11. return kInvoked
```

Rationale:

- `A` (invoke the game's high-level normal Save trigger) is technically sound and
  preserves the real slot UI for free.
- The **manager** is a persistent singleton and is the stable anchor
  (`VERIFIED`). The **window** is created on demand, so the clean callable action is
  "create the normal window in save mode and pump the normal dispatcher" — this is `A`
  at the window level, using only `VERIFIED` entry points.
- The one remaining precondition is the live scene/task object: `sub_824858E8` and
  `sub_8263CBB8` both expect a polymorphic parent whose exact instance is not reachable
  from a static global. Creating the window directly via `sub_82475670` avoids that
  dependency, but we must confirm at runtime that a window created outside a scene
  still populates `menu+268/+280/+328` (the registry type ids the dispatcher uses).
- `B` (drive the low-level `+15980/+15984` state machine directly) is a fallback only;
  it bypasses the normal UI and is more fragile.

`SaveAnywhere()` must run on a guest-bound thread with a valid
`rex::runtime::ThreadState`; `rex::ppc::GuestToHostFunction` no-ops safely when no
thread is bound (temp-F5 guard).

---

## I. Minimal SA-3 runtime experiment (not implemented here)

Goal: resolve the two blocking unknowns before implementation.

1. **Capture the live `r3` at the normal Save trigger.**
   - Set a host-side breakpoint/log on `sub_824858E8` entry during a real save-point
     interaction. Record `r3`, then dump `[r3+40]`, `[r3+16]`, `[r3+280]`, `[r3+332]`,
     `[r3+264]` and the object's vtable pointer.
   - This identifies the owning class and its live storage, converting C/UNKNOWN into
     VERIFIED.
2. **Resolve position behavior.**
   - Create a save via the normal flow at an arbitrary free-roam position (and a second
     one at a legit save point), then load each and record the restored map id and
     player position. Correlate with payload `+44` (map id) and `+72` (checkpoint
     record) to decide between option B and C.
3. **Resolve game-mode flags.**
   - Instrument `sub_82161940`/`[0x82895998]+60` and `sub_8215FB78`/`[type201+5524]`
     during exploration, combat, cutscene, and map load. Capture the manager state
     fields `+15944/+15980/+15984` before/during a save to derive `IsSaveBusy()` and
     the exact "safe state" predicate.

All three are read-only observation; none modify game state.

---

## J. Remaining unknowns

- Exact class name and live storage of the vtable-`0x82039494` task object (`r3` of
  `sub_824858E8`).
- Whether a directly-created window (no scene parent) fully populates
  `menu+268/+280/+328` and opens the slot UI.
- Semantic meaning of `[[0x82895998]+60] == 1` and `[type201+5524]&1`.
- Explicit combat / cutscene / map-transition / async-save flags (still the SA-1
  `NO DETERMINADO` rows).
- Whether `CPlayerInput_DebugCtrl` already contains a save command.
- Exact position-restore behavior (option B vs C) for a Save-Anywhere save.

---

## K. Git integrity before / after

Start of SA-2:

```
HEAD = 11f58cef39beccd52ef0764f1895dbfe7945ac11  (main)
 M src/infinite_undiscovery_app.h
 M src/main.cpp
?? docs/SA-01-save-anywhere-debug-action.md
?? src/vesplume_diag.h
```

End of SA-2:

```
HEAD = 11f58cef39beccd52ef0764f1895dbfe7945ac11  (main)   (unchanged)
 M src/infinite_undiscovery_app.h                          (unchanged, pre-existing)
 M src/main.cpp                                            (unchanged, pre-existing)
?? docs/SA-01-save-anywhere-debug-action.md                (unchanged, pre-existing)
?? src/vesplume_diag.h                                     (unchanged, pre-existing)
?? docs/SA-02-save-anywhere-target-acquisition.md          (new, this deliverable)
```

Only the SA-02 deliverable was added. No source, generated, save, or Vesplume
diagnostic files were modified during SA-2.

---

## Resumen (ES)

- **Solo análisis estático (SA-2); no se implementa nada.**
- El trigger normal de guardado es la ventana `CSaveLoadWnd` (objeto de 12856 bytes),
  creada por `sub_82475670` y gobernada por el despachador `sub_82485188` (slot 25 de
  su vtable `0x820390B4`). `sub_824858E8` abre la ventana desde la escena (slot 7 de
  la vtable `0x82039494`).
- Se corrige/actualiza SA-1: `[0x82A58EA8 + 8]` es el **manager de guardado** de
  18144 bytes (persistente), y `[+12]` es el servicio de contenido (cacheado en
  `menu+264`).
- La cadena normal es: ventana → `sub_82485188` → `sub_824803C8` →
  `sub_8247FF00` (serializar) → `sub_82494A18` → `sub_824958A0` (`XamContent*`).
- El save serializa **map id + índice de checkpoint + flags de progreso**, **no**
  coordenadas vivas. Cargar un guardado "en cualquier lugar" restauraría mapa + spawn
  predefinido (opción B) o el último punto de guardado (opción C); la opción A
  (coordenadas exactas) queda descartada.
- Gating nativo parcial: perfil listo, dispositivo listo, y bloqueo de UI
  (`[[0x82895998]+60]==1`). Combate/cinemática/carga/async siguen `NO DETERMINADO`.
- Arquitectura recomendada: **A** — crear la ventana normal en modo guardar y bombear
  el despachador normal; con un experimento SA-3 para capturar el `r3` vivo y resolver
  el comportamiento de posición.

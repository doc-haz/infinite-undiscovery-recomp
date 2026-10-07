# SA-03 — Save Anywhere Runtime Entry & Safe Invocation

**Verdict:** `NOT READY FOR SA-4`

**Single isolated blocker:** there is no statically proven acquisition path to the live
object that `sub_824858E8` expects in `r3`, and direct construction of the save window
with `sub_82475670` does not populate the window fields that the normal dispatcher
requires to reach the serializer. One small runtime capture (SA-3B) is sufficient to
resolve this without implementing Save Anywhere.

**Status:** Static + code-relationship analysis only. No source, generated, save, or
Vesplume diagnostic files were modified. The only repository change is this report.

Evidence labels:

- `VERIFIED` — direct evidence from the generated guest C++ or the guest flat image.
- `INFERRED` — consistent static interpretation, not runtime-proven.
- `UNKNOWN` — requires dynamic validation.

---

## A. Executive Summary

The normal save architecture is already well understood from SA-1/SA-2:

- Save window: 12856-byte `CSaveLoadWnd`, vtable `0x820390B4`.
- Dispatcher: `sub_82485188` (vtable slot 25).
- Scene-side opener: `sub_824858E8` (slot 7 of `CCampSaveLoadWnd`, vtable
  `0x82039494`).
- Persistent save/menu manager: `[0x82A58EA8 + 8]` (18144 bytes).
- Content-save service: `[0x82A58EA8 + 12]`, cached at window `+264`.
- Normal chain: window -> `sub_82485188` -> `sub_824803C8` -> `sub_8247FF00`
  (serialize) -> `sub_82494A18` -> `sub_824958A0` (XamContent write/close).

SA-3 confirms the preferred architecture remains **A** (invoke the highest-level normal
trigger and let the original UI/dispatcher/service stack run). However, SA-3 also
established that the recommended trigger has an unresolved runtime precondition: the
debug action would have to obtain a live `CCampSaveLoadWnd` owner object, and no static
global/handle for that object was found.

The fallback of constructing a `CSaveLoadWnd` directly is **not** currently safe:

- `sub_82475670` allocates and registers the window but does not populate the
  widget-registry ids at `+268`, `+272`, `+280`, `+284`, `+312`, `+316`, `+320`,
  `+328`.
- `sub_82485188` resolves those ids through type-7 registry lookups and cannot reach
  `sub_824803C8` without them.
- A candidate layout builder (`sub_82482468`'s owner-side counterpart, vtable slot 0 of
  `0x8203CD70`, function `sub_824CC080`) populates those ids, but its exact
  construction/call relationship to the normal flow is not yet proven.

Therefore SA-4 must not guess object pointers or initialization sequences.

---

## B. Evidence Baseline

### B.1 Global address corrections

SA-02 contains two apparent global-address typos. The generated `lis/addi` pairs decode
to:

- Dispatcher guard global: `0x82A95998` (`lis r11,-32087; lwz r11,22936(r11)`).
- Type-registry base: `0x82A380F8` (`lis r11,-32092; addi r11,r11,-32520`).
- Save/menu holder: `0x82A58EA8` (`lis r11,-32090; addi r11,r11,-29016`).

`VERIFIED` — the holder and its `+8`/`+12` fields remain correct. SA-02's
`0x82895998` should read `0x82A95998`, and `0x828380F8` should read `0x82A380F8`.

### B.2 Verified function locations

| Function | Generated file:line |
|---|---|
| `sub_82475670` | `generated/default/infinite_undiscovery_recomp.2.cpp:14168` |
| `sub_824751B8` | `generated/default/infinite_undiscovery_recomp.36.cpp:13328` |
| `sub_82482468` | `generated/default/infinite_undiscovery_recomp.82.cpp:12656` |
| `sub_8248D268` | `generated/default/infinite_undiscovery_recomp.41.cpp:13183` |
| `sub_82485188` | `generated/default/infinite_undiscovery_recomp.108.cpp:14464` |
| `sub_824858E8` | `generated/default/infinite_undiscovery_recomp.145.cpp:13997` |
| `sub_82485850` | `generated/default/infinite_undiscovery_recomp.147.cpp:13712` |
| `sub_824D1CC0` | `generated/default/infinite_undiscovery_recomp.100.cpp:14725` |
| `sub_82161940` | `generated/default/infinite_undiscovery_recomp.76.cpp:202` |
| `sub_8247CFC0` | `generated/default/infinite_undiscovery_recomp.111.cpp:13234` |
| `sub_8247CF58` | `generated/default/infinite_undiscovery_recomp.3.cpp:11218` |
| `sub_824CA100` | `generated/default/infinite_undiscovery_recomp.15.cpp:14357` |
| `sub_824CB090` | `generated/default/infinite_undiscovery_recomp.31.cpp:14981` |
| `sub_824CB2D0` | `generated/default/infinite_undiscovery_recomp.69.cpp:14751` |
| `sub_824CC080` | `generated/default/infinite_undiscovery_recomp.82.cpp:14072` |

---

## C. Live Scene/Object Acquisition

### C.1 What `sub_824858E8` expects

`VERIFIED`

- `r3` is an object whose vtable slot 7 is `sub_824858E8`; that vtable is
  `0x82039494`, i.e. `CCampSaveLoadWnd`.
- `sub_824858E8` stores `r3` to `r31` and reads:
  - `[owner + 16]` as the extra id passed to `sub_82475670`;
  - `[owner + 332]` as the save/load mode written to `window + 12592`;
  - `[owner + 264]` as an owner state that is set to `2` if it was `< 7`.
- `sub_824858E8` calls `sub_824D1CC0(owner)`, but that call is not a true rejection
  gate: `sub_824D1CC0` performs a type-7 lookup and an indirect call, then returns `1`
  unconditionally.

`VERIFIED` — `sub_824858E8` itself does not enforce a physical save-point actor.

### C.2 Owner construction path

`VERIFIED`

- `sub_82485850` calls `sub_82483388` then stores the `0x82039494` vtable pointer at
  `[obj + 0]`. This is the `CCampSaveLoadWnd` constructor.
- `sub_824C8DA0` allocates **336 bytes**, constructs with `sub_82485850`, and stores the
  result through a wrapper.
- `sub_824CA100` wraps that allocation/construction with the same registration pattern
  used by the save-window creator: `sub_8248D268`, `sub_826D1F20`, and
  `sub_8219BCD0`.
- `sub_824CB090` calls `sub_824CA100`, then sets `[newobj + 276] = 8`,
  `[newobj + 280] = 43`, and calls `sub_826D7E08`.
- `sub_824CB090` is reached from `sub_824CB2D0` case 7.

### C.3 Acquisition

`UNKNOWN`

No static global or registry handle to a live `CCampSaveLoadWnd` instance was found in
the paths inspected. The instance is created by a scene/task dispatcher
(`sub_824CB2D0`), but the object identity and storage of that dispatcher's `r3` are not
statically resolved.

This is the primary blocker. A debug action cannot safely call `sub_824858E8` until it
knows how to obtain this live object.

---

## D. Save Window Creation and Registration

### D.1 `sub_82475670` / `sub_824751B8` / `sub_82482468`

`VERIFIED`

- `sub_82475670(r3=stack_wrapper, f1, f2, r6=extra_id, r7=0)`:
  - calls `sub_824751B8`, which allocates **12856** bytes and constructs via
    `sub_82482468`;
  - calls `sub_8248D268` to run vtable slot 1 (registration) and, on failure, slot 3;
  - if the window exists, calls `sub_826D1F20(window, extra_id)` and
    `sub_8219BCD0(window, f1, f2, 0)`.
- `sub_82482468` installs vtable `0x820390B4` and **zeroes** the widget-id slots:
  `+264`, `+268`, `+272`, `+276`, `+280`, `+284`, `+292..328`.

### D.2 Why direct construction is not sufficient

`VERIFIED`

`sub_82485188` (the dispatcher) resolves type-7 registry ids from:

- case 0: `+268` plus the active-command object;
- other cases: `+280`, `+284`, `+312`, `+316`, `+320`, `+328`.

The normal save execute path (`sub_824803C8`) is reached only after those lookups
succeed and the active command object passes the `sub_826D9228` / `+412 == 0` /
`found+480*240+604` checks.

Because `sub_82475670` alone leaves those ids zero, a directly-created window will not
reach the serializer without additional layout/registration work.

### D.3 Candidate layout builder

`INFERRED`

`sub_824CC080` is slot 0 of vtable `0x8203CD70`. It creates widgets via
`sub_826D5E98` and stores the returned ids into `+268`, `+272`, `+280`, `+284`, and
related slots. This matches the missing initialization. However, the exact call
relationship between this builder and the normal window path is not yet proven, and the
owning object for vtable `0x8203CD70` is `UNKNOWN`.

---

## E. Full Normal Save Entry Chain

`VERIFIED` normal chain:

```
scene/task dispatcher (identity UNKNOWN)
  -> sub_824CB2D0            dispatch command id in r4
      -> case 7: sub_824CB090
          -> sub_824CA100    allocate+construct CCampSaveLoadWnd (336 bytes)
              -> sub_824C8DA0 -> sub_82485850
          -> sub_824826.../registration helpers
  -> (later, normal save command) sub_824858E8 (CCampSaveLoadWnd vtable slot 7)
      -> sub_82475670        create CSaveLoadWnd (12856 bytes)
      -> window+12592 = owner+332 (save/load mode)
      -> sub_8219CAF0(window,1)
      -> manager sub_82499198(manager,0)
      -> sub_82485188        dispatcher
          -> sub_824803C8    save execute
              -> sub_8247FF00 serialize
              -> sub_82494A18 stage
              -> sub_824958A0 XamContent write/close
```

The highest useful reusable function remains `sub_824858E8`, but only if a live owner is
available. The next-highest original creator is `sub_824CA100`/`sub_824CB090`, but they
are reached through a dispatcher whose owner acquisition is `UNKNOWN`.

---

## F. Save Safety Predicates

### F.1 Dispatcher top gate

`VERIFIED`

- `sub_82485188` loads `[[0x82A95998]]`.
- If non-zero, it calls `sub_82161940(obj)`.
- `sub_82161940` returns `1` exactly when `[obj + 60] == 1`.
- If that returns `1`, the dispatcher exits without processing the window.

`INFERRED` — this is a global UI/world block object. It is allocated by
`sub_8247CFC0` (72 bytes), constructed by `sub_8247CF58` (which zeroes `+60` and sets
`+53`/`+68`), and registered via `sub_826CE980`.

`UNKNOWN` — the exact condition that sets `[0x82A95998-object + 60] = 1`, and therefore
the semantic meaning of the gate, was not traced.

### F.2 Other conditions

| Condition | Evidence status |
|---|---|
| Profile/service ready | `INFERRED` from SA-1/SA-2 (`sub_824943F0`, `sub_824994B8`) |
| Device ready | `INFERRED` from SA-1/SA-2 (`sub_82495348`) |
| Combat | `UNKNOWN` — canonical predicate not found |
| Cutscene/scripted event | `UNKNOWN` — canonical predicate not found |
| Loading/map transition | `UNKNOWN` — canonical predicate not found |
| Async save/load busy | `INFERRED` manager fields; canonical predicate not proven |
| Another menu active | `UNKNOWN` |

`sub_824D1CC0` is **not** a save-safety predicate; it always returns `1`.

SA-4 should not invent a combat/cutscene/loading list until the game's own predicate is
identified, or until a conservative blocking predicate is validated at runtime.

---

## G. Original Debug/Developer Save Search

`UNKNOWN / no direct path found`

- String-level smoke checks found no `QuickSave`, `ForceSave`, `DebugSave`, or
  `SaveAnywhere` text.
- The presence of `CPlayerInput_DebugCtrl` and `CDebugOutputTask` was confirmed as RTTI
  in SA-2, but no code relationship from those debug classes to the save
  manager/window classes was proven.
- A full vtable/xref audit of `CPlayerInput_DebugCtrl` and `CDebugOutputTask` was not
  completed before the SA-3 stop condition was reached.

This is not the blocking issue; if a hidden debug-save path exists, runtime observation
would likely expose it through the same `sub_824858E8`/dispatcher breakpoints.

---

## H. SavePoint / SaveTarget Requirements

`VERIFIED`

- The serializer (`sub_8247FF00`) does **not** dereference a live save-point actor.
- The normal write path reads map state from a type-201 registry object and a world-state
  flag, as established in SA-2.

`INFERRED`

- `CSavePointManager`, `CSavePointObject`, and `CSaveTarget` gate the normal physical
  save-point interaction and may supply the checkpoint index, but opening the window is
  not itself blocked by `sub_824858E8`.

`UNKNOWN`

- Whether `CSaveTarget` must be selected/synthesized before the window can reach
  `sub_824803C8` for an arbitrary location. This is secondary to the owner-acquisition
  blocker but should be observed in the same runtime capture.

---

## I. Checkpoint Source and Restore Semantics

SA-2 proved that arbitrary live player coordinates are not serialized. SA-3 did not
resolve the checkpoint index writer, so the concrete restore semantics remain:

`UNKNOWN` — choose between:

- B: current map but predefined spawn;
- C: most recent legitimate save point;
- D: another checkpoint/location.

The checkpoint index source (owning object, field offset, setter/update routine) is
still unresolved. This is acceptable for SA-3 because it does not block choosing the
normal-save architecture; it only affects the expected player-spawn result.

---

## J. Async/Content-Service Safety

`INFERRED`

The persistent manager (`[0x82A58EA8 + 8]`) has relevant state fields (`+68`, `+72`,
`+80`, `+104`, `+108`, and state-machine fields around `+15944`, `+15980`, `+15984`).
The normal list/enumeration path uses these to prevent overlapping work.

`UNKNOWN`

No single canonical "save idle/busy" predicate was isolated and proven. SA-4 must either
use the normal dispatcher exclusively (which already manages these transitions) or add a
guard only after the busy encoding is runtime-validated.

---

## K. Recommended Runtime Invocation

Preferred, once the blocker is resolved:

```
Debug Menu -> Save Anywhere
  1. acquire live CCampSaveLoadWnd owner        (currently UNKNOWN)
  2. verify owner is valid and dispatcher gate is clear
  3. call sub_824858E8(owner)                   (normal opener)
  4. let sub_82485188 / sub_824803C8 /
     sub_8247FF00 / sub_82494A18 / sub_824958A0 run normally
```

`VERIFIED` backend components: `sub_824858E8`, `sub_82485188`, `sub_824803C8`,
`sub_8247FF00`, `sub_82494A18`, `sub_824958A0`.

`UNKNOWN` precondition: step 1.

Do **not** call `sub_82475670` directly as the production path until the widget-layout
registration is proven. It is allocator/registration code, not the high-level trigger.

---

## L. SA-3B Minimal Dynamic Validation Plan

Goal: resolve the one blocker and observe the secondary questions in one session.

### L.1 Capture the live owner

- Breakpoint/log entry of `sub_824858E8` during a normal save-point interaction.
- Record:
  - `r3`;
  - `[r3 + 0]` (expect `0x82039494`);
  - `[r3 + 16]`, `[r3 + 40]`, `[r3 + 264]`, `[r3 + 332]`.
- Capture the call stack or caller context to identify where the live object is stored
  (scene task field, registry slot, or global).

This converts section C.3 from `UNKNOWN` to `VERIFIED`.

### L.2 Confirm direct-window insufficiency

- Breakpoint/log after `sub_82475670` in the normal flow and compare `+268`, `+272`,
  `+280`, `+284`, `+312`, `+316`, `+320`, `+328`.
- If they are non-zero in the normal flow but zero in a direct-create experiment, the
  blocker is confirmed and the missing layout/builder call can be identified from the
  normal caller context.

### L.3 Observe checkpoint/save target

- Record the type-201 map-id/checkpoint fields before and after a save.
- Perform a normal save at an arbitrary free-roam position, load it, and record map id
  and player spawn. This resolves section I as B/C/D.

### L.4 Observe the dispatcher gate

- Record `[0x82A95998]` and `[obj + 60]` during exploration, combat, cutscene, and map
  loading.
- Find the writer that sets `+60 = 1` and determine the canonical safety predicate.

This experiment is read-only and can use the existing Vesplume diagnostic scaffolding
as a temporary host-side logger; it does not implement Save Anywhere.

---

## M. Remaining Unknowns

- Live `CCampSaveLoadWnd` acquisition/storage (blocking).
- Exact normal layout/builder call relationship to the `CSaveLoadWnd` window.
- Meaning and writer of `[0x82A95998-object + 60]`.
- Canonical combat/cutscene/loading/async safety predicates.
- Checkpoint index source and exact restore result (B/C/D).
- Whether a pre-existing debug/developer save path exists.

---

## N. Repository Integrity

Start of SA-3:

```
HEAD = 11f58cef39beccd52ef0764f1895dbfe7945ac11  (main)
 M src/infinite_undiscovery_app.h
 M src/main.cpp
?? docs/SA-01-save-anywhere-debug-action.md
?? docs/SA-02-save-anywhere-target-acquisition.md
?? src/vesplume_diag.h
```

End of SA-3:

```
HEAD = 11f58cef39beccd52ef0764f1895dbfe7945ac11  (main, unchanged)
 M src/infinite_undiscovery_app.h              (pre-existing, untouched)
 M src/main.cpp                                (pre-existing, untouched)
?? docs/SA-01-save-anywhere-debug-action.md    (pre-existing, untouched)
?? docs/SA-02-save-anywhere-target-acquisition.md (pre-existing, untouched)
?? src/vesplume_diag.h                         (pre-existing, untouched)
?? docs/SA-03-save-anywhere-runtime-entry.md   (new, this deliverable)
```

Only the SA-03 deliverable was added during this task. No source, generated, save, or
Vesplume diagnostic files were modified.


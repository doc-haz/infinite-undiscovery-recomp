// ui_textures_es.h -- GENERIC SUBSTITUTION of ES TEXTURES (bitmap labels).
//
// Extends the `FindAtlasOnce`/`ReplaceAtlas` mechanism of `translation_es.h` to
// **all** translated textures, without patching the game assets:
//
//   * `tools/ui_textures.py render`  paints the labels (AIF, same size).
//   * `tools/ui_textures.py manifest` deploys the `.aif` files to
//     `translation/` (game cwd) and writes `translation/ui_textures.txt`.
//   * THIS hook reads the manifest and substitutes in guest memory each texture
//     with its SIGNATURE (0x30 B AIF header) + 64-bit FNV-1a of the whole file.
//
// Two lookup paths:
//   1. `OnMsg(base, msgbase)`: called by `TrySubstitute` (which already has the
//      `.msg` base).  Substitutes, in guest memory, the `.msg` (metrics) and the
//      `AIF` atlas preceding it (`msgbase - aif_len`) -> covers the `RMD-`
//      fonts (e.g. accents 005).
//   2. `Tick(base)`: starts ONE thread ONCE that scans guest memory by
//      signature for persistent textures (UI atlases, GAME OVER, Now
//      Loading, Reload...).  The sweep does NOT run on the game thread (doing
//      it with `VirtualQuery` per page hung startup for ~17 s); see
//      `ScanTextures`/`ScannerLoop`.
//
// Manifest:  text, 3 header lines + N lines
//     IUT1
//     <n>
//     <sig_hex> <len> <fnv64_hex> <path>
// Path configurable via `IU_ES_TEXTURES`; defaults to `translation/ui_textures.txt`.
//
// Include only ONCE (from `translation_es.h`).

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <rex/ppc/context.h>

#include "iu_trace.h"
// Controller detection (auto via SDL/log): decides which texture variant
// (icons/image) gets substituted.  See mod/recomp/controller_es.h.
#include "controller_es.h"

namespace iu::ui {

inline uint32_t bswap32u(uint32_t v) { return __builtin_bswap32(v); }

// ---- manifest entry ---------------------------------------------------------
struct Entry {
  std::vector<uint8_t> sig;
  uint32_t len = 0;
  uint64_t hash = 0;
  std::string path;
  // Controller the variant applies to: "*" (all) or "xbox"/"ps5"/"steam".
  std::string ctrl = "*";
  // Atomic + in a unique_ptr so that `Entry` stays movable (the scanner
  // runs on a separate thread; see `EnsureScanner`).
  std::unique_ptr<std::atomic<bool>> done = std::make_unique<std::atomic<bool>>(false);
};

inline std::vector<Entry>& entries() {
  static std::vector<Entry> e;
  return e;
}

// Total substitutions performed (any path: OnMsg/OnAif/scanner).  Used by the
// scanner loop to detect "quiet" sweeps that keep finding nothing and back off.
inline std::atomic<uint32_t>& subst_count() {
  static std::atomic<uint32_t> n{0};
  return n;
}
inline bool& loaded() {
  static bool b = false;
  return b;
}

inline const char* ManifestPath() {
  const char* e = std::getenv("IU_ES_TEXTURES");
  return (e && e[0]) ? e : "translation/ui_textures.txt";
}

// Working directory captured when the manifest is loaded.  The manifest paths
// are relative to it; reading them LATER relative to the (possibly changed)
// process cwd made `ReadFile` fail for late textures (the log showed
// "no abro translation/help_*.aif" although the files existed), which left the
// scanner sweeping forever (§7).  Resolving against this fixed base avoids it.
inline std::string& base_dir() {
  static std::string d;
  return d;
}

inline bool IsAbsolutePath(const std::string& p) {
  if (p.size() >= 2 && p[1] == ':') return true;      // C:\...
  if (!p.empty() && (p[0] == '/' || p[0] == '\\')) return true;
  return false;
}

inline uint64_t Fnv64(const uint8_t* p, size_t n) {
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < n; ++i) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

inline int HexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

inline bool ParseHex(const char* s, std::vector<uint8_t>& out) {
  out.clear();
  for (; s[0] && s[1]; s += 2) {
    int a = HexVal(s[0]), b = HexVal(s[1]);
    if (a < 0 || b < 0) return false;
    out.push_back(static_cast<uint8_t>((a << 4) | b));
  }
  return !out.empty();
}

inline void LoadManifest() {
  if (loaded()) return;
  loaded() = true;
  {
    char cwd[1024] = {0};
    if (GetCurrentDirectoryA(sizeof(cwd), cwd) && cwd[0]) base_dir() = cwd;
  }
  std::FILE* f = std::fopen(ManifestPath(), "rb");
  if (!f) {
    iu::trace::emit(iu::trace::kGeneral, "UI: manifiesto ausente (%s)", ManifestPath());
    return;
  }
  char line[512];
  if (!std::fgets(line, sizeof(line), f)) {
    std::fclose(f);
    return;
  }
  // IUT1: <sig> <len> <hash> <path>              (no controller = "*")
  // IUT2: <sig> <len> <hash> <ctrl> <path>       (per-controller variants)
  const bool v2 = std::strncmp(line, "IUT2", 4) == 0;
  if (!v2 && std::strncmp(line, "IUT1", 4) != 0) {
    std::fclose(f);
    iu::trace::emit(iu::trace::kGeneral, "UI: manifiesto invalido");
    return;
  }
  if (!std::fgets(line, sizeof(line), f)) {
    std::fclose(f);
    return;
  }
  int n = std::atoi(line);
  for (int i = 0; i < n; ++i) {
    if (!std::fgets(line, sizeof(line), f)) break;
    char sighex[160] = {0}, path[256] = {0}, ctrl[32] = {0};
    unsigned len = 0;
    unsigned long long hash = 0;
    Entry e;
    if (v2) {
      if (std::sscanf(line, "%159s %u %llx %31s %255s", sighex, &len, &hash, ctrl, path) != 5)
        continue;
      e.ctrl = ctrl;
    } else {
      if (std::sscanf(line, "%159s %u %llx %255s", sighex, &len, &hash, path) != 4) continue;
    }
    if (!ParseHex(sighex, e.sig)) continue;
    e.len = len;
    e.hash = hash;
    e.path = path;
    // Anchor relative manifest paths to the cwd captured at load time (see
    // base_dir): the process cwd may change later and break lazy reads.
    if (!base_dir().empty() && !IsAbsolutePath(e.path))
      e.path = base_dir() + "/" + e.path;
    entries().push_back(std::move(e));
  }
  std::fclose(f);
  // Controller variants are tried BEFORE the "*" ones, so that the
  // specific one (e.g. ps5) wins over the generic one from the same source.
  std::stable_partition(entries().begin(), entries().end(),
                        [](const Entry& e) { return e.ctrl != "*"; });
  iu::trace::emit(iu::trace::kGeneral, "UI: %zu texturas en manifiesto", entries().size());
}

inline bool ReadFile(const char* path, std::vector<uint8_t>& out) {
  std::FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  out.clear();
  uint8_t tmp[65536];
  size_t n;
  while ((n = std::fread(tmp, 1, sizeof(tmp), f)) > 0) out.insert(out.end(), tmp, tmp + n);
  std::fclose(f);
  return true;
}

inline bool AnyPending() {
  for (auto& e : entries())
    if (!e.done->load(std::memory_order_relaxed)) return true;
  return false;
}

// Substitutes the region at `addr` if its signature (cheap) + hash (strong) match.
// `ignore_done` is used by the `OnAif` path (ASF load): even if the texture was
// already substituted before, if the engine RE-LOADS the ASF the original AIF
// comes back and must be substituted again.  It is cheap because the manifest
// signatures are unique: an AIF matches at most ONE entry.
inline bool TryReplace(uint8_t* base, uint32_t addr, Entry& e, bool ignore_done = false) {
  if (!ignore_done && e.done->load(std::memory_order_relaxed)) return false;
  // Per-controller variants: if the controller has not been detected yet
  // (`auto`), they are DEFERRED (not marked done) to avoid painting the wrong
  // variant.  Once resolved, variants that are NOT for the active controller
  // are discarded (they never apply: `es_controller` requires a restart), so
  // the scanner does not keep sweeping indefinitely for pending textures.
  if (e.ctrl != "*") {
    if (!iu::ctrl::resolved()) return false;
    if (e.ctrl != iu::ctrl::Name(iu::ctrl::Active())) {
      e.done->store(true, std::memory_order_relaxed);
      return false;
    }
  }
  if (addr < 0x10000u) return false;
  if (!iu::trace::readable(base, addr, static_cast<uint32_t>(e.sig.size()))) return false;
  if (std::memcmp(iu::trace::host_ptr(base, addr), e.sig.data(), e.sig.size()) != 0) return false;
  if (!iu::trace::readable(base, addr, e.len)) return false;
  uint8_t* p = iu::trace::host_ptr(base, addr);
  if (Fnv64(p, e.len) != e.hash) return false;
  std::vector<uint8_t> buf;
  if (!ReadFile(e.path.c_str(), buf)) {
    iu::trace::emit(iu::trace::kGeneral, "UI: no abro %s", e.path.c_str());
    e.done->store(true, std::memory_order_relaxed);
    return false;
  }
  if (buf.size() != e.len) {
    iu::trace::emit(iu::trace::kGeneral, "UI: %s tiene %zu != %u", e.path.c_str(), buf.size(), e.len);
    e.done->store(true, std::memory_order_relaxed);
    return false;
  }
  std::memcpy(p, buf.data(), e.len);
  e.done->store(true, std::memory_order_relaxed);
  subst_count().fetch_add(1, std::memory_order_relaxed);
  // The same source can have several variants (controller); when applying one,
  // mark the rest as done so the generic one ("*") does not overwrite it.
  for (auto& o : entries()) {
    if (&o != &e && o.len == e.len && o.hash == e.hash)
      o.done->store(true, std::memory_order_relaxed);
  }
  iu::trace::emit(iu::trace::kGeneral, "UI: SUSTITUIDA %s @ %08X (%u B, mando=%s)",
                  e.path.c_str(), addr, e.len, e.ctrl.c_str());
  return true;
}

// Path 1: called from `TrySubstitute` with the `.msg` base already resolved.
inline void OnMsg(uint8_t* base, uint32_t msgbase) {
  LoadManifest();
  if (!loaded() || msgbase < 0x10000u) return;
  for (auto& e : entries()) {
    if (e.done->load(std::memory_order_relaxed)) continue;
    // The `.msg` itself (RMD- fonts) or the AIF atlas preceding it.
    TryReplace(base, msgbase, e);
    if (!e.done->load(std::memory_order_relaxed) && msgbase >= e.len)
      TryReplace(base, msgbase - e.len, e);
  }
}

// Path 2: signature scan, ON A SEPARATE THREAD.
//
// The scan walks guest memory **by regions** (`VirtualQuery` once per region,
// not per page: walking the 458k pages of 4 GiB cost ~17 s) and within each
// region uses `memchr` (SIMD) to locate "AIF " candidates.  It is limited to the
// guest data RAM ([0x80000000, 0xE0000000)); above that is the physical/MMIO
// alias.  Even so, reading ~1 GB of RAM costs hundreds of ms, so it runs on a
// `std::thread` (never on the game thread).  The only shared state is
// `Entry::done` (atomic) and the manifest (immutable).
inline void ScanRegion(uint8_t* base, uint8_t* start, size_t len, uint32_t guest_start,
                       uint32_t& hits) {
  const uint8_t* end = start + len;
  const uint8_t* q = start;
  while (q + 4 <= end) {
    const size_t span = static_cast<size_t>((end - 4) - q) + 1;
    const uint8_t* a = static_cast<const uint8_t*>(std::memchr(q, 'A', span));
    if (!a) break;
    if (a[1] == 'I' && a[2] == 'F' && a[3] == ' ') {
      ++hits;
      uint32_t addr = guest_start + static_cast<uint32_t>(a - start);
      for (auto& e : entries()) {
        if (e.done->load(std::memory_order_relaxed)) continue;
        TryReplace(base, addr, e);
      }
    }
    q = a + 1;
  }
}

struct Scanner {
  std::thread th;
  std::atomic<bool> stop{false};
  uint8_t* cursor = nullptr;  // sweep resume point
};
inline Scanner& scanner() {
  static Scanner* s = new Scanner();  // leak: usable at shutdown
  return *s;
}

inline void ScanTextures(uint8_t* base) {
  if (!loaded() || !AnyPending()) return;
  // Advances controller detection (auto via log; resolves to xbox after 12 s if
  // none appears) BEFORE deciding per-controller variants.
  iu::ctrl::Active();
  Scanner& s = scanner();
  const auto t0 = std::chrono::steady_clock::now();
  uint32_t regions = 0, hits = 0;
  uint8_t* hstart = iu::trace::host_ptr(base, 0x80000000u);
  // The game heap (and the title ASF, with its embedded AIF) reaches above
  // 0xE0000000 (title elements live at ~0xE67xxxxx), so it scans up to
  // 0xF0000000.  `VirtualQuery` per region + `memchr` keep the sweep cheap; it
  // still runs on a separate thread.
  uint8_t* hend = iu::trace::host_ptr(base, 0xF0000000u);
  // Resumable: each region is split into 16 MB blocks so the budget is checked
  // MID-region (a ~1 GB region took tens of seconds and blocked the thread).
  // On reaching the end, it restarts.
  uint8_t* hp = (s.cursor >= hstart && s.cursor < hend) ? s.cursor : hstart;
  bool budget_hit = false;
  // Smaller chunks (1 MB, was 4 MB) so the budget is checked more often: a
  // single chunk whose `AIF ` hits force several big FNV hashes can no longer
  // monopolise the scanner thread for ~10 s (observed 11 s with 4 MB chunks).
  const size_t kChunk = 1u << 20;  // 1 MB
  while (hp < hend && !budget_hit) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(hp, &mbi, sizeof(mbi))) {
      hp += 0x1000;
      continue;
    }
    uint8_t* rend = static_cast<uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
    if (rend > hend) rend = hend;
    if (rend <= hp) {
      hp += 0x1000;
      continue;
    }
    if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
      ++regions;
      while (hp < rend && !budget_hit) {
        uint8_t* cend = hp + kChunk;
        if (cend > rend) cend = rend;
        ScanRegion(base, hp, static_cast<size_t>(cend - hp),
                   static_cast<uint32_t>(hp - base), hits);
        hp = cend;
        const long long used = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - t0)
                                   .count();
        if (used > 500) budget_hit = true;
      }
    } else {
      hp = rend;
    }
  }
  s.cursor = (hp < hend) ? hp : hstart;  // resume or restart the cycle
  const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - t0)
                           .count();
  iu::trace::emit(iu::trace::kGeneral,
                  "UI: scan END regions=%u aif=%u %lld ms%s", regions, hits, ms,
                  budget_hit ? " (reanuda)" : "");
}

// Thread loop: sweep, wait, repeat while textures remain pending.
//
// The initial sweeps (first 4) run at a short cadence to catch the textures
// loaded at startup.  After that, the entries that are still pending are only
// loaded ON DEMAND (GAME OVER on death, the CAMP help screens, late UI), so a
// sweep that substitutes nothing means we are in the steady state: the cadence
// relaxes to 5 minutes (from the previous 1 min), which cuts the scanner's CPU
// impact ~5x while still catching late textures.  Twenty consecutive quiet
// sweeps (~100 min) stop the scanner entirely.  `IU_ES_SCANFAST=1` keeps the
// short cadence (diagnostic).
inline void ScannerLoop(uint8_t* base) {
  int quiet = 0;  // consecutive sweeps that substituted nothing
  for (int sweep = 0;; ++sweep) {
    Scanner& s = scanner();
    if (s.stop.load(std::memory_order_relaxed)) return;
    if (!AnyPending()) return;
    const uint32_t before = subst_count().load(std::memory_order_relaxed);
    ScanTextures(base);
    const uint32_t after = subst_count().load(std::memory_order_relaxed);
    if (sweep >= 4 && after == before) ++quiet;
    else quiet = 0;
    int wait_ms;
    if (sweep < 4) wait_ms = 800;          // initial load: catch up quickly
    else if (quiet < 3) wait_ms = 60000;   // recent activity: 1 min
    else wait_ms = 300000;                 // steady state: 5 min
    {
      const char* e = std::getenv("IU_ES_SCANFAST");
      if (e && e[0] && e[0] != '0') wait_ms = 1000;
    }
    for (int i = 0; i < wait_ms / 100; ++i) {
      if (s.stop.load(std::memory_order_relaxed)) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (quiet >= 20) return;  // steady state, nothing found in ~100 min
  }
}

inline void StopScanner() {
  Scanner& s = scanner();
  s.stop.store(true, std::memory_order_relaxed);
  if (s.th.joinable()) s.th.join();
}

inline void EnsureScanner(uint8_t* base) {
  static std::atomic<bool> started{false};
  if (started.exchange(true)) return;
  Scanner& s = scanner();
  s.th = std::thread([base]() { ScannerLoop(base); });
  std::atexit(&StopScanner);
}

// Tick: called once per parser.  It only starts the scanner (once); the
// sweep no longer blocks the game thread.
inline void Tick(uint8_t* base) {
  // Advances controller detection even if the sweep is inactive (so the
  // 12 s timeout and log reading happen on time).
  iu::ctrl::Active();
  LoadManifest();
  if (!loaded() || !AnyPending()) return;
  EnsureScanner(base);
}

}  // namespace iu::ui

// ---------------------------------------------------------------------------
// MAIN MENU PROBE: the title labels are loaded BY NAME
// (`CTitleLogoTask` -> `sub_824999A0` -> 22x `sub_82182070`).  This probe
// logs the name and the returned element, and searches for AIF signatures
// reachable from the element, to locate each label's texture.
// ---------------------------------------------------------------------------
namespace iu::ui {
inline bool NameProbeEnabled() {
  static const bool v = []() {
    const char* e = std::getenv("IU_ES_NAMEPROBE");
    // Only enabled with IU_ES_NAMEPROBE=1 (diagnostic); silent by default.
    return e && e[0] && e[0] != '0';
  }();
  return v;
}

inline void ProbeAif(uint8_t* base, const char* how, int off, uint32_t v) {
  if (!iu::trace::readable(base, v, 0x50)) return;
  uint8_t* p = iu::trace::host_ptr(base, v);
  if (p[0] != 'A' || p[1] != 'I' || p[2] != 'F' || p[3] != ' ') return;
  auto be32 = [&](int o) -> uint32_t {
    return (uint32_t(p[o]) << 24) | (uint32_t(p[o + 1]) << 16) |
           (uint32_t(p[o + 2]) << 8) | uint32_t(p[o + 3]);
  };
  auto be16 = [&](int o) -> uint32_t { return (uint32_t(p[o]) << 8) | p[o + 1]; };
  uint32_t total = be32(4), fmt = be32(0x30), w = be16(0x38), h = be16(0x3A);
  char ident[5] = {static_cast<char>(p[0x20]), static_cast<char>(p[0x21]),
                   static_cast<char>(p[0x22]), static_cast<char>(p[0x23]), 0};
  uint64_t h64 = 0;
  if (total >= 0x1000u && total <= 0x4000000u && iu::trace::readable(base, v, total))
    h64 = Fnv64(iu::trace::host_ptr(base, v), total);
  iu::trace::emit(iu::trace::kGeneral,
                  "UI MENU   AIF %s+%02X -> %08X ident=%s fmt=%02X %ux%u len=%u fnv=%016llx",
                  how, off, v, ident, fmt, w, h, total,
                  static_cast<unsigned long long>(h64));
}

inline void ProbeName(uint8_t* base, uint32_t namep, uint32_t elem) {
  if (!NameProbeEnabled()) return;
  char name[64] = {0};
  if (namep >= 0x10000u && iu::trace::readable(base, namep, 1)) {
    uint8_t* p = iu::trace::host_ptr(base, namep);
    for (int i = 0; i < 63; ++i) {
      uint8_t c = p[i];
      if (!c) break;
      name[i] = static_cast<char>(c);
    }
  }
  // Logs each name only ONCE (the lookup repeats every frame).
  static std::unordered_set<std::string> seen;
  if (!seen.insert(name).second) return;
  iu::trace::emit(iu::trace::kGeneral, "UI MENU name='%s' elem=%08X", name, elem);
  if (elem < 0x10000u) return;
  // Searches for AIF pointers in the first 0x200 B of the element (1 and 2
  // levels of indirection) and 8 B texture keys (asset name + word, like AIF+0x20).
  for (int off = 0; off < 0x200; off += 4) {
    if (!iu::trace::readable(base, elem + off, 4)) break;
    uint32_t v = bswap32u(*reinterpret_cast<volatile uint32_t*>(iu::trace::host_ptr(base, elem + off)));
    if (v < 0x10000u) continue;
    if (!iu::trace::readable(base, v, 4)) continue;
    uint32_t m = bswap32u(*reinterpret_cast<volatile uint32_t*>(iu::trace::host_ptr(base, v)));
    if (m == 0x41494620u) {  // "AIF "
      ProbeAif(base, "elem", off, v);
      continue;
    }
    // 2nd level: v is an intermediate object; look at its first 0x100 B.
    for (int o2 = 0; o2 < 0x100; o2 += 4) {
      if (!iu::trace::readable(base, v + o2, 4)) break;
      uint32_t w = bswap32u(*reinterpret_cast<volatile uint32_t*>(iu::trace::host_ptr(base, v + o2)));
      if (w < 0x10000u || !iu::trace::readable(base, w, 4)) continue;
      uint32_t m2 = bswap32u(*reinterpret_cast<volatile uint32_t*>(iu::trace::host_ptr(base, w)));
      if (m2 == 0x41494620u) ProbeAif(base, "elem2", off * 1000 + o2, w);
    }
  }
  // Texture keys (8 B: 4 ASCII + word) inside the element.  Offline they are
  // cross-referenced with AIF+0x20..0x28 to locate the asset.
  for (int off = 0; off + 8 <= 0x200; off += 4) {
    if (!iu::trace::readable(base, elem + off, 8)) break;
    uint8_t* p = iu::trace::host_ptr(base, elem + off);
    bool ascii = true;
    for (int k = 0; k < 4; ++k) {
      uint8_t c = p[k];
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_'))
        ascii = false;
    }
    uint32_t word = (uint32_t(p[4]) << 24) | (uint32_t(p[5]) << 16) |
                    (uint32_t(p[6]) << 8) | p[7];
    if (ascii && word != 0)
      iu::trace::emit(iu::trace::kGeneral, "UI MENU   KEY @ elem+%02X = %c%c%c%c:%08X",
                      off, p[0], p[1], p[2], p[3], word);
  }
}
}  // namespace iu::ui

extern "C" void __imp__sub_82182070(PPCContext& ctx, uint8_t* base);
extern "C" void sub_82182070(PPCContext& ctx, uint8_t* base) {
  const uint32_t namep = ctx.r4.u32;
  __imp__sub_82182070(ctx, base);
  iu::ui::ProbeName(base, namep, ctx.r3.u32);
}

// ---------------------------------------------------------------------------
// ASF/AIF LOADER PROBE (TASK 1): the title ASF embeds the AIF and the engine
// walks them with `sub_822226D8` / `sub_8221D768`, dispatching each
// `AIF ` chunk to `sub_821EF618` / `sub_821EF678`.  These probes log the
// invocation and the chunk tag to know WHERE to substitute (before the engine
// creates the texture).  Gated with `IU_ES_AIFPROBE=1`.
// ---------------------------------------------------------------------------
namespace iu::ui {
inline bool AifProbeEnabled() {
  static const bool v = []() {
    const char* e = std::getenv("IU_ES_AIFPROBE");
    return e && e[0] && e[0] != '0';
  }();
  return v;
}

inline uint32_t Be32At(uint8_t* base, uint32_t g) {
  if (!iu::trace::readable(base, g, 4)) return 0;
  return __builtin_bswap32(*reinterpret_cast<volatile uint32_t*>(iu::trace::host_ptr(base, g)));
}

inline void ProbeChunk(const char* fn, uint8_t* base, uint32_t r3, uint32_t r4) {
  if (!AifProbeEnabled()) return;
  static int n = 0;
  if (n++ > 400) return;
  uint32_t tag = Be32At(base, r4);
  char t[5] = {static_cast<char>(tag >> 24), static_cast<char>(tag >> 16),
               static_cast<char>(tag >> 8), static_cast<char>(tag), 0};
  iu::trace::emit(iu::trace::kGeneral, "UI AIF %s r3=%08X r4=%08X tag='%s'", fn, r3, r4, t);
}

inline void ProbeContainer(const char* fn, uint8_t* base, uint32_t r3, uint32_t r4) {
  if (!AifProbeEnabled()) return;
  static int n = 0;
  if (n++ > 200) return;
  uint32_t type16 = Be32At(base, r3 + 16);
  uint32_t tag = Be32At(base, r3 + 32);
  char t[5] = {static_cast<char>(tag >> 24), static_cast<char>(tag >> 16),
               static_cast<char>(tag >> 8), static_cast<char>(tag), 0};
  iu::trace::emit(iu::trace::kGeneral, "UI AIF %s r3=%08X r4=%08X [r3+16]=%08X tag0='%s'", fn, r3, r4,
                  type16, t);
}

// ---------------------------------------------------------------------------
// SUBSTITUTION AT ASF LOAD (TASK 1, main menu).
//
// The title ASF (`0001F800_000_MESH.asf`) is loaded whole into guest memory
// and the engine walks its chunks: `sub_822226D8`/`sub_8221D768` see the `AIF `
// tag and dispatch it to `sub_821EF618`/`sub_821EF678`, which log the texture
// BEFORE creating/uploading it.  Here we substitute the embedded AIF with the ES
// one at that very moment (same length), so the texture is created already in
// Spanish.  Previously this was done only by the thread scan, which arrived
// ~30 s late (the texture was already uploaded) -> the title came out in English.
//
// It is safe: `TryReplace` checks the 0x30 B signature and the FNV-64 of the
// whole file, so it only touches the AIF that are in the manifest.
// ---------------------------------------------------------------------------
inline void OnAif(uint8_t* base, uint32_t aifptr) {
  if (aifptr < 0x10000u) return;
  LoadManifest();
  if (!loaded()) return;
  for (auto& e : entries()) {
    // `ignore_done=true`: if the ASF is re-loaded, the original AIF comes back
    // and must be substituted again.  Cheap: unique signatures -> at most one hash.
    TryReplace(base, aifptr, e, /*ignore_done=*/true);
  }
}
}  // namespace iu::ui

#define IU_AIF_PROBE_CALL(fn, logcall)                                  \
  extern "C" void __imp__##fn(PPCContext& ctx, uint8_t* base);          \
  extern "C" void fn(PPCContext& ctx, uint8_t* base) {                  \
    logcall;                                                            \
    iu::ui::OnAif(base, ctx.r4.u32);                                    \
    __imp__##fn(ctx, base);                                             \
  }

#define IU_AIF_PROBE_CALL_NOAIF(fn, logcall)                            \
  extern "C" void __imp__##fn(PPCContext& ctx, uint8_t* base);          \
  extern "C" void fn(PPCContext& ctx, uint8_t* base) {                  \
    logcall;                                                            \
    __imp__##fn(ctx, base);                                             \
  }

IU_AIF_PROBE_CALL(sub_821EF618, iu::ui::ProbeChunk("821EF618", base, ctx.r3.u32, ctx.r4.u32))
IU_AIF_PROBE_CALL(sub_821EF678, iu::ui::ProbeChunk("821EF678", base, ctx.r3.u32, ctx.r4.u32))
IU_AIF_PROBE_CALL_NOAIF(sub_822226D8, iu::ui::ProbeContainer("822226D8", base, ctx.r3.u32, ctx.r4.u32))
IU_AIF_PROBE_CALL_NOAIF(sub_8221D768, iu::ui::ProbeContainer("8221D768", base, ctx.r3.u32, ctx.r4.u32))

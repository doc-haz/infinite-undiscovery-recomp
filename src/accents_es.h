// accents_es.h -- SPANISH ACCENT PATCH PER BANK (WS1, integration).
//
// Each `RMD-` atlas is a DXT2/3 `AIF ` with a 32x32 cell grid ordered by
// first appearance (`cell = code - 301`).  Only bank `005` shipped the glyphs
// `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡`; in the rest the pipeline folded them to
// ASCII (`¿`->`?`, `á`->`a`...).  `tools/add_accents_banks.py` composes those
// glyphs PER BANK (own bases + free cells) and emits:
//
//   * `extract/f2/accents/<stem>_accents.aif` + `.msg` (review material)
//   * `translation/accents_atlas.bin`  <- THIS hook consumes it
//   * `translation/accents_manifest.json` (documentation)
//
// Bin format (`IUA1`, little-endian):
//   u32 magic; u32 n;
//   n x {
//     u32 aif_len,width,height,format,ident,tiled_width,pixels_off,
//         element_bytes,total,count,atlas_w,atlas_h;
//     u64 aif_fnv;                       // FNV-1a of the ORIGINAL AIF (strong signature)
//     u32 nglyphs;
//     nglyphs x { u16 cell,u16 base_cell,u16 metric,u16 pad; u8 blocks[1024] }
//   }
//
// HOOKS
// -----
// The `RMD-` is loaded as a single buffer `[AIF ][message data]`; the message
// directory parser (`sub_826D52C0`) receives the base of the `.msg`
// (magic `"Mess"`) in `obj+0x64`/`obj+0x60`.  The atlas is right BEFORE it:
// `aif = msgbase - aif_size` (`aif_size` = field `0x58` of the `.msg`; verified
// in all 81 banks).
//
//   * `OnMsg(base, msgbase)` -- called from `iu::translation::TrySubstitute`
//     (which already resolved `msgbase`).  Identifies the bank by a cheap
//     signature (ident+format+size+dimensions) + FNV-64 of the ORIGINAL AIF,
//     writes the 16 DXT blocks of each glyph into the chosen cells and patches
//     the `.msg` metric table (the new glyph's = its base's, read from the
//     loaded `.msg` -> correct even if two discs share the same atlas).
//
// Cost: the first time a `msgbase` appears the strong signature is computed (an
// FNV over the atlas, hundreds of KB).  Afterwards the result is cached per
// `msgbase`; subsequent frames only reapply the ~16 KiB of blocks.
//
// Does NOT patch game assets: everything happens in guest memory.
//
// Include ONCE only (from `translation_es.h`).

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <rex/ppc/context.h>

#include "iu_trace.h"

namespace iu::accents {

// ---- big-endian guest read/write -------------------------------------------
inline uint32_t rd32(uint8_t* base, uint32_t g) {
  return __builtin_bswap32(
      *reinterpret_cast<volatile uint32_t*>(iu::trace::host_ptr(base, g)));
}
inline uint16_t rd16(uint8_t* base, uint32_t g) {
  return __builtin_bswap16(
      *reinterpret_cast<volatile uint16_t*>(iu::trace::host_ptr(base, g)));
}
inline void wr16(uint8_t* base, uint32_t g, uint16_t v) {
  *reinterpret_cast<volatile uint16_t*>(iu::trace::host_ptr(base, g)) =
      __builtin_bswap16(v);
}

inline uint32_t Be32(const uint8_t* p, int o) {
  return (uint32_t(p[o]) << 24) | (uint32_t(p[o + 1]) << 16) |
         (uint32_t(p[o + 2]) << 8) | uint32_t(p[o + 3]);
}
inline uint32_t Be16(const uint8_t* p, int o) {
  return (uint32_t(p[o]) << 8) | p[o + 1];
}

// ---- Xbox 360 tiled addressing (identical to tools/aif.py) -----------------
inline uint32_t AlignUp(uint32_t v, uint32_t to) { return (v + to - 1) / to * to; }

inline uint32_t TiledOffset(uint32_t x, uint32_t y, uint32_t width_elements,
                            uint32_t element_bytes) {
  uint32_t aligned = AlignUp(width_elements, 32);
  uint32_t log_bpp = 0;
  for (uint32_t e = element_bytes; e > 1; e >>= 1) ++log_bpp;
  uint32_t macro = ((x >> 5) + (y >> 5) * (aligned >> 5)) << (log_bpp + 7);
  uint32_t micro = ((x & 7) + ((y & 6) << 2)) << log_bpp;
  uint32_t offset = macro + ((micro & ~0xFu) << 1) + (micro & 0xFu) +
                    ((y & 8) << (3 + log_bpp)) + ((y & 1) << 4);
  return ((((offset & ~0x1FFu) << 3) + ((offset & 0x1C0u) << 2) +
           (offset & 0x3Fu) + ((y & 16) << 7) +
           (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)) >>
          log_bpp);
}

inline uint64_t Fnv64(const uint8_t* p, size_t n) {
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < n; ++i) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

// ---- data loaded from the bin ----------------------------------------------
struct Glyph {
  uint16_t cell;
  uint16_t base_cell;  // base cell to inherit the metric from (0xFFFF = none)
  uint16_t metric;     // fallback metric (if base_cell is not readable)
  uint16_t pad;
  uint8_t blocks[1024];  // 64 DXT3 blocks (8x8) already tiled+swapped, as on disk
};

struct Bank {
  uint32_t aif_len = 0, width = 0, height = 0, format = 0, ident = 0;
  uint32_t tw = 0, pixels_off = 0, element_bytes = 0;
  uint32_t total = 0, count = 0, atlas_w = 0, atlas_h = 0;
  uint64_t fnv = 0;
  std::vector<Glyph> glyphs;
};

inline std::vector<Bank>& banks() {
  static std::vector<Bank> b;
  return b;
}
inline bool& loaded() {
  static bool l = false;
  return l;
}
// msgbase -> bank index (-1 = not a bank with accents)
inline std::unordered_map<uint32_t, int>& cache() {
  static std::unordered_map<uint32_t, int> c;
  return c;
}

inline const char* BinPath() {
  const char* e = std::getenv("IU_ES_ACCENTS");
  return (e && e[0]) ? e : "translation/accents_atlas.bin";
}

inline void Load() {
  if (loaded()) return;
  loaded() = true;
  std::FILE* f = std::fopen(BinPath(), "rb");
  if (!f) {
    iu::trace::emit(iu::trace::kGeneral, "ACC: bin ausente (%s)", BinPath());
    return;
  }
  uint32_t magic = 0, n = 0;
  if (std::fread(&magic, 4, 1, f) != 1 || std::fread(&n, 4, 1, f) != 1 ||
      magic != 0x31415549u) {
    std::fclose(f);
    iu::trace::emit(iu::trace::kGeneral, "ACC: bin invalido");
    return;
  }
  for (uint32_t i = 0; i < n; ++i) {
    Bank b;
    uint32_t h[12];
    if (std::fread(h, 4, 12, f) != 12) break;
    b.aif_len = h[0]; b.width = h[1]; b.height = h[2]; b.format = h[3];
    b.ident = h[4]; b.tw = h[5]; b.pixels_off = h[6]; b.element_bytes = h[7];
    b.total = h[8]; b.count = h[9]; b.atlas_w = h[10]; b.atlas_h = h[11];
    if (std::fread(&b.fnv, 8, 1, f) != 1) break;
    uint32_t ng = 0;
    if (std::fread(&ng, 4, 1, f) != 1) break;
    b.glyphs.resize(ng);
    bool bad = false;
    for (uint32_t g = 0; g < ng; ++g) {
      Glyph& gl = b.glyphs[g];
      if (std::fread(&gl.cell, 2, 1, f) != 1 ||
          std::fread(&gl.base_cell, 2, 1, f) != 1 ||
          std::fread(&gl.metric, 2, 1, f) != 1 ||
          std::fread(&gl.pad, 2, 1, f) != 1) { bad = true; break; }
      if (std::fread(gl.blocks, 1, 1024, f) != 1024) { bad = true; break; }
    }
    if (bad) break;
    banks().push_back(std::move(b));
  }
  std::fclose(f);
  iu::trace::emit(iu::trace::kGeneral, "ACC: %zu bancos con acentos", banks().size());
}

// Cheap signature: AIF header fields that fit in a 0x40 B memcmp.
inline bool CheapMatch(uint8_t* base, uint32_t aifptr, const Bank& b) {
  if (!iu::trace::readable(base, aifptr, 0x50)) return false;
  uint8_t* p = iu::trace::host_ptr(base, aifptr);
  if (p[0] != 'A' || p[1] != 'I' || p[2] != 'F' || p[3] != ' ') return false;
  return Be32(p, 0x20) == b.ident && Be32(p, 0x30) == b.format &&
         Be16(p, 0x38) == b.width && Be16(p, 0x3A) == b.height;
}

inline void PatchAtlas(uint8_t* base, uint32_t aifptr, const Bank& b) {
  if (!b.element_bytes || !b.atlas_w) return;
  uint8_t* p = iu::trace::host_ptr(base, aifptr);
  uint32_t cols = b.atlas_w / 32;
  if (!cols) return;
  for (const Glyph& g : b.glyphs) {
    uint32_t row = g.cell / cols, col = g.cell % cols;
    size_t bi = 0;
    for (uint32_t gy = 0; gy < 8; ++gy) {
      for (uint32_t gx = 0; gx < 8; ++gx) {
        uint32_t off = TiledOffset(col * 8 + gx, row * 8 + gy, b.tw,
                                   b.element_bytes) * b.element_bytes;
        std::memcpy(p + b.pixels_off + off, g.blocks + bi, b.element_bytes);
        bi += b.element_bytes;
      }
    }
  }
}

inline void PatchMetrics(uint8_t* base, uint32_t msgbase, const Bank& b) {
  uint32_t mo = rd32(base, msgbase + 0x28);
  if (!mo) return;
  for (const Glyph& g : b.glyphs) {
    uint32_t off = msgbase + mo + 2u * g.cell;
    if (!iu::trace::readable(base, off, 2)) continue;
    uint16_t metric = g.metric;
    if (g.base_cell != 0xFFFFu) {
      uint32_t boff = msgbase + mo + 2u * g.base_cell;
      if (iu::trace::readable(base, boff, 2)) metric = rd16(base, boff);
    }
    wr16(base, off, metric);
  }
}

// Main hook.  Cheap after the first sighting of each `msgbase`.
inline void OnMsg(uint8_t* base, uint32_t msgbase) {
  if (msgbase < 0x10000u) return;
  Load();
  if (banks().empty()) return;

  auto it = cache().find(msgbase);
  if (it != cache().end()) {
    int idx = it->second;
    if (idx >= 0) {
      PatchAtlas(base, msgbase - banks()[idx].aif_len, banks()[idx]);
      PatchMetrics(base, msgbase, banks()[idx]);
    }
    return;
  }

  // Validate the `.msg` and locate the atlas preceding it.
  if (!iu::trace::readable(base, msgbase, 0x68) ||
      rd32(base, msgbase) != 0x4D657373u) {  // "Mess"
    cache()[msgbase] = -1;
    return;
  }
  uint32_t aif_len = rd32(base, msgbase + 0x58);
  if (aif_len == 0 || aif_len > msgbase) {
    cache()[msgbase] = -1;
    return;
  }
  uint32_t aifptr = msgbase - aif_len;

  int found = -1;
  int cheap_count = 0, cheap_idx = -1;
  for (size_t i = 0; i < banks().size(); ++i) {
    const Bank& b = banks()[i];
    if (b.aif_len != aif_len) continue;
    if (!CheapMatch(base, aifptr, b)) continue;
    ++cheap_count;
    cheap_idx = static_cast<int>(i);
    if (!iu::trace::readable(base, aifptr, b.aif_len)) continue;
    if (Fnv64(iu::trace::host_ptr(base, aifptr), b.aif_len) != b.fnv) continue;
    found = static_cast<int>(i);
    break;
  }
  // Fallback: if the AIF was already replaced by `ui_textures_es.h` (e.g. the
  // enlarged atlas 005), its FNV is no longer the original one.  If the cheap
  // signature (ident+format+size+dimensions) identifies a SINGLE bank, it is safe.
  if (found < 0 && cheap_count == 1) found = cheap_idx;
  cache()[msgbase] = found;
  if (found >= 0) {
    const Bank& b = banks()[found];
    PatchAtlas(base, aifptr, b);
    PatchMetrics(base, msgbase, b);
    iu::trace::emit(iu::trace::kGeneral,
                    "ACC: banco parcheado msg=%08X aif=%08X (%zu glifos)",
                    msgbase, aifptr, b.glyphs.size());
  } else {
    iu::trace::emit(iu::trace::kGeneral,
                    "ACC: sin banco para msg=%08X aif_len=%u", msgbase, aif_len);
  }
}

}  // namespace iu::accents

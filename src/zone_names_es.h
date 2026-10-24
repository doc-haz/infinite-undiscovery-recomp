// zone_names_es.h -- translate the .exe's zone-name table (MapName) at runtime.
//
// The load/save slot formatter reads the .exe's `MapName` table (guest
// ~0x82A01A58; 59 records of 68 B: u32 zoneId + char name[64]) as ASCII.
// Rewriting that table in guest memory shows the Spanish names wherever it is
// used, without touching any asset.  The names are ASCII-folded (the game's
// zone-title font has no accented glyphs).
//
// NOTE: the in-game "entering <area>" banner uses its OWN serif renderer and is
// NOT covered by this; see docs/re/zone-banner.md.
//
// Include ONCE only from src/main.cpp (through translation_es.h).

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "iu_trace.h"
#include "zone_names.h"

namespace iu::translation {

inline const char* ZoneLookupEs(const char* en) {
  int n = 0;
  const ZoneNamePair* p = ZoneNamePairs(n);
  for (int i = 0; i < n; ++i)
    if (std::strcmp(p[i].en, en) == 0) return p[i].es;
  return nullptr;
}

inline void ZoneRewriteAt(uint8_t* base, uint32_t guest, int& changed) {
  if (!iu::trace::readable(base, guest, 64)) return;
  char* name = reinterpret_cast<char*>(iu::trace::host_ptr(base, guest));
  char en[64];
  std::strncpy(en, name, sizeof(en));
  en[sizeof(en) - 1] = 0;
  size_t l = std::strlen(en);
  while (l && (en[l - 1] == ' ' || en[l - 1] == '\t')) en[--l] = 0;
  if (!l) return;
  const char* es = ZoneLookupEs(en);
  if (!es || std::strcmp(es, en) == 0) return;
  size_t el = std::strlen(es);
  if (el >= 64) return;
  std::memset(name, 0, 64);
  std::memcpy(name, es, el);
  ++changed;
}

// Rewrites the 59-record MapName table in place (run once, at startup).
inline void ZoneNamesStart(uint8_t* base) {
  static bool started = false;
  if (started) return;
  started = true;
  const char* e = std::getenv("IU_ES_ZONENAMES");
  if (e && e[0] == '0') return;  // disabled
  const uint32_t kTable = 0x82A01A58u, kRec = 68, kCount = 59;
  int changed = 0;
  for (uint32_t i = 0; i < kCount; ++i) ZoneRewriteAt(base, kTable + i * kRec + 4, changed);
  iu::trace::emit(iu::trace::kGeneral, "ES: zone-name table rewritten (%d)", changed);
}

}  // namespace iu::translation

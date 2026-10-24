// controller_es.h -- DETECTION of the connected controller (multi-controller).
//
// The mod can show button icons and mapping image matching the controller:
//   * xbox  : Xbox (A/B/X/Y, LB/RB, LT/RT) -- default.
//   * ps5   : DualSense (cross/circle/square/triangle, L1/R1, L2/R2).
//   * steam : Steam Controller (A/B/X/Y, trackpads).
//
// DETECTION SOURCES, in priority order:
//   1. CVAR `es_controller` (auto|xbox|ps5|steam) or env `IU_ES_CONTROLLER`.
//   2. In `auto`: the runtime `runtime.log` is read, which already logs each
//      SDL controller:
//        SDL OnControllerDeviceAdded: "<name>", ..., VendorID(0x045E), ProductID(0x0B13)
//      and the VendorID is mapped: 0x045E=Microsoft(xbox), 0x054C=Sony(ps5),
//      0x28DE=Valve(steam).  It is the same data ReXGlue logs when initializing
//      the SDL driver, without touching SDL or the input system.
//
// The log is read at most once every 500 ms while the controller is not
// resolved (cheap: the file is small and already cached).  If no controller
// appears within 12 s, it resolves to `xbox` (the original console was
// Xbox 360, so it is the safe default look).
//
// Include ONCE only (from ui_textures_es.h).

#pragma once

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include <rex/cvar.h>

#include "iu_trace.h"

namespace iu::ctrl {

enum class Type { Xbox, Ps5, Steam };

inline const char* Name(Type t) {
  switch (t) {
    case Type::Ps5: return "ps5";
    case Type::Steam: return "steam";
    default: return "xbox";
  }
}

inline bool ParseType(const std::string& v, Type& out) {
  std::string s;
  for (char c : v) s.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  if (s == "xbox" || s == "xinput" || s == "microsoft") { out = Type::Xbox; return true; }
  if (s == "ps5" || s == "ps" || s == "playstation" || s == "dualsense" || s == "dualshock") {
    out = Type::Ps5; return true;
  }
  if (s == "steam" || s == "valve" || s == "steamcontroller" || s == "steamdeck") {
    out = Type::Steam; return true;
  }
  return false;
}

// Forced value (CVAR > env); "auto" if it has not been set.
inline std::string Forced() {
  if (rex::cvar::GetFlagSource("es_controller") != rex::cvar::Source::kDefault) {
    std::string v = rex::cvar::GetFlagByName("es_controller");
    if (!v.empty()) return v;
  }
  const char* e = std::getenv("IU_ES_CONTROLLER");
  return (e && e[0]) ? std::string(e) : std::string("auto");
}

// runtime.log path: IU_TRACE_DIR (set by the app) or the log_file CVAR.
inline std::string LogPath() {
  const char* d = std::getenv("IU_TRACE_DIR");
  if (d && d[0]) return std::string(d) + "/runtime.log";
  if (rex::cvar::GetFlagSource("log_file") != rex::cvar::Source::kDefault) {
    std::string lf = rex::cvar::GetFlagByName("log_file");
    if (!lf.empty()) return lf;
  }
  return "runtime.log";
}

// Returns the last VendorID logged by SDL in `path` (hex), or false.
inline bool ParseLog(const char* path, uint32_t& vendor) {
  std::FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  char line[2048];
  uint32_t last = 0;
  bool found = false;
  while (std::fgets(line, sizeof(line), f)) {
    const char* p = std::strstr(line, "OnControllerDeviceAdded");
    if (!p) continue;
    const char* v = std::strstr(p, "VendorID(0x");
    if (!v) continue;
    last = static_cast<uint32_t>(std::strtoul(v + 11, nullptr, 16));
    found = true;
  }
  std::fclose(f);
  if (found) vendor = last;
  return found;
}

inline Type FromVendor(uint32_t v) {
  if (v == 0x054Cu) return Type::Ps5;    // Sony (DualSense/DualShock)
  if (v == 0x28DEu) return Type::Steam;  // Valve
  return Type::Xbox;                     // Microsoft (0x045E) and others
}

inline std::atomic<int>& cachedType() {
  static std::atomic<int> t{static_cast<int>(Type::Xbox)};
  return t;
}
inline std::atomic<bool>& resolvedFlag() {
  static std::atomic<bool> r{false};
  return r;
}
inline bool resolved() { return resolvedFlag().load(std::memory_order_relaxed); }
inline Type cached() { return static_cast<Type>(cachedType().load(std::memory_order_relaxed)); }
inline void setResolved(Type t) {
  cachedType().store(static_cast<int>(t), std::memory_order_relaxed);
  resolvedFlag().store(true, std::memory_order_relaxed);
}

inline void LogResolved(const char* how) {
  static std::atomic<bool> done{false};
  if (done.exchange(true)) return;
  iu::trace::emit(iu::trace::kGeneral, "CTRL: mando=%s (%s)", Name(cached()), how);
}

// Resolves (and caches) the active controller.  Cheap: while unresolved it reads
// the log at most once every 500 ms; after 12 s it falls back to `xbox`.
inline Type Active() {
  if (resolved()) return cached();
  std::string forced = Forced();
  if (forced != "auto" && !forced.empty()) {
    Type t;
    if (ParseType(forced, t)) {
      setResolved(t);
      LogResolved("forzado");
      return t;
    }
  }
  static std::mutex m;
  static auto t0 = std::chrono::steady_clock::now();
  static auto last_check = std::chrono::steady_clock::time_point{};
  std::lock_guard<std::mutex> lock(m);
  if (resolved()) return cached();
  const auto now = std::chrono::steady_clock::now();
  if (last_check != std::chrono::steady_clock::time_point{} &&
      now - last_check < std::chrono::milliseconds(500)) {
    return cached();
  }
  last_check = now;
  uint32_t vendor = 0;
  if (ParseLog(LogPath().c_str(), vendor)) {
    setResolved(FromVendor(vendor));
    LogResolved("auto/log");
    return cached();
  }
  if (now - t0 > std::chrono::seconds(12)) {
    setResolved(Type::Xbox);
    LogResolved("auto/timeout");
  }
  return cached();
}

// Utility for offline tests: forces a specific type (without CVAR).
inline void ForceForTest(Type t) { setResolved(t); }

}  // namespace iu::ctrl

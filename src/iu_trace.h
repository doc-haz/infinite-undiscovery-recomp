#pragma once

// IU debug trace framework: one common, non-blocking, category-gated log layer.
//
// Purely observational. Nothing here modifies guest state.
//
// Enable categories at launch (environment variable, comma/semicolon separated):
//     IU_TRACE=SAVEPOINT              one category
//     IU_TRACE=SAVEPOINT,VESPLUME     several
//     IU_TRACE=ALL                    everything
// Or dynamically at runtime via Community Debug menu (Start Trace / Stop Trace).
// Output directory: IU_TRACE_DIR (set by InfiniteUndiscoveryApp::SetupEnvironment to
// <portable root>/logs, i.e. NTSC-U/logs). Default log file: iu_trace.log, or
// iu_trace_YYYYMMDD_HHMMSS.log when started from recorder.
//
// Cost when a category is disabled: one relaxed atomic mask test (no formatting, no I/O).
// Cost when enabled: format into a preallocated ring slot (no locks, no file I/O on the
// calling thread). A background thread drains the ring every ~50 ms. If the ring is full,
// records are dropped (and counted) instead of blocking the game.
//
// See docs/DEBUG-TRACE-FRAMEWORK.md for how to add a category / trace point.

#include <windows.h>

#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

namespace iu::trace {

// ---- Categories ------------------------------------------------------------
// To add one: add a bit here AND a row in kCategories below.
enum Category : uint32_t {
  kGeneral = 1u << 0,
  kSavePoint = 1u << 1,
  kVesplume = 1u << 2,
  kAll = 0xFFFFFFFFu,
};

struct CategoryInfo {
  const char* name;
  uint32_t bit;
};
inline constexpr CategoryInfo kCategories[] = {
    {"GENERAL", kGeneral},
    {"SAVEPOINT", kSavePoint},
    {"VESPLUME", kVesplume},
};

inline const char* category_name(uint32_t bit) {
  for (const auto& c : kCategories)
    if (c.bit == bit) return c.name;
  return "?";
}

inline uint32_t parse_mask(const char* spec) {
  if (!spec || !spec[0]) return 0;
  uint32_t mask = 0;
  std::string tok;
  auto flush = [&]() {
    if (tok.empty()) return;
    if (_stricmp(tok.c_str(), "ALL") == 0) mask |= kAll;
    for (const auto& c : kCategories)
      if (_stricmp(tok.c_str(), c.name) == 0) mask |= c.bit;
    tok.clear();
  };
  for (const char* p = spec; *p; ++p) {
    if (*p == ',' || *p == ';' || *p == ' ') flush();
    else tok.push_back(*p);
  }
  flush();
  return mask;
}

// Runtime enabled-category mask. Initialized from IU_TRACE if set, otherwise 0 (OFF).
inline std::atomic<uint32_t>& runtime_mask() {
  static std::atomic<uint32_t> m{[]() {
    const char* e = std::getenv("IU_TRACE");
    return e ? parse_mask(e) : 0u;
  }()};
  return m;
}

inline uint32_t mask() {
  return runtime_mask().load(std::memory_order_relaxed);
}

inline bool enabled(uint32_t cat) {
  return (mask() & cat) != 0;
}

inline const std::string& log_dir() {
  static const std::string dir = []() {
    const char* e = std::getenv("IU_TRACE_DIR");
    return std::string((e && e[0]) ? e : "logs");
  }();
  return dir;
}

// ---- Session & Recording State ----------------------------------------------
struct SessionState {
  std::mutex mtx;
  std::string active_path;
  std::string active_filename{"iu_trace.log"};
  double start_time_ms{0.0};
  std::atomic<uint32_t> marker_counter{0};
  std::atomic<bool> rotate_requested{false};
  std::atomic<bool> close_requested{false};
};

inline SessionState& session() {
  static SessionState s;
  return s;
}

// ---- Sink: lock-free bounded ring (many producers, one writer thread) -------
enum Dest : uint8_t { kDestMain = 0, kDestVesplume = 1 };

namespace detail {

constexpr size_t kSlots = 8192;  // power of two
constexpr size_t kText = 500;

struct Slot {
  std::atomic<size_t> seq;
  uint8_t dest;
  uint16_t len;
  char text[kText];
};

struct Ring {
  Slot slots[kSlots];
  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};  // writer thread only
  std::atomic<unsigned long long> dropped{0};
  std::atomic<unsigned long long> seq{0};
  std::atomic<bool> stop{false};
  std::atomic<bool> started{false};
  std::atomic<bool> starting{false};
  std::thread worker;
  std::FILE* files[2] = {nullptr, nullptr};
  Ring() {
    for (size_t i = 0; i < kSlots; ++i) slots[i].seq.store(i, std::memory_order_relaxed);
  }
};

inline Ring& ring() {
  static Ring* r = new Ring();  // intentionally leaked: usable during shutdown
  return *r;
}

inline double now_ms() {
  static const LARGE_INTEGER freq = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
  static const LARGE_INTEGER t0 = []() { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t; }();
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  return double(t.QuadPart - t0.QuadPart) * 1000.0 / double(freq.QuadPart);
}

inline std::string dest_path(uint8_t dest) {
  if (dest == kDestVesplume) {
    const char* e = std::getenv("IU_VESPLUME_TRACE_PATH");
    if (e && e[0]) return e;
    return log_dir() + "\\vesplume_trace.log";
  }
  std::lock_guard<std::mutex> lk(session().mtx);
  if (!session().active_path.empty()) {
    return session().active_path;
  }
  return log_dir() + "\\iu_trace.log";
}

inline void write_slot(Ring& r, const Slot& s) {
  if (s.dest == kDestMain && session().rotate_requested.exchange(false, std::memory_order_acq_rel)) {
    if (r.files[kDestMain]) {
      std::fflush(r.files[kDestMain]);
      std::fclose(r.files[kDestMain]);
      r.files[kDestMain] = nullptr;
    }
  }
  std::FILE*& f = r.files[s.dest];
  if (!f) {
    f = std::fopen(dest_path(s.dest).c_str(), "a");
    if (!f) return;  // logging failure never affects the game
    if (s.dest == kDestMain) std::fprintf(f, "---- IU TRACE SESSION START (mask=0x%08X) ----\n", mask());
  }
  std::fwrite(s.text, 1, s.len, f);
  std::fputc('\n', f);
}

inline bool drain_some(Ring& r) {
  bool any = false;
  for (;;) {
    const size_t pos = r.tail.load(std::memory_order_relaxed);
    Slot& s = r.slots[pos & (kSlots - 1)];
    if (s.seq.load(std::memory_order_acquire) != pos + 1) break;
    write_slot(r, s);
    s.seq.store(pos + kSlots, std::memory_order_release);
    r.tail.store(pos + 1, std::memory_order_relaxed);
    any = true;
  }
  return any;
}

inline void flush_files(Ring& r) {
  for (auto* f : r.files)
    if (f) std::fflush(f);
}

inline void finish() {
  Ring& r = ring();
  if (!r.started.load()) return;
  r.stop.store(true);
  if (r.worker.joinable()) r.worker.join();
  drain_some(r);
  const auto d = r.dropped.load();
  if (d && r.files[kDestMain]) std::fprintf(r.files[kDestMain], "---- DROPPED %llu records (ring full) ----\n", d);
  flush_files(r);
}

inline void ensure_started() {
  Ring& r = ring();
  if (r.started.load(std::memory_order_acquire)) return;
  if (r.starting.exchange(true)) {
    while (!r.started.load(std::memory_order_acquire)) Sleep(0);
    return;
  }
  r.worker = std::thread([]() {
    Ring& rr = ring();
    unsigned long long reported = 0;
    while (!rr.stop.load(std::memory_order_relaxed)) {
      const bool any = drain_some(rr);
      const auto d = rr.dropped.load(std::memory_order_relaxed);
      if (d != reported && rr.files[kDestMain]) {
        std::fprintf(rr.files[kDestMain], "---- DROPPED %llu records so far ----\n", d);
        reported = d;
      }
      if (session().close_requested.exchange(false, std::memory_order_acq_rel)) {
        flush_files(rr);
        if (rr.files[kDestMain]) {
          std::fclose(rr.files[kDestMain]);
          rr.files[kDestMain] = nullptr;
        }
      }
      if (any) flush_files(rr);
      Sleep(50);
    }
  });
  std::atexit(&finish);
  r.started.store(true, std::memory_order_release);
}

// Reserve a slot. Returns nullptr if the ring is full (record dropped).
inline Slot* acquire(Ring& r, size_t& pos_out) {
  size_t pos = r.head.load(std::memory_order_relaxed);
  for (;;) {
    Slot& s = r.slots[pos & (kSlots - 1)];
    const size_t seq = s.seq.load(std::memory_order_acquire);
    const intptr_t diff = intptr_t(seq) - intptr_t(pos);
    if (diff == 0) {
      if (r.head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
        pos_out = pos;
        return &s;
      }
    } else if (diff < 0) {
      r.dropped.fetch_add(1, std::memory_order_relaxed);
      return nullptr;
    } else {
      pos = r.head.load(std::memory_order_relaxed);
    }
  }
}

}  // namespace detail

// Write a pre-formatted line (no prefix) to a specific destination file.
inline void emit_line_to(uint8_t dest, const char* line) {
  detail::ensure_started();
  auto& r = detail::ring();
  size_t pos;
  auto* s = detail::acquire(r, pos);
  if (!s) return;
  size_t len = std::strlen(line);
  if (len >= detail::kText) len = detail::kText - 1;
  std::memcpy(s->text, line, len);
  s->dest = dest;
  s->len = uint16_t(len);
  s->seq.store(pos + 1, std::memory_order_release);
}

// Standard record: "[   123.456 ms] #000042 tid=1234 CATEGORY message".
inline void emit(uint32_t cat, const char* fmt, ...) {
  char body[detail::kText];
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(body, sizeof(body), fmt, ap);
  va_end(ap);
  auto& r = detail::ring();
  const unsigned long long n = r.seq.fetch_add(1, std::memory_order_relaxed) + 1;
  char line[detail::kText + 96];
  std::snprintf(line, sizeof(line), "[%10.3f ms] #%06llu tid=%lu %s %s", detail::now_ms(), n,
                static_cast<unsigned long>(GetCurrentThreadId()), category_name(cat), body);
  emit_line_to(kDestMain, line);
}

// ---- Runtime Trace Recorder Control -----------------------------------------
inline bool IsRecording() {
  return mask() != 0;
}

inline const char* GetCurrentSessionFilename() {
  std::lock_guard<std::mutex> lk(session().mtx);
  return session().active_filename.c_str();
}

inline double GetSessionDurationSeconds() {
  if (!IsRecording()) return 0.0;
  return (detail::now_ms() - session().start_time_ms) / 1000.0;
}

inline uint64_t GetDroppedCount() {
  return detail::ring().dropped.load(std::memory_order_relaxed);
}

inline void StartRecording(uint32_t cat_mask = kAll) {
  SYSTEMTIME st;
  GetLocalTime(&st);
  char fname[64];
  std::snprintf(fname, sizeof(fname), "iu_trace_%04d%02d%02d_%02d%02d%02d.log",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  {
    std::lock_guard<std::mutex> lk(session().mtx);
    session().active_filename = fname;
    session().active_path = log_dir() + "\\" + fname;
    session().start_time_ms = detail::now_ms();
    session().marker_counter.store(0, std::memory_order_relaxed);
  }
  session().rotate_requested.store(true, std::memory_order_release);
  runtime_mask().store(cat_mask, std::memory_order_release);
  detail::ensure_started();
  emit(kGeneral, "---- IU TRACE RECORDER STARTED SESSION: %s (mask=0x%08X) ----", fname, cat_mask);
}

inline void StopRecording() {
  if (!IsRecording()) return;
  const double dur = (detail::now_ms() - session().start_time_ms) / 1000.0;
  emit(kGeneral, "---- IU TRACE RECORDER STOPPED (duration=%.3f s) ----", dur);
  runtime_mask().store(0, std::memory_order_release);
  session().close_requested.store(true, std::memory_order_release);
}

inline void AddMarker(const char* label = nullptr) {
  if (!IsRecording()) return;
  const uint32_t idx = session().marker_counter.fetch_add(1, std::memory_order_relaxed) + 1;
  emit(kGeneral, "======================== [MARKER #%u: %s] ========================",
       idx, (label && label[0]) ? label : "MANUAL_MARKER");
}

// ---- Guest memory helpers ---------------------------------------------------
// Guest addresses translate to host according to memory mapping:
// guest < 0xE0000000 -> base + guest
// guest >= 0xE0000000 -> base + guest + 0x1000
inline uint8_t* host_ptr(uint8_t* base, uint32_t guest) {
  uintptr_t offset = (guest >= 0xE0000000u) ? (static_cast<uintptr_t>(guest) + 0x1000u)
                                            : static_cast<uintptr_t>(guest);
  return base + offset;
}

// True if [guest, guest+len) is committed + readable host memory. Only used on
// the (already slow) enabled-trace path.
inline bool readable(uint8_t* base, uint32_t guest, uint32_t len = 4) {
  if (guest < 0x10000u) return false;
  const uint8_t* p = host_ptr(base, guest);
  MEMORY_BASIC_INFORMATION mbi;
  if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
  if (mbi.State != MEM_COMMIT) return false;
  if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
  const uint8_t* end = static_cast<const uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
  return p + len <= end;
}

inline bool peek8(uint8_t* base, uint32_t g, uint8_t& out) {
  if (!readable(base, g, 1)) return false;
  out = *(volatile uint8_t*)host_ptr(base, g);
  return true;
}
inline bool peek16(uint8_t* base, uint32_t g, uint16_t& out) {
  if (!readable(base, g, 2)) return false;
  out = __builtin_bswap16(*(volatile uint16_t*)host_ptr(base, g));
  return true;
}
inline bool peek32(uint8_t* base, uint32_t g, uint32_t& out) {
  if (!readable(base, g, 4)) return false;
  out = __builtin_bswap32(*(volatile uint32_t*)host_ptr(base, g));
  return true;
}

// Format helper: "0x%08X" or "<unreadable>" for a 32-bit guest field.
inline std::string field32_str(uint8_t* base, uint32_t g) {
  uint32_t v;
  char b[24];
  if (!peek32(base, g, v)) return "<unreadable>";
  std::snprintf(b, sizeof(b), "0x%08X", v);
  return b;
}

// ---- Higher-level trace helpers (templated on ctx to avoid include coupling) --
// Registers: r1, r3..r10, LR, CTR.
template <class Ctx>
inline void regs(uint32_t cat, const char* tag, Ctx& ctx) {
  if (!enabled(cat)) return;
  emit(cat, "%s REGS r1=%08X r3=%08X r4=%08X r5=%08X r6=%08X r7=%08X r8=%08X r9=%08X r10=%08X LR=%08X CTR=%08X",
       tag, ctx.r1.u32, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32, ctx.r8.u32,
       ctx.r9.u32, ctx.r10.u32, static_cast<uint32_t>(ctx.lr), ctx.ctr.u32);
}

// Object + vtable: guest ptr, host ptr, vtable ptr, first `nslots` slots, plus
// any extra slot indices in `extra_slots` (byte offsets / 4).
inline void object(uint32_t cat, const char* tag, uint8_t* base, uint32_t obj, uint32_t nslots = 4,
                   const uint32_t* extra_slots = nullptr, size_t n_extra = 0) {
  if (!enabled(cat)) return;
  uint32_t vt = 0;
  if (!peek32(base, obj, vt)) {
    emit(cat, "%s OBJECT guest=0x%08X host=%p vtable=<unreadable>", tag, obj, (void*)host_ptr(base, obj));
    return;
  }
  std::string slots;
  char b[40];
  for (uint32_t i = 0; i < nslots; ++i) {
    std::snprintf(b, sizeof(b), " [%u]=%s", i, field32_str(base, vt + i * 4).c_str());
    slots += b;
  }
  for (size_t i = 0; i < n_extra; ++i) {
    std::snprintf(b, sizeof(b), " [%u]=%s", extra_slots[i], field32_str(base, vt + extra_slots[i] * 4).c_str());
    slots += b;
  }
  emit(cat, "%s OBJECT guest=0x%08X host=%p vtable=0x%08X slots:%s", tag, obj, (void*)host_ptr(base, obj), vt,
       slots.c_str());
}

// Arbitrary 32-bit field: obj+off.
inline void field32(uint32_t cat, const char* tag, const char* name, uint8_t* base, uint32_t obj, uint32_t off) {
  if (!enabled(cat)) return;
  emit(cat, "%s FIELD %s (0x%08X+%u) = %s", tag, name, obj, off, field32_str(base, obj + off).c_str());
}

}  // namespace iu::trace

// Cheap-when-disabled convenience macros.
#define IU_TRACE(cat, ...) \
  do { if (::iu::trace::enabled(cat)) ::iu::trace::emit((cat), __VA_ARGS__); } while (0)
#define IU_TRACE_REGS(cat, tag, ctx) ::iu::trace::regs((cat), (tag), (ctx))
#define IU_TRACE_OBJECT(cat, tag, base, obj, ...) ::iu::trace::object((cat), (tag), (base), (obj), ##__VA_ARGS__)
#define IU_TRACE_FIELD32(cat, tag, name, base, obj, off) \
  ::iu::trace::field32((cat), (tag), (name), (base), (obj), (off))

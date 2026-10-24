// translation_cvars.h -- CVARs for the translation MOD (F8).
//
// Exposes the mod's options as ReXGlue CVARs, so they show up in the Community
// Debug overlay, in the TOML config and on the command line
// (`--es_translation=false`, etc.).  The text/texture hook reads them at
// runtime, falling back to the usual environment variables (IU_ES, IU_ES_*).
//
// Define in a SINGLE .cpp (main.cpp):
//     #include "translation_cvars.h"
//     IU_TRANSLATION_DEFINE_CVARS();
//
// And read them from anywhere:
//     bool on = iu::tr::EsEnabled();

#pragma once

#include <cstdlib>
#include <string>

#include <rex/cvar.h>

namespace iu::tr {

inline bool ParseBool(const std::string& v, bool def) {
  if (v.empty()) return def;
  return !(v == "0" || v == "false" || v == "False" || v == "FALSE" || v == "off" ||
           v == "no");
}

// Priority: CVAR (if touched via config/CLI/console) > environment variable >
// default value.  This way the existing scripts (IU_ES=1 ...) still apply
// when the CVAR is untouched, and the CVAR wins when set on purpose.
inline bool FlagByName(const char* name, const char* env, bool def) {
  if (rex::cvar::GetFlagSource(name) != rex::cvar::Source::kDefault) {
    return ParseBool(rex::cvar::GetFlagByName(name), def);
  }
  const char* e = std::getenv(env);
  if (e && e[0]) return !(e[0] == '0');
  return def;
}

inline std::string StrByName(const char* name, const char* env, const char* def) {
  if (rex::cvar::GetFlagSource(name) != rex::cvar::Source::kDefault) {
    std::string v = rex::cvar::GetFlagByName(name);
    if (!v.empty()) return v;
  }
  const char* e = std::getenv(env);
  return (e && e[0]) ? std::string(e) : std::string(def);
}

// --- mod options ---
inline bool EsEnabled()  { return FlagByName("es_translation", "IU_ES", true); }
inline bool EsTextures() { return FlagByName("es_textures", "IU_ES_SCAN", true); }
inline std::string EsCatalog() {
  return StrByName("es_catalog", "IU_ES_CATALOG", "translation/es_catalog.bin");
}
inline std::string EsTexturesManifest() {
  return StrByName("es_textures_manifest", "IU_ES_TEXTURES", "translation/ui_textures.txt");
}
// Target controller for icons/image (auto|xbox|ps5|steam).  In `auto` it is detected
// via SDL (VendorID from runtime.log); see mod/recomp/controller_es.h.
inline std::string EsController() {
  return StrByName("es_controller", "IU_ES_CONTROLLER", "auto");
}

}  // namespace iu::tr

// --- definition (only once, from main.cpp) ---
#define IU_TRANSLATION_DEFINE_CVARS()                                                       \
  REXCVAR_DEFINE_BOOL(es_translation, true, "Translation",                                  \
                      "Traduccion al castellano de los textos (0 = ingles original)")       \
      .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);                                   \
  REXCVAR_DEFINE_BOOL(es_textures, true, "Translation",                                     \
                      "Traducir tambien los rotulos que son TEXTURAS (HUD, menus, titulo)") \
      .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);                                   \
  REXCVAR_DEFINE_STRING(es_catalog, "translation/es_catalog.bin", "Translation",            \
                        "Ruta del catalogo de traduccion (es_catalog.bin)");                \
  REXCVAR_DEFINE_STRING(es_textures_manifest, "translation/ui_textures.txt", "Translation", \
                        "Ruta del manifiesto de texturas traducidas");                         \
  REXCVAR_DEFINE_STRING(es_controller, "auto", "Translation",                                  \
                        "Mando para iconos/imagen: auto|xbox|ps5|steam (auto = detecta SDL)")  \
      .lifecycle(rex::cvar::Lifecycle::kRequiresRestart)

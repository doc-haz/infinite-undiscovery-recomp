# Opciones del mod (F8) — dentro del propio recomp

Infinite Undiscovery (recomp) + mod de traducción al castellano.

**No hay un segundo ejecutable.** Todas las opciones del mod se configuran desde
la **misma UI gráfica del recomp** (el asistente / gestor de perfiles) y desde
los **CVARs**, y se persisten en `setup.json` / `<perfil>/config.json`.

---

## 0. Registro de mods (extensible)

Los mods se definen en una tabla `k_mods` (en `portable_setup.cpp`). Cada entrada
declara:

| Campo | Descripción |
|---|---|
| `id` | identificador estable usado en el JSON (p. ej. `"es"`) |
| `name` / `description` | etiquetas bilingües (pasan por `Text()`) |
| `installed_default` / `enabled_default` | estado por defecto |
| `options[]` | ajustes booleanos del mod: `{ cvar, label, valor }` |

Hoy el registro contiene **un mod**:

| id | Nombre | CVARs que controla |
|---|---|---|
| `es` | Traducción al Castellano | `es_translation` (texto) y `es_textures` (texturas) |

Para **añadir un mod nuevo** basta con declarar su flag persistente y añadir una
entrada a `k_mods`; el asistente y el gestor lo listan automáticamente, sin
tocar la UI.

---

## 1. Asistente de configuración (Asset Setup Wizard)

Se abre al configurar los archivos por primera vez (o con `--asset_setup`).

El indicador de progreso tiene **cuatro pasos**: `Disc 1` → `Disc 2` →
`DLC (Opcional)` → `Mods`.

El paso **Mods** es una página propia (`Siguiente` desde la página de medios):
lista los **mods instalables** con una **casilla** cada uno. El mod de
traducción puede además desglosarse en sus dos opciones:

| Opción | Clave en config | Efecto |
|---|---|---|
| **Texto en castellano** | `es_translation` | Textos (menús, diálogo, créditos…) en castellano |
| **Texturas en castellano** | `es_textures` | Rótulos bitmap (HUD, menús, título) en castellano |

Las opciones del mod solo son conmutables mientras el mod está activado
(si se desactiva, quedan atenuadas). `Atrás` vuelve a la página de medios y
`Instalar e iniciar` arranca la copia.

## 2. Gestor de perfiles de contenido (Content Profile Manager)

Es la pantalla para **relanzar y cambiar parámetros** con el juego ya instalado.
Se abre:

* al arrancar con `--profile_manager` (o `--asset_setup`);
* desde el juego con la tecla **F10** («Volver al Gestor de Perfiles») o desde la
  acción homónima del overlay **Community Debug** (que relanza el proceso).

El gestor tiene **dos pestañas** en la tarjeta principal:

* **Perfiles**: la lista de perfiles de contenido (como antes).
* **Mods**: la lista de **mods instalados**, cada uno con un **conmutador**
  activar/desactivar y, debajo, sus opciones.

Al pulsar **Iniciar perfil**, el cambio se aplica en el siguiente arranque.

Los valores se guardan en `run/setup.json` y en cada
`run/<PERFIL>/config.json` (junto a `language`, `active_profile`, …).

### Persistencia

```json
{
  "language": "es",
  "es_translation": true,
  "es_textures": true,
  "mods": {
    "es": { "installed": true, "enabled": true }
  }
}
```

* `mods.<id>.enabled` es el **interruptor maestro** del mod: si es `false`, la
  traducción **no** se aplica aunque `es_translation`/`es_textures` sigan a
  `true`.
* Las claves planas `es_translation` / `es_textures` se **siguen escribiendo**
  por compatibilidad con scripts y hook antiguos. Al leer un `config.json`
  antiguo **sin** bloque `mods`, el mod de traducción se considera activado si
  alguna de esas claves lo estaba.

## 3. CVARs (overlay + línea de comandos)

Definidas en `mod/recomp/translation_cvars.h` (categoría **Translation**):

```
es_translation           bool    true
es_textures              bool    true
es_catalog               string  "translation/es_catalog.bin"
es_textures_manifest     string  "translation/ui_textures.txt"
es_controller            string  "auto"   # auto|xbox|ps5|steam (iconos/imagen)
```

Aparecen en el overlay **Community Debug** y en la CLI:

```bat
InfiniteUndiscoveryRecomp.exe --no-es_textures
InfiniteUndiscoveryRecomp.exe --es_catalog translation/es_catalog.bin
InfiniteUndiscoveryRecomp.exe --es_controller ps5
```

> `es_controller` elige los **iconos de botón** y la **imagen del mapeo** según el
> mando.  En `auto` se detecta por SDL (VendorID del `runtime.log`); ver
> `docs/re/controller.md`.

### Prioridad de las opciones

```
CVAR (CLI/config/consola)  >  variable de entorno  >  asistente (setup.json)  >  defecto
```

Así los scripts antiguos (`IU_ES=1 IU_ES_SCAN=0 ./...exe`) siguen funcionando
cuando no se toca el CVAR, y lo elegido en la GUI manda por defecto.

---

## Flujo de datos

```
Asistente/Gestor (portable_setup.cpp)
        │  registro k_mods  +  toggles (mods / opciones)
        ▼
  g_mod_state[].enabled  ∧  g_es_translation / g_es_textures
        │  WriteConfig
        ├──────────────►  setup.json + <perfil>/config.json
        │                        │
        │  EsTranslation()/EsTextures()   (valor efectivo:
        │  mod activado && opcion activada)                 │ (siguiente arranque)
        ▼                                                    ▼
  hook de runtime (translation_es.h)  ◄────────  Initialize() lee setup.json
```

---

## Reproducir el build

`mod/recomp/build.sh` copia los headers del mod a `extract/f2/recomp/src/`
(incluido `translation_cvars.h`) y hace `touch` de `main.cpp`.

El parche sobre el proyecto recomp (hashes de DLL + `src/main.cpp` con los
`#include` del mod y `IU_TRANSLATION_DEFINE_CVARS()`, además de
`portable_setup.cpp/.h` y `setup_strings.inc` con el soporte de mods) está en
`mod/recomp/local-changes.patch`.

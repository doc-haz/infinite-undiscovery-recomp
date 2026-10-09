# Soporte multi-mando (detección + iconos/imagen por mando)

> **Estado:** implementado, **compilado** y **verificado offline**.  No
> verificado en juego (orden del usuario de no ejecutar el juego).
> Código: `mod/recomp/controller_es.h` (detección) + variantes en
> `tools/controller_skins.py`; integrado en el manifiesto IUT2 que consume
> `mod/recomp/ui_textures_es.h`.

## 0. Resumen

La consola original era Xbox 360: la interfaz dibuja **iconos de botón (A/B/X/Y,
LB/RB, LT/RT)** y una **imagen del mapeo del mando** (`26EF5000`, un mando Xbox
360 con líneas de llamada).  El mod ahora puede mostrar iconos e imagen acordes
al mando conectado:

| Mando | Iconos de botón | Imagen del mapeo |
|---|---|---|
| **Xbox** (por defecto) | A/B/X/Y, LB/RB, LT/RT (sin cambios) | original (sin sustituir) |
| **PS5 / DualSense** | ✕ ○ □ △, L1/R1, L2/R2 | DualSense dibujada |
| **Steam Controller** | A/B/X/Y, LB/RB, LT/RT (igual que Xbox) | Steam Controller dibujado |

Todo por **hook en runtime** (sin parchear `ud1.bin`/`ud2.bin`): solo se añaden
variantes a `translation/` y un selector en el manifiesto.

## 1. Detección del mando (`mod/recomp/controller_es.h`)

Opciones (CVAR / variable de entorno):

| CVAR | env | valores | defecto |
|---|---|---|---|
| `es_controller` | `IU_ES_CONTROLLER` | `auto`, `xbox`, `ps5`, `steam` (alias: `ps`, `playstation`, `dualsense`, `dualshock`, `valve`, `steamdeck`…) | `auto` |

En `auto` se reutiliza **el mismo dato que ya loguea ReXGlue**: el driver SDL
escribe en `runtime.log` una línea por mando conectado, p. ej.

```
SDL OnControllerDeviceAdded: "DualSense Wireless Controller", JoystickType(1), GameControllerType(3), VendorID(0x054C), ProductID(0x0CE6)
```

`iu::ctrl::Active()` lee el **último** `VendorID(0x…)` y lo mapea:

```
0x045E Microsoft -> xbox      0x054C Sony -> ps5      0x28DE Valve -> steam
cualquier otro -> xbox
```

Detalles de robustez:

* **Ruta del log**: `IU_TRACE_DIR/runtime.log` (lo fija `InfiniteUndiscoveryApp`
  en `SetupEnvironment`); respaldo al CVAR `log_file` y a `runtime.log` del cwd.
* **Barato y sin bloquear el juego**: mientras no esté resuelto se relee el log
  **como mucho una vez cada 500 ms** (`std::mutex`), y **a los 12 s sin mando se
  resuelve a `xbox`** (aspecto seguro por defecto).  La detección se cachea.
* **No usa SDL directamente**: los símbolos SDL no se exportan desde
  `rexruntime.dll` (comprobado), y enlazar una segunda copia de SDL arriesgaría
  el input.  El log es el canal fiable.
* Línea de traza propia: `CTRL: mando=ps5 (auto/log)`.

### 1.1 Alternativa forzada

`--es_controller=ps5`, `es_controller=steam` en `runtime.toml`, o
`IU_ES_CONTROLLER=ps5` fuerzan el mando sin leer el log.  Es el primer paso
aceptable si la lectura del log no funcionara en algún entorno.

## 2. Iconos de botón (`tools/controller_skins.py`)

Los iconos están en el **atlas de UI `000A4000_002_IMG.aif`** (1024²,
A8R8G8B8 = sin pérdida), en dos zonas (localizadas con componentes conexos por
color y coordenadas verificadas por rejilla):

* **Fila** (y≈639): `x = 596,628,660,692,724,756,788,820` (A·A·B·B·X·X·Y·Y),
  `852,884` (LB·LB), `916,948` (RB·RB).
* **Pila izquierda**: `x = 25,71,117,163,209,255`; filas `y = 742,792` (Rb/Lb)
  y `837,887` (A·B·X·Y·Rt·Lt).

Para **PS5** se reescribe el disco de cada icono (sombreado radial limpio, sin
el fantasma de la letra) y se dibuja la **forma** con supersampling 4×:

```
A -> ✕ (cruz, disco azul)      B -> ○ (círculo, disco rojo)
X -> □ (cuadrado, disco rosa)  Y -> △ (triángulo, disco verde)
LB->L1  RB->R1  LT->L2  RT->L2
```

(mapa posicional: A/B/X/Y y Y/X/B/A ocupan la misma posición que en el original,
por eso la forma corresponde al botón correcto).

**Steam** usa A/B/X/Y y LB/RB/LT/RT igual que Xbox → **no necesita variante de
atlas** (reutiliza `atlas_ui_es.aif`).

## 3. Imagen del mapeo (`26EF5000_000_IMG.aif`, 960×540 DXT5)

Se redibuja el mando (silueta suavizada con Chaikin 4×, sticks, cruceta,
botones, gatillos/bumpers) **conservando las líneas de llamada azules
originales** y colocando los controles en las **mismas posiciones** que los
puntos azules del original, de modo que los rótulos dinámicos (ya traducidos)
siguen alineados.  La capa azul se aísla por color/alpha (líneas azul oscuro
semitransparentes + puntos azul claro) excluyendo el disco azul del botón X.
Se re-codifica el DXT5 completo conservando cabecera/tamaño.

* Xbox: **no se sustituye** (el original ya es Xbox).
* PS5 → `controllermap_ps5_es.aif`;  Steam → `controllermap_steam_es.aif`.

> **Decisión de diseño:** las líneas/posiciones se mantienen (como pidió el
> enunciado); por eso el DualSense dibujado usa la disposición de sticks del
> mando original.  Es una limitación consciente.

## 4. Integración (manifiesto IUT2)

`tools/ui_textures.py manifest` escribe ahora **IUT2**, con una columna de mando:

```
IUT2
<n>
<sig_hex(0x30B)> <len> <fnv64_hex> <ctrl> <ruta>
```

`<ctrl>` = `*` (todas) o `xbox`/`ps5`/`steam`.  El hook
(`mod/recomp/ui_textures_es.h`) lee IUT2 (y sigue leyendo IUT1 antiguo, con
`*`).  Reglas:

1. Las entradas con mando se **ordenan antes** que las genéricas (`*`) para que
   la variante específica gane sobre la del mismo origen.
2. Si `es_controller=auto` y el mando **aún no está resuelto**, las entradas con
   mando **se difieren** (no se marcan hechas) para no pintar la variante
   equivocada; el atlas `xbox`/`steam`/`ps5` y los mapas esperan a la detección.
3. Al aplicar una variante, se marcan como hechas **las demás del mismo origen**
   (misma firma+hash), de modo que la genérica no la pise.

Entradas de mando que emite el manifiesto:

```
atlas (000A4000_002):  xbox  -> atlas_ui_es.aif
                       steam -> atlas_ui_es.aif         (mismos botones que Xbox)
                       ps5   -> atlas_ui_ps5_es.aif     (iconos PS)
mapa  (26EF5000):      ps5   -> controllermap_ps5_es.aif
                       steam -> controllermap_steam_es.aif
                       (xbox: sin entrada -> se deja el original)
```

## 5. Reproducción / verificación

```bash
# generar variantes (usa el atlas ES ya renderizado como base)
python -X utf8 tools/ui_textures.py render --out extract/f2/ui_es
python -X utf8 tools/controller_skins.py all --out extract/f2/ui_es
# manifiesto IUT2 + despliegue
python -X utf8 tools/ui_textures.py manifest --out extract/f2/ui_es \
       --deploy extract/f2/run/translation
# verificacion offline
python -X utf8 tools/verify_controller.py
bash mod/recomp/build.sh
```

`tools/verify_controller.py` comprueba:
1. detección por VendorID (fixtures Microsoft/Sony/Valve + el `runtime.log` real);
2. el manifiesto IUT2 contiene las 3 variantes y sus rutas;
3. cada variante tiene **el mismo tamaño y la misma firma** que el origen y
   decodifica.

Salida esperada: `RESULTADO: OK`.

### Evidencia (offline)

| Fichero | Contenido |
|---|---|
| `extract/f2/ui_es/controller_maps_cmp.png` | Xbox vs PS5 vs Steam (lado a lado) |
| `extract/f2/ui_es/controller_buttons_cmp.png` | iconos Xbox (arriba) vs PS5 (abajo) |
| `extract/f2/ui_es/controllermap_ps5_preview.png` | mapa PS5 (pre-encode) |
| `extract/f2/ui_es/controllermap_steam_preview.png` | mapa Steam |
| `extract/f2/ui_es/atlas_ui_ps5_row.png`, `..._stack.png` | iconos PS en el atlas |
| `extract/f2/run/translation/ui_textures.txt` | manifiesto **IUT2** (62 texturas) |

## 6. Qué queda sin cubrir / riesgos

* **No verificado en juego** (orden de no ejecutar).  Pendiente de un run real
  con mando PS5/Steam para confirmar la sustitución de atlas/mapa.
* **Timing de detección vs. subida de textura**: si el escáner encontrara el
  atlas *antes* de que SDL registre el mando (línea ~7 s tras arrancar; el atlas
  se ve a ~12 s) la variante PS5 se diferiría; en el caso normal la detección ya
  está resuelta.  Si el mando se conecta tarde, `xbox` (u original) hasta la
  próxima recarga del atlas.
* **Iconos pequeños**: el re-pintado de iconos de 26–34 px es una aproximación
  estilizada; puede haber mínimas diferencias de tono/borde respecto al original.
* **Otros prompts de botón**: se cubren el atlas de UI (`000A4000_002`).  No se
  ha encontrado otra textura con iconos A/B/X/Y (el inventario de §9.1b de
  `ui-textures.md` ya cerró la búsqueda); si apareciera alguno en juego, se
  añadiría como variante.

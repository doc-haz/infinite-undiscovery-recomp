# Rótulos que son TEXTURAS — inventario, método e integración (F5/F6)

> **Estado:** traducidas e integradas **por hook de runtime** (sin parchear los
> assets): atlas de UI (CAMP + combate/HUD), **menú principal** (`title_menu` /
> `title_continue`, AIFs incrustados en el ASF del título; sustituidos **en la
> carga del ASF** vía `OnAif`, §3.3), `GAME OVER`,
> `Now Loading…`, `Reload.`, los **acentos del banco 005** (atlas **y** métricas
> del `.msg`) y las **51 pantallas de ayuda del CAMP** (1280×720, §9). El menú
> principal y las pantallas se localizaron **offline** por contenido
> (`tools/scan_aif_text.py`); ver §6 y §9.
>
> **Novedad (TAREA 1 + TAREA 2):** el menú principal se sustituye **al cargar el
> ASF** (`OnAif` sobre `sub_821EF618/678`), no por escáner tardío → **verificado
> en juego** («Pulsa START», «Nueva partida», «Continuar», «Opciones»).  Y
> `TARGET` se dibuja **glifo a glifo** (6 sprites), así que se pinta una letra
> por sprite → `BLANCO` completo (§4b).
>
> Documentos relacionados: `second-text-path.md` (por qué son texturas),
> `glyph-extension.md` (acentos offline), `runtime-hook.md` (hook de texto).

## 0. Resumen

Los rótulos fijos **no** pasan por el parser de mensajes (`sub_826D52C0`): son
píxeles horneados en texturas `AIF` (ver `second-text-path.md`). Se traducen
pintando el castellano sobre el atlas con `tools/ui_textures.py` (mismo tamaño,
re-codificación sin pérdida) y se **sustituyen en memoria guest por su firma**
desde `mod/recomp/ui_textures_es.h`, generalizando el mecanismo que ya integraba
el atlas de UI.

```
tools/ui_textures.py render   ->  extract/f2/ui_es/*.aif     (pintado, sin pérdida)
tools/ui_textures.py manifest ->  extract/f2/run/translation/{*.aif, ui_textures.txt}
mod/recomp/ui_textures_es.h   ->  sustituye cada textura por su FIRMA en memoria
```

## 1. Inventario (recurso → EN → ES)

La fuente de cada textura es un `AIF` de `extract/f2/all_d1ud1` (o de `extract/f0`
para los bancos `RMD-`). Registro completo en `tools/ui_textures.py::_reg()` y
`_extra_assets()`.

### 1.1 Atlas de UI `000A4000_002_IMG.aif` (= `26D8C000_000_IMG.aif`, PG02, 1024², A8R8G8B8)

| Recurso (caja) | EN | ES |
|---|---|---|
| pestañas CAMP | Items, Skills, Equipment, Creation, Status, Map, Party, Options, Data, Talk | Objetos, Habs., Equipo, Creación, Estado, Mapa, Grupo, Opciones, Datos, Hablar |
| sub-pestañas | Battle, Connect, Flute, Magic, Personal | Combate, Conexión, Flauta, Magia, Personal |
| barra 3 | Enchant, Save, Load, System, Tutorial | Encantar, Guardar, Cargar, Sistema, Tutorial |
| combate | Player Advantage! | Ventaja aliada |
| combate | Enemy Advantage! | Ventaja enemiga |
| combate | Level Up | Subir nivel |
| combate | TARGET (glifo a glifo) | BLANCO |
| combate | GUARD | GUARDIA |
| combate | CRITICAL | CRÍTICO |
| combate | Situation Bonus! | Bonif. situación |
| combate | CONNECT | CONEXIÓN |
| HUD (×2) | HP MP ATKDEFINT HITAGL | PS PV ATQDEFINTPREAGI |
| barra inferior | Pause / Now Loading… | Pausa / Cargando… |

### 1.2 Texturas independientes

| Fichero (`extract/f2/all_d1ud1/`) | Tamaño/formato | EN | ES |
|---|---|---|---|
| `274AE800_000_IMG.aif` | 640×480 A8R8G8B8 | GAME OVER | FIN DEL JUEGO |
| `28EB4000_000_IMG.aif` | 256×32 A8R8G8B8 | Now Loading… | Cargando… |
| `26F88000_000_IMG.aif` | 256×256 A8R8G8B8 | Reload. | Recargar. |

> `00016000_000_IMG.aif` es **byte-idéntico** a `28EB4000_000_IMG.aif` (la misma
> textura), por eso no tiene entrada propia.

### 1.2b Menú principal (AIFs **incrustados** en `0001F800_000_MESH.asf`)

Los rótulos del título **no** son `.aif` sueltos: son dos `AIF ` A8R8G8B8
incrustados en el ASF `extract/f2/all_d1ud1/0001F800_000_MESH.asf`
(`base` = offset dentro del ASF → los píxeles empiezan en el siguiente límite de
0x1000 del contenedor). Se localizaron por **contenido** con
`tools/scan_embedded_aif.py` (decodifica cada `AIF ` incrustado y pasa OCR).

| AIF (offset en el ASF) | Tamaño | Rótulos | ES |
|---|---|---|---|
| `@0x2F51F0` (`title_menu`) | 895×201 | Easy, Normal, Hard, VeryHard, Options, New Game, Load Game, Xbox LIVE Marketplace, PRESS START button | Fácil, Normal, Difícil, Muy difícil, Opciones, Nueva partida, Cargar partida, Xbox LIVE Marketplace, Pulsa START |
| `@0x3D21F0` (`title_continue`) | 415×54 | Infinity, Continue | Infinity, Continuar |

`copyrgiht` (línea legal) y los iconos ornamentales (`Icon…`) se dejan intactos.
`PressStart2` reusa la región `PRESS START` del atlas `title_menu`.

### 1.3 Acentos (banco 005)

`d1_ud1_000A4000_005` (fuente latina principal, 768², DXT2/3). El atlas original
**no** tiene `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡`; se añaden en las 16 celdas libres
finales (códigos **858–873**) con `tools/add_accents_banks.py`, **sin cambiar
`count`** (huella del banco estable → el catálogo sigue casando).

| | origen (firma) | sustituto |
|---|---|---|
| atlas | `extract/f0/d1_ud1_000A4000_005.aif` | `extract/f2/accents/d1_ud1_000A4000_005_accents.aif` |
| `.msg` (métricas) | `extract/f0/d2_ud1_000A4000_005.msg` | `extract/f2/accents/d1_ud1_000A4000_005_accents.msg` |

> **Ojo con el `.msg` "original":** `add_accents_banks.py --sync-f0` sobreescribió
> `extract/f0/d1_ud1_000A4000_005.msg` con las métricas de acento. La copia **sin
> parchear** (idéntica salvo los 32 B de las métricas 557–572, con ceros) es
> `extract/f0/d2_ud1_000A4000_005.msg`; es la que casa con la memoria del juego,
> así que el manifiesto usa **esa** como firma/hash. En memoria, el atlas y el
> `.msg` se cargan juntos en el `RMD-`: `OnMsg` sustituye el `.msg` en `msgbase`
> y el atlas en `msgbase - aif_len`.

## 2. Método de pintado (`tools/ui_textures.py`)

* **Decodifica** el `AIF` (`tools/vendor/.../aif.py`) y pinta el rótulo ES con
  PIL respetando estilo/color/caja (`STYLES`, cajas en `_reg()`).
* **Re-codifica** conservando formato y tamaño:
  * `A8R8G8B8` → edición 1:1 byte-exacta (`rebuild_argb8888`).
  * `DXT1/2/3/4/5` → re-codifica **sólo** los bloques 4×4 tocados
    (`tools/aif_edit.py`); el resto del atlas queda intacto.
* La fuente se **reduce** para caber en el hueco disponible (reservando el margen
  de contorno+halo) y nunca invade al rótulo vecino.
* Verificación por textura: `sin pérdida` (A8R8G8B8) y `envoltorio OK`
  (tiling+byte-order exactos en el round-trip).

## 3. Integración en runtime (`mod/recomp/ui_textures_es.h`)

El manifiesto `translation/ui_textures.txt` (generado por
`ui_textures.py manifest`) lista, por textura:

```
IUT1
<n>
<sig_hex(0x30 B)> <len> <fnv64_hex> <ruta>
```

> **Novedad multi-mando (formato IUT2):** ahora el manifiesto es
> `IUT2 <n> <sig> <len> <fnv64> <ctrl> <ruta>`, donde `<ctrl>` = `*` (todas) o
> `xbox`/`ps5`/`steam`.  El atlas de UI y el mapa del mando (`26EF5000`) tienen
> variantes por mando (generadas por `tools/controller_skins.py`); el hook las
> ordena antes que `*`, las difiere hasta que se detecte el mando y marca como
> hechas las del mismo origen.  Detalle: `docs/re/controller.md`.

* **Firma** = cabecera `AIF`/`.msg` de 0x30 B (longitud + id + nº de recurso);
  **hash FNV-1a de 64 bits** del fichero entero para desambiguar (p. ej. `Now
  Loading` EN/JP comparten cabecera).
* Dos vías de localización en memoria guest:
  1. `iu::ui::OnMsg(base, msgbase)` — llamada desde `TrySubstitute` (que ya tiene
     la base del `.msg`): sustituye el `.msg` (métricas) y el atlas que le
     precede (`msgbase - aif_len`). Cubre las fuentes `RMD-` (**acentos 005**).
  2. `iu::ui::Tick(base)` — arranca **una sola vez** un **hilo** que escanea por
     firma las texturas persistentes (atlas UI, GAME OVER, Now Loading, Reload) y
     se repite mientras queden pendientes (1.ª pasada rápida, luego cada ~8 s,
     hasta un máximo de ~400 pasadas).
* `IU_ES_SCAN=0` desactiva el escaneo; `IU_ES_TEXTURES=<ruta>` cambia el
  manifiesto.
* **No** se toca ningún asset del juego: todo es memoria guest.

### 3.1 El cuelgue (diagnóstico y arreglo)

La 1.ª versión del escaneo se ejecutaba **en el hilo del juego** con
`VirtualQuery` **una vez por página** de 4 KB sobre `0x80000000..0xF0000000`
(458 752 llamadas). Medido en un run: **~17 s** sólo en `VirtualQuery` (≈25 µs
por llamada), más el barrido por 16 B dentro de cada página; el arranque quedaba
en negro. Por encima de `0xE0000000` (alias físico/MMIO) la lectura era aún más
lenta (17 s por 64 MiB). Además, la vía `OnAif` llamaba a **`sub_821EF678`**,
que **no es el cargador de `AIF `** (su cuerpo es otra cosa y no se invoca en el
arranque): se ha eliminado.

Arreglo (estable, verificado):

* **Se itera por REGIONES**: `VirtualQuery` una vez por región y salto por
  `mbi.RegionSize` (decenas de llamadas, no 458 752).
* Dentro de cada región, `memchr` (SIMD) localiza los candidatos `"AIF "`.
* El barrido corre en un **`std::thread`** (`ScannerLoop`), nunca en el hilo del
  juego. El único estado compartido es `Entry::done` (atómico).
* Rango de datos guest `[0x80000000, 0xF0000000)`: se amplió desde `0xE0000000`
  porque el ASF del **menú principal** (con sus AIF incrustados) y sus elementos
  viven por encima (`~0xE67xxxxx`). El barrido por regiones lo mantiene barato.
* Resultado: 1.ª pasada **~0.5 s** (en el hilo del escáner), sin bloquear el
  juego; el log muestra `UI: scan END regions=19 aif=606 477 ms` con **otro
  `tid`** que el del juego.

### 3.2 Cambios en `translation_es.h` (mínimos, avisados)

1. `#include "ui_textures_es.h"` tras `iu_trace.h`.
2. En `TrySubstitute`: `iu::ui::OnMsg(base, msgbase);` tras resolver `msgbase`.
3. En `sub_826D52C0`: `if (want_scan) iu::ui::Tick(base);`.
4. Se **elimina** el override de `sub_821EF678` (no era el cargador de `AIF `).

### 3.3 Tercera vía: sustitución en la **carga del ASF** (`OnAif`) — TAREA 1

El escáner por firma (§3) encontraba el AIF del **menú principal** pero
**demasiado tarde**: el ASF del título se carga entero en memoria guest y el
motor crea/sube la textura en el acto, así que cuando el hilo escáner llegaba
(~30 s después) la textura ya estaba en la GPU y el título seguía en inglés.

**Dónde carga el motor el ASF.** El ASF `0001F800_000_MESH.asf` se lee de
`ud1.bin` y se deposita completo en memoria guest (p. ej. base `0xEB72E000` en
un run). El motor recorre sus chunks con `sub_822226D8` / `sub_8221D768`: al ver
el tag `AIF ` lo despacha a **`sub_821EF618`** / `sub_821EF678`, que registran la
textura **antes** de crearla/subirla.  (Medido: ASF cargado a los ~57 s; los 8
AIF incrustados registrados uno a uno en `sub_821EF618`; `title_menu` en
`base+0x2F51F0`, `title_continue` en `base+0x3D21F0`.)

**Arreglo.** `iu::ui::OnAif(base, r4)` se llama desde los overrides de
`sub_821EF618`/`sub_821EF678` **antes** del original: recorre el manifiesto y, si
el AIF apuntado por `r4` casa por firma+FNV, lo sustituye en el sitio (misma
longitud).  La textura se crea ya en castellano; el escáner queda como red de
seguridad para el resto de texturas (atlas, GOT, pantallas de ayuda…).  Coste
despreciable: como las 58 firmas del manifiesto son **únicas**, por cada AIF
registrado hay a lo sumo un `memcmp` de 0x30 B y, si casa, un FNV-64.

Evidencia en vivo (`title_test.log`, `IU_ES_AIFPROBE=1`):

```
UI AIF 822226D8 r3=EB72E000 [r3+16]=00010140 tag0='ao__'
UI AIF 821EF618 r3=E5BF0A90 r4=EB72E0D0 tag='AIF '
...
UI AIF 821EF618 r3=E5BF0A90 r4=EBA231F0 tag='AIF '     # title_menu
UI AIF 821EF618 r3=E5BF0A90 r4=EBB001F0 tag='AIF '     # title_continue
UI: SUSTITUIDA translation/title_menu_es.aif @ EBA231F0 (806416 B)
UI: SUSTITUIDA translation/title_continue_es.aif @ EBB001F0 (110096 B)
```

y captura del título: **«Pulsa START» / «Nueva partida» / «Continuar» /
«Opciones» / «Xbox LIVE Marketplace»** (`extract/f2/run/title_test_shots/`).


## 4. Acentos: pipeline de texto

`tools/build_es_codes.py`:

* `find_msg()` **prefiere** `extract/f2/accents/<stem>_accents.msg` → el reflow usa
  la métrica real de los acentos.
* `max_code = 300 + celdas_del_atlas` **sólo si** existe el atlas ampliado
  integrado (`extract/f2/accents/<stem>_accents.aif`); si no, `300 + count`
  (seguro: nunca emitir códigos sin glifo). Así entran los códigos con celda en
  el atlas (858–873 en `005`, `count=557` → `300+576=876`). El charmap ya los
  mapea.
* **Cambio compartido** (avisado): era `300 + struct.unpack_from(">I", b, 0x34)`
  (PLEGABA los acentos mientras el atlas no estuviera integrado). Ahora incluye
  las celdas del atlas cuando el atlas ampliado está presente.

Resultado: `d1_ud1_000A4000_005` emite ~17 255 glifos acentuados (incl. `¿` 872 y
`¡` 873, 1981 y 3248 usos), que el atlas ampliado dibuja y las métricas del
`.msg` parcheado (sustituido en runtime) dimensionan. Prueba offline:
`tools/verify_accents_render.py` confirma que las 16 celdas (557–572) tienen
tinta y métrica propia.

### 4a. Forma de los diacríticos (raya blanca corregida)

`tools/add_accents.py` dibujaba los diacríticos con un **borde negro grueso**
(`width + 3`) sobre un núcleo blanco: la aguda quedaba como un trazo corto y
grueso que en el juego se leía como una **raya/barra** sobre la letra. Arreglo
en `add_accents.py` (lo usa también `add_accents_banks.py`):

* `draw_stroke`: borde fino (1 px por lado, `width + 2`).
* `add_acute`: aguda **diagonal fina** (núcleo 2 px, ~60°, de `(cx-4, 8)` a
  `(cx+3, 1)`), como la aguda tipográfica — no una barra.
* `add_diaeresis`: dos puntos redondos con borde fino.
* `add_tilde`: virgulilla con **más amplitud** (`amp=3.5`, `w=16`) para que no
  parezca una línea horizontal.

Regenerado con
`python -X utf8 tools/add_accents_banks.py --stems d1_ud1_000A4000_005` y
verificado offline (`verify_accents_render.py` →
`extract/f2/accents_offline_proof.png`: `El niño comió café con azúcar. ¡Qué
frío! ¿Dónde está el pingüino? ÁÉÍÓÚÜÑ`).

## 4b. Encaje de rótulos (sin solapes ni desbordes)

`tools/ui_textures.py`:

* El **espaciado con `tracking`** (estilos `target`, `reload`) avanzaba por el
  **ancho de tinta** (`getbbox`) de cada glifo → las letras se **solapaban**
  (p. ej. `OBJETIVO`). Ahora se avanza por el **ancho de avance real**
  (`font.getlength`), también al medir para `fit_font` y al dimensionar la capa.
* **`TARGET` → `BLANCO` — corte en el juego corregido (causa raíz real).** El
  motor **no** muestrea una única región del atlas: dibuja la palabra **glifo a
  glifo**, con **seis sprites independientes** de 22-24×26 px.  Se descubrió
  leyendo la **tabla de sprites** del atlas, `000A4000_003_TTD.bin` (magic
  `DTT\0`, entradas LE de 16 B: `x0,y0,x1,y1,w,h,sid,flag` con las coordenadas
  en **1/4 de píxel**).  Ahí, `TARGET` son seis entradas con las cajas
  `(293.5,484.5)-(314.5,509.5)`, `(325.5,…)`, … `(453.5,…)-(474.5,…)`, una por
  letra.  Por eso pintar `OBJETIVO`/`BLANCO` como **una sola banda** producía
  trozos («OB ET VO»): cada sprite recortaba su trozo de la palabra.  **Arreglo:**
  `_reg()` pinta **una letra por sprite** (`T→B, A→L, R→A, G→N, E→C, T→O`), cada
  una centrada en su caja TTD.  `TARGET` y `BLANCO` tienen 6 letras, encaje
  exacto.  Prueba: `extract/f2/ui_es/_target_glyph_fix.png`.
  (Las demás etiquetas del atlas **sí** son un único sprite — p. ej. las
  pestañas del CAMP, `Options`… —; por eso sólo `TARGET` necesitaba esto.)
  * **`OBJETIVO` (8 letras) NO cabe:** el motor dibuja **exactamente 6 sprites**
    (posiciones fijas de la TTD, no una fuente con un glifo por letra: las 6
    letras `T,A,R,G,E,T` son sprites *dedicados* a `TARGET`, fuera de la rejilla
    de la fuente).  Repartir `OBJETIVO` en 6 cajas obliga a 2 letras por caja,
    que salen **más pequeñas, con huecos desiguales y recortadas** — se probaron
    varias distribuciones (`extract/f2/ui_es/_target_variants*.png`) y ninguna
    queda digna.  Por eso se **mantiene `BLANCO`** (6 letras = 6 sprites,
    encaje exacto).
* **Menú principal: alineación y tamaño unificados.** En el título, el motor
  también dibuja cada rótulo **centrado en pantalla**.  Cambios en `_reg()`:
  * Las cajas son el **bbox real de la tinta** del original (medido con
    componentes conexos sobre el AIF del ASF).
  * `center: True` en todos los rótulos del título: el ES se centra en la misma
    caja → misma posición que el inglés (antes `Pulsa START`, `Opciones` y
    `Xbox LIVE Marketplace` quedaban **alineados a la izquierda** y salían
    descentrados).
  * **`Pulsa START` (caso especial):** el motor **no** muestrea la banda
    `PRESS START button` completa, sino **sólo `PRESS START`** (el rótulo se
    llama `PressStart`). Centrar en la banda entera (centro 502) desplazaba
    «Pulsa START» ~100 px a la derecha en pantalla. La caja de pintado es el
    ink de `PRESS START` (`290..574`), con un ajuste fino a **centro 450**
    (un pelín a la derecha del ink original 432, pedido por el usuario); y
    `clear_box` borra además la palabra inglesa `button` (que el motor no
    muestrea). Verificado en pantalla: centro ≈ 1305 px (1280 = centro).
    Comparativa: `extract/f2/ui_es/_pulsa_fine_cmp.png`.
  * El tamaño de fuente se **armoniza globalmente por estilo** (`_harmonize_sizes`,
    compartido entre specs): antes `title_menu` daba 30 y `title_continue` 39, y
    `Continuar` salía **más grande** que el resto del menú.  Ahora ambos a 30.
  * Prueba: `extract/f2/ui_es/title_menu_cmp.png`, `title_continue_cmp.png`
    (líneas azules = centro de caja, coinciden origen/ES).
* Revisados con montaje lado a lado origen/ES: `VENTAJA ALIADA/ENEMIGA`,
  `SUBIR NIVEL`, `GUARDIA`, `CRÍTICO`, `BONIF. SITUACIÓN`, `CONEXIÓN`, fila de
  stats, `Pausa` y `Cargando…`. Todos caben en su hueco.
* Re-render + verificación visual:
  `python -X utf8 tools/ui_textures.py render --only atlas_ui --preview`.

## 4c. Encabezados `~ <título> ~` del CAMP (TAREA — ancho del texto)

**Síntoma:** en el CAMP, los encabezados de subpantalla salen como `~ Equipo ~` /
`~ Tutorial ~`, pero el adorno `~` de **cierre** queda muy separado de la palabra
(el izquierdo cerca, el derecho lejos).

**Cómo compone el motor el encabezado.** El título es **texto dinámico** (lo
traduce el hook).  Los `~` son **sprites decorativos** del encabezado, a
**posiciones fijas** de pantalla; el título se dibuja **centrado** entre ellos.
El centro se calcula con el **ancho resuelto** `obj+0xC4`, que `sub_826D52C0`
copia del **registro del `.msg`** — y ese ancho se calculó sobre el **inglés**.
Al sustituir el texto, el hook cambiaba el puntero pero **no el ancho**, así que
un título ES **más corto** que el inglés se dibuja desplazado a la izquierda y el
`~` de cierre se aleja.

Medido offline (banco `005`, ancho del registro EN vs ancho ES recalculado):

| id | EN → ES | ancho EN | ancho ES | desplazamiento |
|---|---|---:|---:|---:|
| 10051 | `Equipment` → `Equipo` | 102 | 65 | **18 u a la izquierda** |
| 10059 | `Options` → `Opc.` | 75 | 46 | 14 u |
| 48 | `Options` → `Opciones` | 78 | 90 | −6 u (a la derecha) |
| 47 | `Continue` → `Continuar` | 90 | 93 | −2 u |

(La pantalla de **Options** del título salía bien porque `Opciones` es *más
largo* que `Options`; el caso roto es el título *más corto* que el inglés.)

**Arreglo (`mod/recomp/translation_es.h`).** Tras sustituir el texto, `FixWidth`
recalcula el ancho ES y lo escribe en `obj+0xC4` **y** en el registro del `.msg`:

* La **métrica** de cada glifo (tabla `metrics_off`, 1 byte) es su avance; el
  ancho escala con la **suma de métricas** (el registro aplica además un factor de
  kerning ~0.87 constante que se cancela al escalar).  Así:
  `ancho_ES = ancho_EN · (Σ métricas ES / Σ métricas EN)`.
* Solo para textos **cortos** (`≤ 24` códigos: rótulos/encabezados centrados); los
  diálogos largos no se centran y no lo necesitan.
* El renderizador **no** lee `obj+0xC4`, así que esto **no** cambia el dibujado
  del texto: solo corrige el centrado/posición de quien lo consulta.

**Verificación:** la tabla anterior es la prueba **offline** (la causa y el valor
corregido).  No se ha podido verificar en juego (el usuario pidió no ejecutar el
juego); el usuario debe confirmarlo.

## 5. Reproducción

```bash
# 1) pintar texturas + desplegar manifiesto al layout de ejecución
python -X utf8 tools/ui_textures.py render --out extract/f2/ui_es
python -X utf8 tools/ui_textures.py manifest --out extract/f2/ui_es \
       --deploy extract/f2/run/translation

# 2) acentos (offline) del banco 005 -> extract/f2/accents/
python -X utf8 tools/add_accents_banks.py --stems d1_ud1_000A4000_005

# 3) pipeline de texto (usa acentos) + catálogo
python tools/build_es_codes.py

# 4) compilar + ejecutar
bash mod/recomp/build.sh
bash mod/recomp/run_es_test.sh

# round-trip obligatorio del códec de texturas
python -X utf8 tools/aif_edit.py selftest
python -X utf8 tools/ui_textures.py verify extract/f2/ui_es/atlas_ui_es.aif
```

Todo el ciclo está en `mod/recomp/qa.sh` (valida + genera + texturas + build + run).

## 6. Menú principal (RESUELTO, verificado en juego)

Los rótulos del título (`NewGame`, `Continue`, `OPTIONS`, `XboxLive`,
`EASY/NORMAL/HARD/VERY_HARD`, `PressStart`, `LoadGame`, `PressStart2`,
`Infinity`, `copyrgiht`, `Icon…`) se cargan **por nombre** (`CTitleLogoTask` →
`sub_824999A0` → 22× `sub_82182070`). El nombre sólo aparece en
`default.exe@0x8203AB44` y en la tabla de nodos de `ud1.bin@0x9A210`.

**Dónde están las texturas:** no son `.aif` sueltos en los discos, sino **AIFs
incrustados** en el ASF del título `0001F800_000_MESH.asf` (§1.2b). Por eso los
escaneos OCR de los `.aif` de primer nivel no los encontraban.

**Método offline (sin ejecutar el juego):** `tools/scan_embedded_aif.py` recorre
todos los ficheros extraídos de ambos discos, localiza cada firma `AIF `
(también dentro de contenedores ASF/MRON/BIN), decodifica el bitmap y le pasa
OCR (tesseract). Filtrado a `A8R8G8B8` (rótulos de UI) dio los dos atlases del
título. El resultado crudo está en `extract/f2/re_analysis/emb_scan.out`.

**Integración (¡ojo, la clave!):** las dos entradas (`title_menu`,
`title_continue`) están en `_reg()` con `source` = el ASF y `offset` = la
posición del AIF incrustado. El pintado preserva el `base` (los píxeles siguen
empezando en el mismo offset relativo a `AIF `), de modo que la **firma de 0x30 B**
coincide byte a byte.  Pero la sustitución **por escáner** llegaba tarde (§3.3);
la vía buena es **`OnAif`**, enganchada a `sub_821EF618`/`sub_821EF678` en el
momento en que el motor **registra** cada AIF del ASF, antes de crear/subir la
textura.  **Sin tocar el ASF ni `ud1.bin`.**

La sonda `IU_ES_NAMEPROBE` sigue disponible (ahora **desactivada por defecto**;
se activa con `IU_ES_NAMEPROBE=1`) para confirmar en runtime los nombres y las
direcciones `AIF @ elem+XX`; con el atlas ya identificado no es necesaria.  La
sonda nueva `IU_ES_AIFPROBE=1` registra los chunks `AIF `/`ao__` que despachan
`sub_821EF618/678/822226D8/8221D768`.

## 7. Evidencia

* `extract/f2/ui_es/*_preview.png` — renders ES (atlas completo, GAME OVER, …).
* `extract/f2/accents/d1_ud1_000A4000_005_accents_{row,demo,cells}.png` — acentos.
* `extract/f2/accents_ingame_proof.png` — **prueba EN JUEGO** del disclaimer real
  (bancos 005): *«Este juego es ficción. …»* con la `ó` de **ficción** dibujada de
  verdad desde la celda de acento (858+) y su métrica sustituida. Sin recuadros.
* `extract/f2/accents_offline_proof.png` — frase real con acentos compuesta **como
  el motor** (`verify_accents_render.py`): `El niño comió café con azúcar. ¡Qué
  frío! ¿Dónde está el pingüino?` + `ÁÉÍÓÚÜÑ`. Confirma celda+métrica de 858–873.
* `extract/f2/run/translation/ui_textures.txt` — manifiesto (**8 texturas**,
  incl. `title_menu`/`title_continue` y el `.msg` de métricas de acentos).
* `extract/f2/re_analysis/emb_scan.out` — hallazgo **offline** de los atlases del
  título (OCR de AIFs incrustados).
* `extract/f2/ui_es/title_menu_cmp.png`, `title_continue_cmp.png` — **origen vs
  ES** del menú principal con los **centros de caja** marcados (coinciden).
* `extract/f2/ui_es/_target_glyph_fix.png` — `TARGET` original vs `BLANCO` ES
  **glifo a glifo** (cajas TTD en verde).
* `extract/f2/run/title_test_shots/shot_8.png`, `shot_9.png` — **prueba EN JUEGO**
  del título en castellano (`Pulsa START`, `Nueva partida`, `Continuar`,
  `Opciones`, `Xbox LIVE Marketplace`).
* `extract/f2/run/title_test.log` — log con la sonda `IU_ES_AIFPROBE`: los
  chunks `AIF `/`ao__` del ASF y la sustitución de `title_menu`/`title_continue`.
* **Log del hook** (`extract/f2/run/EUROPE/logs/iu_trace.log`, run real):

  ```
  UI: 6 texturas en manifiesto
  UI: SUSTITUIDA translation/d1_ud1_000A4000_005_accents.aif @ E944E000 (593920 B)
  UI: SUSTITUIDA translation/d1_ud1_000A4000_005_accents.msg @ E94DF000 (1516032 B)
  UI: SUSTITUIDA translation/nowloading_es.aif @ A8E01000 (36864 B)
  UI: SUSTITUIDA translation/atlas_ui_es.aif @ A8E0F000 (4198400 B)
  UI: scan END regions=19 aif=606 477 ms     # en el hilo del escáner (tid≠juego)
  UI MENU name='ROOT' elem=E64FD8B0          # sonda del menú principal activa
  ...
  # TAREA 1: sustitución del título EN LA CARGA DEL ASF (misma sesión, ~57 s):
  UI AIF 822226D8 r3=EB72E000 [r3+16]=00010140 tag0='ao__'
  UI AIF 821EF618 r3=E5BF0A90 r4=EBA231F0 tag='AIF '
  UI: SUSTITUIDA translation/title_menu_es.aif @ EBA231F0 (806416 B)
  UI: SUSTITUIDA translation/title_continue_es.aif @ EBB001F0 (110096 B)
  ```

  El atlas de UI aparece en la **misma** dirección (`A8E0F000`) que en la sesión
  previa de `FindAtlasOnce` → misma textura, ahora vía manifiesto. El arranque
  **no se cuelga**: el juego llega al título y avanza a los créditos con texto ES
  (`Director de escenarios`, `Programador jefe`).

## 8. Confianza

| Afirmación | Confianza | Evidencia |
|---|---|---|
| Los rótulos son texturas (no texto) | **Alta** | `second-text-path.md` |
| El pintado es sin pérdida (A8R8G8B8) | **Alta** | `_verify_bytes` + `envoltorio OK` |
| El atlas de UI se sustituye en runtime | **Alta** | log `SUSTITUIDA translation/atlas_ui_es.aif @ A8E0F000` |
| El arranque NO se cuelga con la integración | **Alta** | run completo a título + créditos; `scan END … 477 ms` en otro `tid` |
| El manifiesto identifica cada textura | **Alta** | firma 0x30 B + FNV-64 (colisiones comprobadas) |
| Acentos 005: atlas **y** `.msg` (métricas) se sustituyen | **Alta** | log `SUSTITUIDA …_accents.msg @ E94DF000` |
| Los acentos 005 se emiten y dibujan | **Alta** | `build_es_codes.py` (17 255 usos) + `verify_accents_render.py` + **`accents_ingame_proof.png`** (`ficción`) |
| La aguda/ñ ya no salen como barra | **Alta** | `add_accents.py` (trazo fino) + `accents_offline_proof.png` |
| `OBJETIVO` y demás rótulos encajan sin solape | **Alta** | montaje origen/ES + `_advance`/`getlength` |
| El bitmap del menú principal está en `0001F800_000_MESH.asf` | **Alta** | OCR offline (`scan_embedded_aif.py` → `emb_scan.out`): `New Game`/`Options`/… en el AIF `@0x2F51F0` |
| El menú principal se sustituye **al cargar el ASF** | **Alta** | `OnAif` sobre `sub_821EF618/678`; log `SUSTITUIDA title_menu_es.aif @ EBA231F0` **y captura en juego** (`shot_8/9`) |
| El escáner por firma llegaba tarde al título | **Alta** | log antiguo: `SUSTITUIDA title_menu…` a los ~108 s, con el título ya dibujado |
| `TARGET` se dibuja **glifo a glifo** (6 sprites) | **Alta** | tabla de sprites `000A4000_003_TTD.bin` (6 cajas de 22-24×26 en y=484.5) + el corte observado al pintar la banda entera |
| `BLANCO` se pinta por glifo y encaja | **Alta** | `_target_glyph_fix.png` (cada letra dentro de su caja TTD) |
| Título: ES centrado en la misma caja que el inglés | **Alta** | `title_menu_cmp.png`/`title_continue_cmp.png` (centros coinciden) |
| Título: `Continuar` con el mismo tamaño que el resto | **Alta** | `_harmonize_sizes` global por estilo → `title`=30 en ambas specs |

> **Prueba en juego de los acentos de celda:** el disclaimer del banco 005
> (`accents_ingame_proof.png`) muestra *«Este juego es ficción…»* con la `ó`
> acentuada renderizada desde la celda ampliada; confirma atlas + métricas en
> runtime.

## 9. Pantallas de ayuda del CAMP (1280×720) — traducción (WS-texturas)

Las pantallas de ayuda/tutorial del CAMP son **capturas estáticas 1280×720
DXT1** con el texto **horneado** (rótulo + barras de ayuda + paneles). No son
texto dinámico: no pasan por `sub_826D52C0`. Se traducen con
`tools/ui_help_screens.py`, que **quita** el texto original con *inpainting*
(`cv2.inpaint` sobre una máscara de tinta) y **pinta** el castellano encima
(re-codificando DXT1 sin cambiar tamaño). Igual que el resto: **hook**, sin
tocar assets.

### 9.1 Inventario sistemático (`tools/scan_aif_text.py`)

Escáner general (OCR tesseract TSV) de **todos** los AIF de ambos discos:
AIF sueltos **y** AIF incrustados en contenedores `ASF/MRON/BIN/AAF/ACF`.

| Conjunto | Ficheros | AIF | con texto (dedup) | pantallas 1280×720 |
|---|---:|---:|---:|---:|
| Ambos discos, **todo** (sueltos + incrustados) | 21 692 | 10 316 | **4 291** | **100** |
| — `.aif` sueltos (`all_d1ud1`+`all_d1ud2`) | 872 | 841 | 628 | 100 |
| — AIF incrustados en contenedores | 20 820 | 9 475 | 3 663 | 0 |

De las **104** pantallas 1280×720 hay **100 bitmaps únicos**: **51 en inglés** y
**53 en japonés**; **4 pares EN/JP son byte-idénticos** (no tienen texto
localizable salvo el rótulo, que es igual en ambos). Los dos bloques se separan
por contenido: **JP = `26FEE000`…`27FB8800`**, **EN = `28000000`…`28E67800`**
(el par EN de cada JP se localiza por arte: `pair_diff` < 15 frente a > 23 del
siguiente).

### 9.1b Triaje de los 4 291 «con texto»: **no hay rótulos nuevos**

Los 4 291 son, salvo las pantallas y lo ya traducido, **falsos positivos** de
OCR: texturas repetitivas de modelos (piedra, hierba, ladrillo, normal maps,
tejidos…) que tesseract lee como palabras cortas sin sentido («ate», «the»,
«bay»). Criterios de descarte aplicados:

* **Bigramas contra el inglés del juego** (`extract/messages/text/*.txt`):
  **1** única textura con bigramas reales → el menú principal (ya traducido).
* **Palabras largas (≥5) con conf≥70**: 15 entradas → todas ruido o atlas de
  glifos.
* **Palabras clave de UI** (TARGET/SAVE/…): 5 entradas → las ya traducidas.
* **Inspección visual** de los mejores por `score` y por formato
  (`extract/f2/re_analysis/emb_noise_sheet.png`, `emb_candidates.png`,
  `emb_img_candidates.png`, `emb_last.png`, `emb_960.png`).

| Grupo | Ejemplos | Veredicto |
|---|---|---|
| Materiales/normal maps `MTEX`/`MESH` | `69AB6000_017`, `3A123800_00x`, `18A6F000_003` | **ruido** (sin texto) |
| Atlas de glifos (fuentes) | `7EC08000_011_RMD`, `2E8D1800_003_RMD`, `26E7C000_000_IMG` | **glifos**, no rótulos |
| Logos (marcas) | `0009B800_000_MESH.asf`, `0001F800_000_MESH.asf@10B2A0` | **nombre propio** |
| Copyright / boot | `26EC7000_000_IMG.aif` | **descartado** (aviso legal) |
| Mando Xbox | `26EF5000_000_IMG.aif` | **iconos de botón** (A/B/X/Y) |
| Fondos/arte | `26F05800_000_IMG.aif`, `_MAIF`, `_EPAC`/`_SKAC` | sin texto |
| Ya traducido | atlas UI, GAME OVER, Now Loading, Reload, menú, 51 pantallas | — |

**Conclusión: 0 texturas de UI nuevas por traducir.** El inventario de rótulos
horneados queda cerrado con lo ya integrado.

### 9.2 Registro de pantallas traducidas

Registro en `tools/ui_help_screens.py` (`_TITLES` + `LABELS`). **51/51**
pantallas EN con el **rótulo** traducido; además, las pantallas con paneles
explicativos llevan traducido su texto (barra de ayuda, cabeceras de panel,
listas de opciones, cajas de diálogo). **335 rótulos** en total.

| HEX (EN) | Rótulo EN | ES | rótulos |
|---|---|---|---:|
| 28000000/28044800/2808B000 | BATTLE 1 | BATALLA 1 | 1/1/18 |
| 280D2800 / 28119000 | CAMP | CAMPAMENTO | 25/9 |
| 28164800 | SEARCHING | BUSCAR | 6 |
| 281B0000/281FA800/28243000 | TREASURE CHESTS | COFRES DEL TESORO | 1/4/2 |
| 2828A800/282D5000/28323800/2836D800 | CONNECT 1 | CONEXIÓN 1 | 1/8/1/8 |
| 283B8800 | SAVING | GUARDAR | 13 |
| 28404000 | FLEEING | HUIR | 1 |
| 2844E000/28493800/284DF000 | TACTICS | TÁCTICAS | 9/4/4 |
| 2852C000/28577800/285BF800/28608000 | DETECTION | DETECCIÓN | 2/3/9/5 |
| 28650800 | RECOVER REQUEST | PEDIR AYUDA | 1 |
| 28698000/286DF800 | FLUTE | FLAUTA | 17/1 |
| 2872E000/28777000/287BB000 | CREATION 1 | CREACIÓN 1 | 10/22/11 |
| 28800000/28845000 | PARTY SETUP | GRUPO | 18/13 |
| 2888A000/288D1800 | MULTIPLE PARTY SETUP | GRUPO MÚLTIPLE | 14/13 |
| 28916800/28961800/289AD800/289F7000/28A40800 | BATTLE 2 | BATALLA 2 | 1 (×5) |
| 28A8B000 | HEALING REQUEST | PEDIR CURACIÓN | 3 |
| 28AD6000 | (sin texto) | — | 0 |
| 28B26000/28B71800 | BATTLE 3 | BATALLA 3 | 1/1 |
| 28BB9000/28C01800 | CONNECT 2 | CONEXIÓN 2 | 6/5 |
| 28C4E000/28C94800 | CREATION 2 | CREACIÓN 2 | 17/28 |
| 28CE0000/28D32000 | SPECIAL CONDITIONS 1/2 | CONDICIÓN ESPECIAL 1/2 | 9/1 |
| 28D7C800/28DC9000 | INVISIBLE ENEMIES | ENEMIGOS INVISIBLES | 1/1 |
| 28E19000/28E67800 | GLYPH RATE | TASA DE GLIFOS | 1/1 |

> **Pendiente (texto pequeño en escena):** en muchas pantallas quedan rótulos
> diminutos dibujados por el motor *dentro* de la captura (nombres de PNJ,
> `Lv.`, cifras de PS, `TARGET`), que no se han repintado. Son redundantes con
> el HUD real (ya traducido por el atlas/hook). Ver §9.5.

### 9.3 Integración

`ui_help_screens.py render` → `extract/f2/ui_es/help_<hex>_es.aif`;
`ui_textures.py manifest` los añade al manifiesto (`_help_assets()`), con la
**firma de 0x30 B del AIF original** y el **FNV-64** (todas las pantallas
comparten cabecera DXT1 → el hash es imprescindible para desambiguar).
Manifiesto: **58 texturas** (8 previas + 50 pantallas; `28AD6000` no cambia y no
se registra). Verificación por pantalla: `envoltorio OK` (tiling+byte-order
exactos) y tamaño idéntico al original.

### 9.4 Reproducción

```bash
# inventario OCR de ambos discos (sueltos + incrustados)
python -X utf8 tools/scan_aif_text.py --roots extract/f2/all_d1ud1 extract/f2/all_d1ud2 \
       --out extract/f2/re_analysis/aif_text_embedded.json
# pintar pantallas + manifiesto
python -X utf8 tools/ui_help_screens.py render --out extract/f2/ui_es --preview
python -X utf8 tools/ui_textures.py manifest --out extract/f2/ui_es \
       --deploy extract/f2/run/translation
```

### 9.5 Evidencia y confianza

* `extract/f2/re_analysis/aif_text_loose.json` / `aif_text_embedded.json` — inventario.
* `extract/f2/re_analysis/help_proposals.json` — cajas OCR de las pantallas.
* `extract/f2/re_analysis/help_previews_sheet.png` — hoja de las 51 pantallas ES.
* `extract/f2/ui_es/cmp_help_*.png` — origen (EN) vs ES.

| Afirmación | Confianza | Evidencia |
|---|---|---|
| Hay 104 pantallas 1280×720 (100 únicas, 51 EN) | **Alta** | escáner + agrupado por arte |
| Las 51 pantallas EN se sustituyen por firma+FNV-64 | **Media-Alta** | manifiesto (58), envoltorio OK; falta run |
| El rótulo de las 51 se traduce y encaja | **Alta** | hoja de previews |
| El texto de panel (335 rótulos) se traduce | **Media-Alta** | previews por pantalla |
| El texto diminuto en escena no se repinta | — | pendiente (§9.5) |
| **No hay rótulos de UI nuevos** en ambos discos | **Alta** | triaje §9.1b: bigramas (1 hit), palabras largas (15), keywords (5), inspección visual de ~60 previews |
| El manifiesto tiene 58 texturas y todas dan `envoltorio OK` | **Alta** | 57 AIF verificados (2 con su `base`) + `.msg` de acentos |
| No se tocaron `ud1.bin`/`ud2.bin` ni `translation_es.h` | **Alta** | `git status` limpio en esos ficheros |

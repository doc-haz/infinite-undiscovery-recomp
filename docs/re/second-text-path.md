# Rótulos que NO cambian con el hook — ¿textura o segunda ruta de texto? (F2/F5)

> **Pregunta:** los rótulos `46..49` (`New Game`/`Continue`/`Options`/`Xbox Live`),
> `10021` (`Battle Skills`), `10036`/`10038` (`Connect`/`Support Skills`), `10049`
> (`Items`), `10088` (`Usable Items`) y `Pause` **no** cambian con el hook sobre
> `sub_826D52C0`. ¿Son **(A)** texturas pre-generadas o **(B)** una segunda ruta
> de texto que lee el `.msg` con otro lector?

## 0. Conclusión ejecutiva

**Son (A): TEXTURAS / sprites pre-renderizados.** No existe una segunda ruta de
texto que lea el `.msg`. Las etiquetas que no cambian **nunca** llegan al
`sub_826D52C0` (el único resolvedor `id → texto`) y, para el menú principal, se
cargan además por **nombre** como recursos de UI (`NewGame`, `Continue`,
`OPTIONS`, `XboxLive`, …).

En consecuencia, el hook en `obj+0x78` **no puede** cambiarlas: son píxeles, no
códigos de glifo. Para traducirlas hay que **sustituir la textura** (o interceptar
el elemento de UI que la dibuja), no el parser de mensajes.

---

## 1. Evidencia

### 1.1 Sólo existe UN resolvedor `id → texto` (no hay ruta B)

Búsqueda exhaustiva sobre **todos** los ficheros del C++ generado
(`extract/f2/recomp/generated/default/*.cpp`, ~10 000 funciones):

* El patrón exacto del parser del directorio `.msg` —leer `[base+0x20]`
  (records), `[base+0x24]` (pool), recorrer registros de 16 B comparando
  `record[0]` con `obj+0x88`, e **imagen** `obj+0x78 = pool + record[1]`— sólo
  aparece en **`sub_826D52C0` (`0x826D52C0`)**.
* **`sub_826D2F88`** (el *fetcher* del `.msg` por clave de fuente) tiene
  **exactamente 3 llamadores**: `sub_826D3E20` (`0x826D3E20`, clave→base),
  `sub_826D4190` (`0x826D4190`, **sólo comprueba existencia** de un id) y
  `sub_826D3DB8` (`0x826D3DB8`, trae la fuente "pequeña", clave 1). Ninguno
  devuelve texto.
* **`sub_826D52C0`** sólo tiene **2 llamadores**: `sub_826D5528` (update de
  `CTextWnd`, `0x826D5528`) y `sub_826E73E8` (orquestador de `CMessageTextWnd`,
  `0x826E73E8`). Ambos van al mismo parser.
* La tabla de controles `0x4000..0x4019` (**26 entradas**, la que tratan los
  rótulos `{0x4003:…}`/`{0x4001:…}`) aparece **únicamente** en
  **`sub_826D43F8`** (`0x826D43F8`, renderiza) y **`sub_826D30B8`**
  (`0x826D30B8`, cuenta), que son *downstream* de `sub_826D52C0`.

⇒ Si un rótulo se dibujara como texto, **obligatoriamente** pasaría por
`sub_826D52C0`. No hay atajo.

### 1.2 Los rótulos nunca llegan al parser (traza en vivo)

`extract/f2/run/EUROPE/logs/iu_trace.log` (sesión de ~68 s: menú de
configuración + CAMP + créditos). El hook registra `ES RESOLVED fp=… id=…` una
vez por `(huella, id)`:

* SÍ resuelve cientos de ids del banco `005` (`fp=DC0D11A6`), incluidos los
  rótulos **dinámicos** del CAMP (`10061` "Select an item to use",
  `10062`, `10070`, `10088`, `10109`, `10162`…`10424`, `75000001`…).
* **Nunca** resuelve `46`, `47`, `48`, `49`, ni `10049`–`10060` (los rótulos de
  pestaña del CAMP: `Items`, `Magic`, `Equipment`, `Skills`, `Connect`,
  `Flute Music`, `Creation`, `Status`, `Map`, `Party`, `Options`, `Data`).

### 1.3 Prueba decisiva: parche *in-place* del `.msg`

Con `IU_ES_TESTID=10049` el hook reescribe **el propio pool** del `.msg`:

```
ES PATCH id=10049 @ pool+9211 (7 bytes)
```

Es decir, el texto codificado de `10049` en el banco `005` (`E94DF000`) quedó
sustituido por basura segura. Si el rótulo "Items" se leyera del `.msg` en
runtime, habría cambiado (el CAMP se muestra **después** de aplicar el parche).
**No cambió** ⇒ el rótulo en pantalla no se lee del pool del `.msg`.

### 1.4 Menú principal = recursos de UI **por nombre** (prueba de código)

* `CTitleLogoTask` (vtable `0x8203AC54`, COL `0x820F3FE4`) tiene en el **slot 12**
  a **`sub_8249AE60` (`0x8249AE60`)**.
* `sub_8249AE60` llama a **`sub_824999A0` (`0x824999A0`)**, que ejecuta **22**
  llamadas a **`sub_82182070` (`0x82182070`)** y **ninguna** función del
  subsistema de mensajes/fuente (no llama a `sub_826D52C0`, `sub_826D2F88`,
  `sub_826D5FC0` ni `sub_826D5650`; sus otras dos llamadas, `sub_821A3028` y
  `sub_82147140`, son un *getter* de singleton y el registro de objetos).
* **`sub_82182070(gestor, nombre, r5)`** calcula un **hash del nombre**
  (`h = (h*8 + c) % modulus`) y busca el recurso en el gestor
  (`[gestor+68]`, vtable slot 16). Devuelve un **elemento de UI** (ajeno al
  texto): el load de `sub_824999A0` anima cada resultado con el slot `0xE8`
  (alfa) → son *sprites/texturas*.
* Los nombres cargados (leídos del `.exe`, región `0x8203AB44`–`0x8203ABC4`):

  ```
  Icon  Icon1  Icon2  Icon3  OPTIONS  NewGame  XboxLive
  EASY  NORMAL  HARD  VERY_HARD  copyrgiht  Continue
  PressStart2  Infinity  LoadGame  PressStart
  ```

  Es decir, **`NewGame`, `Continue`, `OPTIONS`, `XboxLive`** (los ids 46–49) y
  `LoadGame`/`PressStart` son **recursos con nombre**, no entradas del `.msg`.
  Los mismos nombres aparecen en `ud1.bin` (tabla de elementos de UI,
  offset `0x9A210`–`0x9A368`), junto a su *layout*.

> **Consecuencia:** aunque el `.msg` tenga `46="New Game"`, el menú principal
> dibuja la **textura** llamada `NewGame`. Por eso ni el hook ni el parche
> *in-place* la afectan.

### 1.5 Rótulos del CAMP = bitmap pre-renderizado

Los ids de las pestañas del CAMP (`10049`–`10060`) existen en el `.msg` pero
**nunca** se pasan al parser (§1.2/§1.3). Las cadenas aparecen ya *pixeladas* en
texturas `CH05` de 1280×720 (imágenes de la ayuda de menú):

| Fichero (`extract/f2/all_d1ud1/`) | Tamaño | Contenido |
|---|---|---|
| `280D2800_000_IMG.aif` | 1280×720 | menú **CAMP EN**: `CAMP`, pestañas `Items/Skills/Equipment/Status/Map/System/Talk`, subm.: `Battle/Connect/Flute/Magic/Personal`, `Items`/`Magic`… |
| `27035800_000_IMG.aif` | 1280×720 | la misma pantalla en **JP** |
| `26FEE000`, `2719F800`, `27509800`, `27789000`, `277CF800`, `278A8000`, `279D1800`, `27A1C800`, `27AFB800`, `27BE1800`, `27C74800`, `27CBD000`, `27D4F800`, `2808B000`, `2828A800`, `2836D800`, `28916800`, `28961800`, `289AD800`, `28A40800`, `28B26000`, `28BB9000`, `28C4E000`, `28C94800`, `2759F000`, `2762D000`… (`_IMG.aif`) | 1280×720 | otras pantallas de ayuda con `battle`/`connect`/`equipment`/`status`/`system`/`party`/`flute` |

(OCR: `extract/f2/re_analysis/ocr_scan*.out`; render: `extract/f2/re_analysis/ui/`.)

### 1.6 Más rótulos horneados (confirman el patrón)

El juego **sí** usa texturas para rótulos fijos. Comprobado decodificando los
`AIF` con `tools/aif_edit.py png`:

* `GAME OVER` → `274AE800_000_IMG.aif` (PG02, 640×480).
* `Now Loading…` → `28EB4000_000_IMG.aif` (EN) y `00016000_000_IMG.aif` (JP), 256×32.
* `Pause` y `Now Loading…` → atlas de UI `000A4000_002_IMG.aif`
  (= `26D8C000_000_IMG.aif`, PG02, 1024×1024).
* `Reload.` → `26F88000_000_IMG.aif` (PG02, 256×256).

---

## 2. Funciones implicadas

| Dirección | Nombre | Papel | Relevancia |
|---|---|---|---|
| `0x826D52C0` | `sub_826D52C0` | **Único** parser `id → (pool+off, w, h)`; escribe `obj+0x78` | El hook actual |
| `0x826D5528` | `sub_826D5528` | update de `CTextWnd` (vtable slot 46); llama al parser | Ruta de texto |
| `0x826E73E8` | `sub_826E73E8` | orquestador `CMessageTextWnd` (slot 50) | Ruta de texto |
| `0x826D43F8` | `sub_826D43F8` | render de `CTextWnd` (slot 43); tabla de controles 26 | Ruta de texto |
| `0x826D30B8` | `sub_826D30B8` | contador (slot 42); misma tabla de controles | Ruta de texto |
| `0x826D3268` | `sub_826D3268` | mapa código→glifo (cmap), métricas | Ruta de texto |
| `0x826D2F88` | `sub_826D2F88` | fetcher del `.msg` por clave de fuente (3 llamadores) | Ruta de texto |
| `0x826D4190` | `sub_826D4190` | **sólo** comprueba si un id existe (recorre registros) | NO es una ruta de texto |
| `0x826D5FC0` | `sub_826D5FC0` | helper genérico: crea `CTextWnd` + `SetMessage(fontKey,id)` | Ruta de texto **dinámica** |
| `0x821A2F10` | `sub_821A2F10` | `SetMessage(obj, fontKey=+0x84, id=+0x88)` | Setter del id |
| `0x82182070` | `sub_82182070` | **lookup de recurso de UI por NOMBRE** (hash → gestor) | **(A)** menú principal |
| `0x824999A0` | `sub_824999A0` | carga los 22 elementos con nombre del título | **(A)** menú principal |
| `0x8249AE60` | `sub_8249AE60` | `CTitleLogoTask` slot 12 → llama a `sub_824999A0` | **(A)** menú principal |
| `0x82477158` | `sub_82477158` | `CTitleTask` slot 6 (crea el rótulo del *disclaimer*, id 152) | contexto |

Vtables: `CTextWnd` slot 0 = `0x820065EC`; `CMessageTextWnd` = `0x8206AF4C`;
`CTitleLogoTask` = `0x8203AC54` (COL `0x820F3FE4`).

---

## 3. Cómo abordarlo en la traducción

### 3.1 Lo que el hook actual hace bien (ruta de texto)

El hook de `sub_826D52C0` → `obj+0x78` sigue siendo correcto para **todo texto
dinámico** (diálogos, listas, barra de ayuda del CAMP `10061+`, opciones,
créditos…). No hay que tocarlo.

### 3.2 Rótulos horneados (A): tres opciones, de más simple a más fina

1. **Sustituir la textura del asset** (más robusto y sin código de runtime):
   reemplazar el `AIF` correspondiente (`280D2800_000_IMG.aif`,
   `27035800_000_IMG.aif`, `274AE800_000_IMG.aif`, `28EB4000_000_IMG.aif`, …)
   por una versión traducida, re-codificando con `tools/aif_edit.py`
   (round-trip DXT/tiling ya validado). Sirve para el menú CAMP y los rótulos
   fijos.
2. **Interceptar el *lookup* por nombre** para el menú principal: hook de
   **`sub_82182070` (`0x82182070`)** (mismo patrón de alias débil
   `__imp__sub_82182070`). Si `nombre ∈ {NewGame, Continue, OPTIONS, XboxLive,
   LoadGame, PressStart, …}`, devolver el recurso de UI traducido en lugar del
   original. Es el punto exacto donde el título elige el rótulo.
3. **Interceptar el dibujo del elemento de UI**: en `sub_824999A0` los
   elementos viven en `[obj+48/52/56/60/64/68/72/80]` y se animan por el slot
   `0xE8` de su vtable. Un hook sobre el *draw* de esos elementos permitiría
   sustituir la textura, pero es más frágil que (1) o (2).

> El patrón de hook es el ya usado en `mod/recomp/translation_es.h`:
> ```cpp
> extern "C" void __imp__sub_82182070(PPCContext& ctx, uint8_t* base);
> extern "C" void sub_82182070(PPCContext& ctx, uint8_t* base) {
>   /* leer el nombre en r4 (guest) y, si hay sustituto, ... */
>   __imp__sub_82182070(ctx, base);
> }
> ```

### 3.3 Lo que **no** hay que hacer

* **No** buscar una "segunda ruta de texto": no existe. Los ids `46..49` y
  `10049..10060` del `.msg` son **entradas no usadas** por la UI (probablemente
  legado/para otra función); el rótulo en pantalla es un bitmap.

---

## 4. Reproducción

```bash
# 1) El único resolvedor y sus llamadores
python (legacy exploration tool, removed) func 0x826D52C0     # parser
python (legacy exploration tool, removed) func 0x826D5528     # update CTextWnd
python (legacy exploration tool, removed) func 0x826D43F8     # render (tabla 26 controles)

# 2) Menú principal: nombres cargados
python (legacy exploration tool, removed) func 0x824999A0     # cargador (22 nombres)
python (legacy exploration tool, removed) func 0x82182070     # lookup por nombre (hash)
python (legacy exploration tool, removed) func 0x8249AE60     # CTitleLogoTask slot 12

# 3) Trazas en vivo
#   extract/f2/run/EUROPE/logs/iu_trace.log  (ES RESOLVED / ES PATCH / P_43F8…)
#   extract/f2/run_capture2.sh               (IU_ES_TESTID=<id>)

# 4) Texturas
python tools/aif_edit.py png extract/f2/all_d1ud1/280D2800_000_IMG.aif /tmp/camp_en.png
python tools/aif_edit.py png extract/f2/all_d1ud1/274AE800_000_IMG.aif /tmp/gameover.png
```

Artefactos de apoyo (no versionados, en `extract/`): `extract/f2/re_analysis/`
(volcados de funciones, `ui/*.png`, `ocr_scan*.out`).

---

## 5. Confianza

| Afirmación | Confianza | Evidencia |
|---|---|---|
| `sub_826D52C0` es el **único** resolvedor `id→texto` | **Alta** | barrido exhaustivo del C++ generado; 3 llamadores de `sub_826D2F88`, 2 de `sub_826D52C0` |
| Sólo `sub_826D43F8`/`sub_826D30B8` tratan los controles `0x4000..0x4019` | **Alta** | única tabla de 26 entradas en todo el binario |
| Los rótulos no llegan a `sub_826D52C0` | **Alta** | traza `iu_trace.log` (no aparece `46..49` ni `10049..10060` en `ES RESOLVED`) |
| El parche *in-place* de `10049` no cambia el rótulo | **Alta** | `ES PATCH id=10049` + observación del padre |
| El menú principal carga `NewGame`/`Continue`/`OPTIONS`/`XboxLive` **por nombre** como elementos de UI | **Alta** | `sub_824999A0` ×22 `sub_82182070`, lista de nombres en `.exe`/`ud1.bin` |
| Los rótulos CAMP `10049..10060` están horneados en bitmaps | **Alta** | no se parsean + aparecen pixelados en los `CH05` 1280×720 |
| `sub_82182070` es el punto ideal para el hook del menú principal | **Media-Alta** | es la función que resuelve nombre→elemento; falta verificar en runtime |
| Ubicación exacta del bitmap de cada pestaña CAMP en tiempo de ejecución | **Media** | hay muchos `CH05` 1280×720 de ayuda; lo seguro es que no es texto dinámico |

**Respuesta a la pregunta:** **(A) texturas**. No hay segunda ruta de texto; los
rótulos no se leen del `.msg` en runtime. Para traducirlos, sustituir el bitmap
(o hookear `sub_82182070` en el menú principal) — nunca `sub_826D52C0`.

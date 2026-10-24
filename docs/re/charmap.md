# Charmap `código → carácter` por banco (F2.5)

> **Estado:** completo. Los **81 bancos** `RMD-` (48 únicos) tienen charmap en
> `translation/charmaps/<stem>.json` y texto en
> `extract/messages/text/<stem>.txt`.
> **Cobertura global de glifos (por ocurrencia en los 71.386 mensajes): 99.99 %.**
> **Herramientas:** `tools/decode_messages.py`, `(legacy exploration tool, removed)`,
> `(legacy exploration tool, removed)`, `(legacy exploration tool, removed)`, `(legacy exploration tool, removed)`,
> `tools/build_charmaps.py`, `tools/write_texts.py`.
> Complementa a `docs/re/font-loader.md` (formato) y `docs/re/disc2-messages.md`.

---

## 0. TL;DR

* El texto del juego **no** usa ASCII/UTF-16: usa un alfabeto propio de códigos
  (`code < 300` → fuente pequeña, índice `code-1`; `code >= 300` → fuente
  principal, índice `code-301`). Cada `RMD-` trae su **atlas `AIF `** y su
  **charmap propio** (no hay charmap global).
* El atlas de cada banco está ordenado por **primera aparición** del glifo en el
  texto de ese banco, así que `código = 301 + índice_de_celda` (con la salvedad
  de celdas duplicadas, resuelta por hash — ver §3).
* Método: **clusterizar** los 20.112 glifos de los 48 atlas en **3.266 formas
  distintas** (por md5 del bitmap), leer a mano las más frecuentes (hojas de
  contacto etiquetadas) y rellenar la cola larga con **Tesseract (jpn)**.
* Se validó el mapeo reproduciendo mensajes conocidos (p. ej. `022` id=2
  "Great song. What's it called?"; `040` id=1 "セラフィックゲートはようこそ").

---

## 1. Estructura del `.msg` (recordatorio)

Big-endian. Cabecera: `0x20 records_off`, `0x24 pool_off`, `0x28 metrics_off`,
`0x30 total`, `0x34 count`, `0x44/0x48 cell=32`, `0x60/0x64 atlas_w/h`.
Directorio `[records_off, pool_off)`: registros de **16 B**
`(id u32, offset u32, ancho f32, alto f32)`, terminador `(0,0)`.
Pool: byte-stream variable
`b0<0x80 → code=b0; b0≥0x80 → b1; code=(b1<<7)|(b0&0x7F)`; `code==0` fin;
`0x4000..0x4019` control (con payload); resto glifo.
Atlas: `cols = atlas_w/cell_w`, row-major.

## 2. Cómo se construyó el charmap

1. **Clusterizado** (`(legacy exploration tool, removed)`): se recorren los atlas de los 48
   bancos únicos y se agrupan las celdas de 32×32 por `md5(RGBA)`. Resultado:
   **20.112 celdas → 3.266 bitmaps distintos** (`extract/f2/glyphs/reps.pkl`,
   `hash_to_cid.json`, `cluster_meta.json`). Una misma forma se lee **una vez** y
   sirve para todos los bancos.
2. **Lectura visual** (`(legacy exploration tool, removed)`): hojas con el glifo ampliado, su
   `cid` y la propuesta de Tesseract. Se leyeron a mano las ~1.700 formas más
   frecuentes (`extract/f2/glyphs/read/*.json`, `manual.json`).
3. **Relleno automático** (`(legacy exploration tool, removed)`): Tesseract 5.4 con
   `jpn.traineddata` sobre cada glifo ampliado y binarizado; se usa solo para la
   cola larga (formas raras) y se marca como `tess` (baja confianza).
4. **Validación** por contexto: se decodificaron los bancos con estructura
   **JP/EN pareada** (ids impares en japonés, pares en inglés) y se corrigieron
   los mapeos que producían palabras inglesas inválidas
   (`(legacy exploration tool, removed)`). Ejemplos corregidos: `cid2='l'` (barra simple)
   vs `cid455='I'` (con serifas), `cid448=','`, `cid466='O'`, `cid463='x'`,
   `cid469='J'`, `cid471='q'`.
5. **Charmaps** (`tools/build_charmaps.py`): para cada celda, `cid` por hash →
   carácter (prioridad: verdad de referencia por banco > lectura manual >
   Tesseract). Salida plana `{"301":"セ", ...}`.

### 2.1 Verdad de referencia (semillas)

`extract/f2/glyphs/ground_truth.json` fija caracteres verificados visualmente
(53 semillas) para `022` y `040`; tienen prioridad absoluta.

## 3. Trampas resueltas

* **Celdas duplicadas / orden de aparición.** En algunos bancos (p. ej. `005`)
  hay celdas idénticas repetidas (espacios, variantes). Por eso el mapeo se hace
  por **hash de la celda**, nunca asumiendo `cid = código-301`. El atlas "se lee"
  como el texto deduplicado, pero con huecos/variantes.
* **Polaridad del bitmap.** El glifo va en **RGB** (blanco) con un contorno
  oscuro; el canal **alpha es una máscara redondeada invertida**. La lectura y el
  OCR usan el **canal RGB** (`max(r,g,b)`), no el alpha.
* **`I` / `l` / `1`.** En esta fuente la barra simple se comparte entre `l` e
  `I`. Se resolvió por contexto: `cid455` = `I` (con serifas), y las barras
  simples (`cid2`, `81`, `149`, `452`, …) = `l`. Restos de ambigüedad en cadenas
  de depuración (p. ej. `005` "lNVALlD HANDLE" → realmente "INVALID HANDLE").
* **Controles `0x4000..0x4019`.** Los tamaños de payload se derivaron de
  `sub_826D43F8` (recomp.140.cpp): `0x4001:1`, `0x4003:4`, `0x4007/08/0C/0E:4`,
  `0x4009:1`, `0x400A:2`, `0x4010:cadena-NUL`, `0x4018:1`, resto 0. En el texto
  se escriben como `{0x400A:2,1}`.

## 4. Entregables

| Ruta | Contenido |
|---|---|
| `tools/decode_messages.py` | codec + `MsgBank` (parsea `.msg`/`.aif`, decodifica, renderiza) |
| `tools/game_text_codec.py` | codec byte-exacto (round-trip 71.386/71.386) |
| `translation/charmaps/<stem>.json` | **charmap por banco** `código → carácter` (81 ficheros) |
| `extract/messages/text/<stem>.txt` | **texto** por banco, `id<TAB>texto` (81 ficheros) |
| `extract/messages/text/_coverage.json` | cobertura por banco |
| `extract/messages/context_manual.json` | contexto por banco |
| `extract/f2/glyphs/` | clusters, hojas, etiquetas, fuentes por `cid` |
| `tools/write_texts.py`, `build_charmaps.py`, `cluster_glyphs.py`, `render_top.py`, `render_verify.py`, `ocr_tess.py`, `diagnose_errors.py` | pipeline |

### Formato del texto

```
<id>\t<texto>
```
* controles → `{0x400A:2,1}` (código hex : payload decimal);
* glifo sin leer → `?[<code>]` (o `?[small:<code>]` para `code<300`);
* `0x4000` (fin de segmento) se omite.

## 5. Cobertura

* **Global (por ocurrencia de glifo de la fuente principal, `code>=300`):
  2.731.921 / 2.731.995 = 99.997 % (100,00 % redondeado).**
* Celdas de atlas sin carácter: **2** (padding no usado).
* Etiquetas por fuente: **1.702 manuales**, **1.563 de Tesseract** (de 3.265
  cids distintos usados).
* Únicos `?[…]` que quedan: **`?[small:N]`** (~70 ocurrencias), iconos de un
  recurso de **fuente pequeña** (`obj+96`, `code<300`) que **no** forma parte del
  atlas `AIF ` del `RMD-` (no extraído aquí). Ejemplo: `Use: Press and hold
  {0x4019}?[small:1],` (icono de botón).

### 5.1 Cobertura y contexto por banco (48 únicos; `d2_*` idénticos)

| Banco (canónico d1) | Contexto | Mensajes | Cobertura |
|---|---|---:|---:|
| `d1_ud1_000A4000_004` | fuente | 0 | — |
| `d1_ud1_000A4000_005` | debug | 16.647 | 99.99 % |
| `d1_ud1_317BD800_037` | dialogo | 326 | 100 % |
| `d1_ud1_34C6D000_019` | dialogo | 486 | 100 % |
| `d1_ud1_39513800_022` | dialogo | 654 | 100 % |
| `d1_ud2_17325800_002` | sistema | 898 | 100 % |
| `d1_ud2_1793D000_018` | sistema | 6 | 100 % |
| `d1_ud2_19478000_028` | sistema | 26 | 100 % |
| `d1_ud2_198FA800_008` | enemigo | 14 | 100 % |
| `d1_ud2_1B067800_037` | dialogo | 248 | 100 % |
| `d1_ud2_1E70B800_010` | menu | 978 | 100 % |
| `d1_ud2_224F6800_028` | dialogo | 162 | 100 % |
| `d1_ud2_254C2000_039` | dialogo | 60 | 100 % |
| `d1_ud2_28E68000_023` | dialogo | 438 | 100 % |
| `d1_ud2_2E8D1800_003` | menu | 1.726 | 100 % |
| `d1_ud2_339DF000_020` | dialogo | 666 | 100 % |
| `d1_ud2_3A123800_010` | sistema | 1.132 | 100 % |
| `d1_ud2_40141800_004` | menu | 2.418 | 100 % |
| `d1_ud2_4AEF1000_007` | dialogo | 556 | 100 % |
| `d1_ud2_4F0A3000_037` | dialogo | 82 | 100 % |
| `d1_ud2_510DD000_025` | menu | 338 | 100 % |
| `d1_ud2_5503E000_026` | dialogo | 184 | 100 % |
| `d1_ud2_5861C800_058` | menu | 34 | 100 % |
| `d1_ud2_59DEA800_017` | menu | 1.608 | 100 % |
| `d1_ud2_603B9000_033` | dialogo | 48 | 100 % |
| `d1_ud2_62CF4800_014` | sistema | 8 | 100 % |
| `d1_ud2_644C2000_017` | dialogo | 286 | 100 % |
| `d1_ud2_69AB6000_009` | menu | 2.346 | 100 % |
| `d1_ud2_778A1000_030` | dialogo | 116 | 100 % |
| `d1_ud2_7B2EA000_042` | dialogo | 46 | 100 % |
| `d1_ud2_7EC08000_011` | menu | 2.566 | 100 % |
| `d1_ud2_8E200800_046` | dialogo | 76 | 100 % |
| `d1_ud2_916D8000_026` | dialogo | 366 | 100 % |
| `d1_ud2_96119800_017` | dialogo | 332 | 100 % |
| `d1_ud2_A014E800_005` | dialogo | 498 | 100 % |
| `d1_ud2_A4DAA000_040` | bonus | 4 | 100 % |
| `d2_ud1_59425000_042` | menu | 16 | 100 % |
| `d2_ud1_5B259800_027` | objeto | 2 | 100 % |
| `d2_ud1_5E3DD000_031` | objeto | 2 | 100 % |
| `d2_ud1_60F05800_035` | sistema | 8 | 100 % |
| `d2_ud1_63366800_048` | menu | 16 | 100 % |
| `d2_ud1_6623B000_023` | objeto | 2 | 100 % |
| `d2_ud1_68653000_037` | objeto | 2 | 100 % |
| `d2_ud1_6ABA3000_024` | objeto | 2 | 100 % |
| `d2_ud1_6BCF7000_038` | objeto | 2 | 100 % |
| `d2_ud1_6DFC5000_029` | menu | 32 | 100 % |
| `d2_ud1_7281F800_035` | objeto | 2 | 100 % |
| `d2_ud1_78EA4000_017` | objeto | 8 | 100 % |

Los `d2_ud2_*` (31 bancos) son **byte a byte idénticos** a sus `d1_ud2_*`
correspondientes (mismo `.msg`), y `d2_ud1_000A4000_004/005` idénticos a los
`d1_ud1_*`: comparten charmap y texto.

## 6. Contexto: observaciones

* La mayoría de bancos son **escenas/diálogo** y almacenan **cada línea dos
  veces**: id impar en **japonés**, id par en **inglés** (o viceversa). Esto se
  ve en `022`, `317BD800_037`, `1B067800_037`, etc.
* `d1_ud1_000A4000_005` (16.647 mensajes) es el banco de **cadenas de
  sistema/error** ("INVALID HANDLE", "UNKNOWN CHARACTER", nombres de personaje).
  Su "atlas" es en gran medida la **cobertura de la fuente latina principal**.
* `d1_ud1_000A4000_004` es un **atlas de fuente sin mensajes** (`n_recs=0`).
* `*_011`, `*_004`, `*_003`, `*_025`, `*_058`, etc. contienen **carteles de
  mapa** ("General Shop", "House", "Bar", "Inn", "Teleporter", "Lever").
* Los bancos pequeños de `d2_ud1` son de **obtención de objetos**
  ("Obtained a ..." / `…を手に入れた`) o **gestión de grupo**.
* `A4DAA000_040` y `78EA4000_017` son del **Seraphic Gate** (mazmorra extra).
* **Un mismo contexto usa varios bancos** (p. ej. "menu" agrupa 19 bancos) y
  **un banco puede mezclar contextos** (p. ej. `69AB6000_009` = carteles + diálogo;
  `4AEF1000_007` = diálogo + carteles). En la tabla se marca el contexto
  dominante; el detalle está en `extract/messages/context_manual.json`.

## 7. Puntos dudosos / trabajo pendiente

1. **Cola larga kanji (Tesseract).** ~1.563 de los 3.265 cids usados se
   rellenaron con Tesseract jpn (confianza media-baja). Errores típicos:
   confusión de kana/kanji similares (`は↔に`, `カ↔力`, `工↔エ`). El texto
   **inglés** (lo que más interesa para traducir) está validado por contexto; el
   **japonés** puede contener imprecisiones en kanji raros. Para elevar la
   calidad: releer a mano más cids de `extract/f2/glyphs/read/` (p. ej. los
   ~1.500 de baja frecuencia) y reconstruir con `tools/build_charmaps.py`.
2. **`I`/`l`/`1`.** Ambigüedad intrínseca (mismo trazo). Resuelta por contexto en
   su mayor parte; quedan cadenas de depuración (`005`, `d2_ud1_59425000_042`
   "lnn" por "Inn").
3. **Controles.** La semántica exacta de cada `0x4000..0x4019` (color, rect,
   subcadena, estado) está identificada en payload pero **no** traducida a texto;
   se emiten como marcadores. `0x4010` (`SUBSTR`) incrusta una subcadena NUL que
   podría ser texto adicional; se conserva como payload crudo.
4. **Fuente pequeña (`code<300`).** Usa un recurso de fuente **distinto** del
   atlas del `RMD-` (`obj+96`), no extraído aquí; los ~70 códigos `<300` que
   aparecen son **iconos de botón** y se emiten como `?[small:<code>]`.
   Pendiente: localizar/extraer ese recurso (probablemente otra clave de fuente
   del `fontMgr`, no un `RMD-`).
5. **2 celdas sin carácter** (padding no usado). No aparecen en ningún texto.

# Formato `RMD-` — message resource (hallazgos F0/F1)

> **Estado:** **F0 cerrado**. **F1 en curso** (estructura conocida; decodificación
> del texto pendiente).
> **Fuente:** volcado real de los dos discos EUROPE/PAL + `tools/vendor/pc-infiniteundiscovery`.

## 0. Resumen

Un recurso `RMD-` ("message resource") es un **atlas de fuente `AIF `** seguido de
**datos de mensaje** con cabecera `MessageConvertLib_1.0.0.0`:

```
RMD- payload
├─ AIF  (atlas de glifos: latino + kana + kanji)   ← primera mitad
└─ <message data>                                   ← segunda mitad (F1)
     ├─ "MessageConvertLib_1.0.0.0" + NULs
     ├─ cabecera de campos u32 big-endian (0x1C..0x68)
     ├─ [0x24, 0x28)  sección de códigos/mensajes
     └─ [0x28, EOF)   tabla de 2 B × count
```

**Hallazgo central:** el texto **no** está en ASCII ni en UTF-16 (en los 81 `.msg`
no hay ni una palabra legible). Es un flujo de **palabras u16 little-endian**:

- `>= 0x8000` → **código de control** (`0x8080`, `0x8087`, `0x808A`, …).
- `< 0x8000` → **código de glifo** (índice/código de la fuente, no ASCII).

Esto corresponde al **Plan B** del plan (§6.3 H4 / riesgo R1): la traducción
tendrá que trabajar con códigos de glifo, no con cadenas.

## 1. Censo (edición EUROPE/PAL) — verificado

Extraídos con `tools/extract_rmd.py`. **Coincide exactamente con el censo de la
comunidad**:

| Contenedor | `RMD-` | Bytes de mensaje |
|---|---:|---:|
| Disc 1 `ud1.bin` | 5 | — |
| Disc 1 `ud2.bin` | 31 | — |
| Disc 2 `ud1.bin` | 14 | — |
| Disc 2 `ud2.bin` | 31 | — |
| **Total** | **81** | **6 987 136** |

> `ud2.bin` es **idéntico** entre discos (mismos 31 `RMD-`, mismos tamaños), como
> ya anticipaba el censo. Los `default.xex` de Disc 1 casan con el perfil EUROPE
> conocido; el de Disc 2 no (solo se publicaron hashes del Disco 1).

## 2. Cabecera del message resource

Big-endian, tras el magic de 25 B (`"MessageConvertLib_1.0.0.0"`).

| Offset | Campo (provisional) | Observación |
|---|---|---|
| `0x00` | magic | `"MessageConvertLib_1.0.0.0"` + NULs |
| `0x20` | `data_start` | `0x80` en casi todos; `0` en un recurso atípico |
| `0x24` | `msg_section_off` | inicio de la sección de códigos/mensajes |
| `0x28` | `glyph_table_off` | inicio de la tabla de 2 B × `count` |
| `0x30` | `total_size` | **= tamaño del fichero `.msg`** |
| `0x34` | `count` | nº de entradas (¿mensajes? ¿glifos?) |
| `0x38`/`0x3C` | `field38/3c` | `800/800` o `512/512` |
| `0x40` | `field40` | `34` o `30` |
| `0x44`/`0x48` | `cell_w`/`cell_h` | `32/32` (celda del atlas) |
| `0x4C` | `bytes_per_code` | `2` |
| `0x50` | `field50` | `301` o `1` |
| `0x54` | `count2` | **= `count`** |
| `0x58` | `aif_size` | **= longitud del `AIF `** del mismo `RMD-` |
| `0x5C` | `magic2` | `0x0131F508` (constante en todos) |
| `0x60`/`0x64` | `atlas_w`/`atlas_h` | **= dimensiones del atlas** |

**Comprobaciones que sostienen la lectura** (fichero `d1_ud2_A4DAA000_040.msg`,
2 176 B):

- `total_size` (0x30) = 2176 = tamaño real. ✓
- `count` (0x34) = `count2` (0x54) = 150. ✓
- cola `[0x28, EOF)` = 1792 → 384 B ≈ `count` × 2 = 300 B (+ relleno). ✓
- `aif_size` (0x58) = 200704 = `aif_len` del manifiesto para ese recurso. ✓
- `atlas_w/h` (0x60/0x64) = 512×320 = dimensiones del atlas. ✓

## 3. Flujo de mensajes (u16)

Ejemplo real (`d1_ud2_A4DAA000_040.msg`, sección `[0x100, 0x700)`):

```
bytes:  8A 80 | 02 01 | AD 02 AE 02 AF 02 B0 02 ... BA 02 | 87 80 | 00 00 42 F0 | 80 80 ...
LE:     808A    0102    02AD 02AE 02AF 02B0 ... 02BA      8087    0000 42F0    8080
BE:     8A80    0201    AD02 AE02 AF02 B002 ... BA02      8780    0000 42F0    8080
```

**Endianness sin cerrar**, con evidencia a favor de **big-endian**:

| Lectura | Códigos `<0x8000` | Rango | Controles | Valores más frecuentes |
|---|---|---:|---:|---|
| LE | 125 distintos | `0x0000..0x42F0` | 59 | `0x9B03`, `0x9603`, `0x9C03`… |
| **BE** | **49 distintos** | **`0x0000..0x07B6`** | 135 | **`0x039B`, `0x0396`, `0x039C`…** |

En **BE** los códigos caen en un conjunto pequeño y coherente (típico de texto:
unos pocos caracteres repetidos), mientras que en LE se dispersan. Esto apunta a:

- códigos de glifo = u16 **big-endian** en el rango `0x02XX`/`0x03XX`;
- `0x8080`, `0x8081`, `0x808A`, … = **códigos de control** (alta bit a 1);
- `0x0000` y `0x42F0` (`float 120.5`) = parámetros/padding.

> Nota: el byte más frecuente del flujo es `0x03` (≈66 %), lo que es consistente
> con códigos BE `0x03XX`. No obstante, la interpretación exacta de los controles
> y del mapeo código→celda sigue abierta y probablemente **requiere F2** (leer la
> función del `.xex` que consume estos códigos), invirtiendo el orden estricto
> F1→F2 del plan.

## 4. La tabla de 2 B (`0x28`)

Contiene `count` valores u16 (BE `0x1C0E`, `0x1C0D`, `0x1C0C`, … con `0x1C`=28
constante y un segundo byte variable). **No son offsets** dentro de la sección de
mensajes (no caben en el pool y no son monótonos). Encajan mejor con **métricas
por glifo** (avance/bearing) o coordenadas de celda, dado `cell_w/cell_h = 32`.

## 5. Atlas de fuente y F3 (acentos)

Los 81 atlas se exportaron a PNG. Son **fuentes completas**: mayúsculas,
minúsculas, dígitos, símbolos, **kana y kanji** (p. ej. `d1_ud1_000A4000_005.png`,
768×768, con `A–Z a–z 0–9` y rejillas de kana/kanji).

**Observación para F3:** visualmente **no aparecen** `á é í ó ú ü ñ ¿ ¡` en el
atlas. Hay que confirmarlo celda a celda, pero apunta a que hará falta **extender
el `AIF`** (F3-b) o aplicar el plan C. Ver `glyph-coverage.md`.

## 6. Evidencia / comandos

```bash
# F0: extraer y partir los RMD- + atlas PNG + manifiesto
python tools/extract_rmd.py --roms roms --out extract/f0 --png
#   -> 81 RMD-, 6 987 136 B de mensajes, extract/f0/manifest.csv

# F1: cabecera y secciones
python (legacy exploration tool, removed) extract/f0/d1_ud2_A4DAA000_040.msg --codes 32

# F1: primeros auxilios sobre la cola
python (legacy exploration tool, removed) extract/f0/*.msg --strings
#   -> en los 81 ficheros: 0 cadenas ASCII/UTF-16 (solo el magic)
```

## 7. Preguntas abiertas (siguiente sesión F1)

1. **¿Qué es `count` (0x34/0x54)?** ¿nº de mensajes o de glifos? Contrastar con
   la tabla de 2 B: ¿son offsets, anchos, o celdas?
2. **Endianness y mapa código→glifo.** Confirmar BE y cómo se relaciona un código
   (`0x039B`) con una celda del atlas: ¿offset constante, tabla intermedia, o el
   orden del grid del `AIF`?
3. **Semántica de los controles** `0x8080/0x8081/0x808A/…`: inicio/fin de
   mensaje, color, salto de línea, iconos de botón.
4. **`0x0000`/`0x42F0`:** parámetros de los controles.
5. **Relación con el atlas:** confirmar el grid de celdas (tamaño, orden) del
   `AIF` y cruzar con los códigos observados.
6. **La tabla de 2 B:** ¿métricas por glifo (ancho/alto), o algo más?
7. **El recurso atípico** de 768 B (`d1_ud1_000A4000_004`, `data_start=0`,
   `count=278`, `field50=1`): ¿es un recurso sólo-fuente sin mensajes?

### Próximos experimentos sugeridos

1. **Descartado:** la tabla de 2 B **no** son offsets (probado en BE y LE: no
   caben en la sección ni son monótonos).
2. **Decisivo:** localizar en `default.xex` la función que consume estos códigos
   (F2). El bucle que recorre el flujo y consulta la fuente resolvería de golpe
   endianness, rango de códigos y semántica de controles. **Se recomienda
   adelantar F2 aunque F1 no esté cerrado.**
3. **Barato:** renderizar una hipótesis de mapeo (código → celda del atlas) para
   un recurso pequeño y comparar el resultado con una cadena de UI conocida del
   juego (p. ej. un menú), lo que confirmaría el mapa.

## 8. Revisión desde F2 (recompilación)

Tras regenerar el C++ del juego con ReXGlue (ver `message-subsystem.md`) y
escanear los contenedores:

- **No hay texto plano en ningún sitio.** Ni en los 81 `.msg` ni en los ~5 GB de
  los contenedores del Disco 1 aparece diálogo legible (solo nombres de shaders
  y binario). Confirma que el texto va **codificado**.
- **El ejecutable contiene descriptores de fuente embebidos**: 4 copias de
  `MessageConvertLib_1.0.0.0` acompañadas de `MessageConvertLib.dll` y nombres
  `FOT-… Std B` (fuentes OpenType japonesas), precedidas de tablas de códigos
  Shift-JIS. Es decir, la cabecera `MessageConvertLib` es, al menos en parte,
  **metadatos de fuente**.
- **La cabecera del `.msg` encaja con una fuente**: `count` (0x34/0x54) ≈ nº de
  glifos y coincide con las celdas del atlas (`atlas_w/cell_w × atlas_h/cell_h`;
  p. ej. 768/32 × 768/32 = 576 ≥ 557); la tabla de 2 B (0x28) parece **métricas
  por glifo**, no offsets.

**Conclusión provisional:** un `RMD-` es un recurso de **fuente** (atlas `AIF` +
métricas + tabla de códigos) y, muy probablemente, **también** contiene las
cadenas del juego como secuencias de códigos de glifo. Separar "metadatos de
fuente" de "mensajes" dentro del `.msg` es el objetivo inmediato de F1.

**Experimento decisivo pendiente:** cruzar los códigos del `.msg` con el grid del
atlas y renderizar; o fijar el parser vía Ghidra (F2). Ver
`message-subsystem.md §6`.

## 9. Mapeo código→glifo — **CONFIRMADO** (F5)

Renderizando códigos del `.msg` sobre el atlas se confirma:

> **`celda = código − 0x200`**, en una rejilla **24×24 de celdas de 32 px**
> (para el atlas 768×768; `celdas = (ancho/32) × (alto/32)`).

Prueba: al pintar la secuencia de códigos del *pool* del `.msg` grande se obtienen
**glifos legibles** (no basura), lo que fija el mapa. Ver
`extract/f2/pool_LE.png` / `pool_mosaic.png`.

## 10. Revisión clave: el `.msg` es la **fuente**, no los mensajes

El volcado exhaustivo en runtime (F5) y el análisis del `.msg` muestran que:

- El `.msg` grande es un **recurso de fuente**: `count` (0x34) = nº de glifos (557,
  ≈ celdas del atlas), la tabla de 2 B (0x28) = métricas, y el *pool* (0x24..0x28)
  son **datos de glifos** (al renderizar como celdas salen glifos sueltos, no
  frases).
- En runtime, las ventanas de mensaje (`CMessageWnd`, vtable `0x8206A6D4`) guardan
  un **descriptor de fuente** en `+0x10` y un id/contexto en `+0x28`; el texto se
  resuelve y se convierte a glifos al dibujar (librería `0x826Dxxxx`).
- `sub_82147140` es un **registro de objetos** (tabla hash `clave→objeto`), no la
  resolución de mensaje.
- El texto **no** está en ASCII en ningún contenedor → va **codificado** (códigos
  de glifo).

**Conclusión:** el mapa código→glifo está resuelto; **falta localizar dónde se
guardan las secuencias de códigos de cada mensaje** (no están en el `.msg`, que es
la fuente). Candidatos: los `SCE-`/SNC u otro recurso; o la propia fuente guarda
un *cmap* y los mensajes usan índices de carácter. Esto se cierra con la traza
dinámica ya montada (hooks en `extract/f2/recomp/src/translation_trace.h`).

## 11. Búsqueda exhaustiva de cadenas conocidas (sin éxito) — F5

Con cadenas **conocidas del juego** (aportadas por el usuario): `"Of Course."`,
`"PRESS START"`, `"Options"`, `"New Game"`,
`"A charm imbued with magic that suppresses ambition."`.

Búsquedas realizadas, todas con **0 coincidencias**:

| Búsqueda | Alcance |
|---|---|
| ASCII de las cadenas | ISO disc1 **y** disc2 completas + `default.exe` |
| UTF-16 LE/BE de las cadenas | ISO disc1 y disc2 + exe |
| Glyph-codes `celda+base` (bases 0, 0x20, 0x40, 0x100, 0x200, 0x300; u16 BE/LE y bytes) | ISO disc1, los 81 `.msg` y 14.107 recursos |
| Regiones densas en códigos `0x02XX`/`0x80XX` | contenedores disc1 (`ud1`,`ud2`) |
| 14.107 recursos **descomprimidos** (SLZ) | disc1 `ud1` |

**Conclusión:** el texto no está en ASCII/UTF-16 ni como códigos de glifo con esas
bases. Se almacena con un ***cmap* propio del motor** (código de carácter → glifo).
Para cerrarlo hay que **decodificar el cmap** (probablemente en los registros de
16 B del `.msg`: `(A=índice, B=0,3,6,35,70,83,90,103…)`, o en la carga de fuente
del binario) o **capturar en runtime** el punto donde se leen los códigos.

> Nota: la búsqueda ASCII inicial exigía runs ≥16; se repitieron las búsquedas con
> subcadenas cortas (`"imbued"`, `"Course"`) para descartar falsos negativos.

## 12. Tabla del `.msg`: `(A, B, C, D)` — hallazgo F5

Los registros de 16 B del `.msg` (desde `0x80` hasta `msg_section_off`) son:

| Campo | Interpretación observada |
|---|---|
| `A` (u32) | id/código, con **rangos**: `1..201`, `10000..10446`, `100001..`, … |
| `B` (u32) | **offset** (hasta ≈ tamaño del *pool*: `1248283`); 16 647 valores distintos |
| `C`, `D` (f32) | `20.0/26.0`, `164.0/26.0`… (¿posición/avance?) |

- Hay **16 648 registros** en el `.msg` grande.
- El *pool* (`[0x24,0x28)`) es una **secuencia de glyph-codes** (`0x02XX`), continua.
- **Renderizar** el pool en los offsets `B` con `celda = código − 0x200` produce
  **glifos reales pero secuencias ilegibles** → ese mapeo directo **no** es el de
  los mensajes (probablemente hay un *cmap* intermedio, o el *pool* es la
  cobertura/orden de glifos de la fuente y los textos van aparte).

**Estado:** la tabla `id→offset` es sólida; la **decodificación final del texto**
(qué codificación usan los mensajes y cómo se mapea a glifos) sigue pendiente.

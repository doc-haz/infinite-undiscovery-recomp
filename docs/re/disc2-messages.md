# Disco 2 — censo y decodificación de mensajes (`RMD-`)

> **Estado:** completado. Extracción F0 del Disco 2 + decodificación F1 de sus 45
> bancos de mensajes, con fusión disc1+disc2.
> **Herramientas:** `tools/extract_rmd.py` (F0, extrae y parte `RMD-`) y
> `(legacy exploration tool, removed)` (F1, decodifica `.msg` → JSON y fusiona volcados).
> **Formato:** ver `docs/re/font-loader.md` y `docs/re/rmd-format.md`.

## 1. Resumen

| Disco | Contenedor | `RMD-` | Registros de mensaje |
|---|---|---:|---:|
| Disc 1 | `d1_ud1` | 5 | 18 113 |
| Disc 1 | `d1_ud2` | 31 | 18 266 |
| **Disc 1 (total)** | | **36** | **36 379** |
| Disc 2 | `d2_ud1` | 14 | 16 741 |
| Disc 2 | `d2_ud2` | 31 | 18 266 |
| **Disc 2 (total)** | | **45** | **35 007** |
| **Censo completo** | | **81** | **71 386** |
| **Únicos (sin duplicar `ud2` ni las 2 fuentes compartidas)** | | **48 bancos** | **36 473** |

- **Disc 2: 45 `RMD-`** (14 en `ud1.bin` + 31 en `ud2.bin`), tal como preveía el
  censo. Extraídos a `extract/f0_d2/` (`.aif` + `.msg` + `.png`).
- Volcado a `extract/messages/all_codes_d2.json` (45 stems, 35 007 registros).
- Fusión en `extract/messages/all_codes.json` (81 stems, 71 386 registros) con
  `origin` por banco y `identical_to` para los bancos idénticos entre discos.
- **`ud2.bin` es idéntico entre discos a nivel de recurso**: los 31 `RMD-` de
  `d1_ud2` y `d2_ud2` son **byte a byte iguales** (`.msg` con SHA-256 idéntico),
  solo cambia su **posición** dentro del contenedor (de ahí que las etiquetas
  `_<offset>_` difieran). Marcados `origin = d1,d2`.
- Además de los 31 de `ud2`, otros **2 bancos de `ud1` son idénticos** entre
  discos: el atlas de fuente sin mensajes (`..._000A4000_004`, 640×448) y la
  fuente principal (`..._000A4000_005`, 768×768). Total **33 pares idénticos**.

## 2. Diferencias con el Disco 1

### 2.1 `ud2.bin` — idéntico

Los 31 bancos de `d1_ud2` y `d2_ud2` decodifican a exactamente el mismo contenido
(mismos `count`, `atlas`, `n_recs` y `msgs`) y sus `.msg` son byte a byte iguales.
Solo difieren las etiquetas porque el contenedor coloca los recursos en otros
offsets:

| | Disc 1 | Disc 2 |
|---|---|---|
| RMD- | 31 | 31 |
| Registros | 18 266 | 18 266 |
| Contenido | idéntico | idéntico |

### 2.2 `ud1.bin` — contextos distintos

| Conjunto | Bancos | Detalle |
|---|---:|---|
| Solo Disc 1 | 3 | `d1_ud1_317BD800_037`, `d1_ud1_34C6D000_019`, `d1_ud1_39513800_022` |
| Compartidos | 2 | `d1_ud1_000A4000_004`, `d1_ud1_000A4000_005` (fuentes) |
| Solo Disc 2 | 12 | `d2_ud1_59425000_042`, `d2_ud1_5B259800_027`, `d2_ud1_5E3DD000_031`, `d2_ud1_60F05800_035`, `d2_ud1_63366800_048`, `d2_ud1_6623B000_023`, `d2_ud1_68653000_037`, `d2_ud1_6ABA3000_024`, `d2_ud1_6BCF7000_038`, `d2_ud1_6DFC5000_029`, `d2_ud1_7281F800_035`, `d2_ud1_78EA4000_017` |

Disc 1 tiene 5 `RMD-` en `ud1` (18 113 registros); Disc 2 tiene 14 (16 741).
Salvo los dos bancos de fuente compartidos, **cada disco aporta sus propios
bancos de mensajes en `ud1`** (menús/escenas propios de cada disco); el
diccionario de mensajes de escenas comunes vive en `ud2` (idéntico).

## 3. Disc 2 — `stem → nº mensajes → atlas (w×h)`

| Stem (banco) | Contenedor | Nº mensajes | Atlas (w×h) | `count` (glifos) | `origin` |
|---|---|---:|---|---:|---|
| `d2_ud1_000A4000_004` | ud1 | 0 | 640×448 | 278 | d2 |
| `d2_ud1_000A4000_005` | ud1 | 16647 | 768×768 | 557 | d2 |
| `d2_ud1_59425000_042` | ud1 | 16 | 256×128 | 22 | d2 |
| `d2_ud1_5B259800_027` | ud1 | 2 | 256×128 | 28 | d2 |
| `d2_ud1_5E3DD000_031` | ud1 | 2 | 256×128 | 24 | d2 |
| `d2_ud1_60F05800_035` | ud1 | 8 | 256×256 | 51 | d2 |
| `d2_ud1_63366800_048` | ud1 | 16 | 128×64 | 7 | d2 |
| `d2_ud1_6623B000_023` | ud1 | 2 | 256×128 | 31 | d2 |
| `d2_ud1_68653000_037` | ud1 | 2 | 256×128 | 30 | d2 |
| `d2_ud1_6ABA3000_024` | ud1 | 2 | 256×128 | 26 | d2 |
| `d2_ud1_6BCF7000_038` | ud1 | 2 | 256×128 | 25 | d2 |
| `d2_ud1_6DFC5000_029` | ud1 | 32 | 384×320 | 100 | d2 |
| `d2_ud1_7281F800_035` | ud1 | 2 | 256×128 | 30 | d2 |
| `d2_ud1_78EA4000_017` | ud1 | 8 | 384×384 | 127 | d2 |
| `d2_ud2_345EE000_002` | ud2 | 898 | 1024×832 | 828 | d2 |
| `d2_ud2_34C05800_018` | ud2 | 6 | 256×128 | 32 | d2 |
| `d2_ud2_36740800_028` | ud2 | 26 | 256×192 | 42 | d2 |
| `d2_ud2_36BC3000_008` | ud2 | 14 | 384×256 | 78 | d2 |
| `d2_ud2_38330000_037` | ud2 | 248 | 768×576 | 418 | d2 |
| `d2_ud2_3B9D4000_010` | ud2 | 978 | 1024×960 | 909 | d2 |
| `d2_ud2_3F7BF000_028` | ud2 | 162 | 640×576 | 354 | d2 |
| `d2_ud2_4278A800_039` | ud2 | 60 | 512×512 | 252 | d2 |
| `d2_ud2_46130800_023` | ud2 | 438 | 768×640 | 437 | d2 |
| `d2_ud2_4BB9A000_003` | ud2 | 1726 | 1152×1152 | 1274 | d2 |
| `d2_ud2_50CA7800_020` | ud2 | 666 | 896×768 | 661 | d2 |
| `d2_ud2_573EC000_010` | ud2 | 1132 | 1024×896 | 884 | d2 |
| `d2_ud2_5D40A000_004` | ud2 | 2418 | 1280×1088 | 1321 | d2 |
| `d2_ud2_681B9800_007` | ud2 | 556 | 896×768 | 624 | d2 |
| `d2_ud2_6C36B800_037` | ud2 | 82 | 512×448 | 198 | d2 |
| `d2_ud2_6E3A5800_025` | ud2 | 338 | 768×576 | 431 | d2 |
| `d2_ud2_72306800_026` | ud2 | 184 | 640×576 | 323 | d2 |
| `d2_ud2_758E5000_058` | ud2 | 34 | 256×128 | 23 | d2 |
| `d2_ud2_770B3000_017` | ud2 | 1608 | 1152×1152 | 1247 | d2 |
| `d2_ud2_7D681800_033` | ud2 | 48 | 512×384 | 164 | d2 |
| `d2_ud2_7FFBD000_014` | ud2 | 8 | 256×192 | 42 | d2 |
| `d2_ud2_8178A800_017` | ud2 | 286 | 768×640 | 438 | d2 |
| `d2_ud2_86D7E800_009` | ud2 | 2346 | 1280×1216 | 1490 | d2 |
| `d2_ud2_94B69800_030` | ud2 | 116 | 640×512 | 282 | d2 |
| `d2_ud2_985B2800_042` | ud2 | 46 | 512×320 | 159 | d2 |
| `d2_ud2_9BED0800_011` | ud2 | 2566 | 1280×1152 | 1432 | d2 |
| `d2_ud2_AB4C9000_046` | ud2 | 76 | 512×512 | 238 | d2 |
| `d2_ud2_AE9A0800_026` | ud2 | 366 | 768×640 | 449 | d2 |
| `d2_ud2_B33E2000_017` | ud2 | 332 | 768×640 | 470 | d2 |
| `d2_ud2_BD417000_005` | ud2 | 498 | 896×704 | 581 | d2 |
| `d2_ud2_C2072800_040` | ud2 | 4 | 512×320 | 150 | d2 |

> El `count` es el nº de glifos de la fuente del banco (`0x34` del `.msg`), no el
> nº de mensajes. El atlas de cada banco es su tabla de glifos (rejilla
> `atlas_w/32 × atlas_h/32`, row-major). `origin` indica en qué discos aparece un
> banco con ese contenido exacto.

## 4. Estructura del JSON

`extract/messages/all_codes_d2.json` — banco (stem) como clave, **sin fusionar
stems distintos**:

```json
{
  "d2_ud2_345EE000_002": {
    "count": 828,
    "atlas": [1024, 832],
    "n_recs": 898,
    "msgs": { "1": [301, 302, ...], "2": [...] }
  }
}
```

`extract/messages/all_codes.json` (fusión) añade a cada banco:

- `origin`: lista de discos donde aparece ese contenido (`["d1"]`, `["d2"]` o
  `["d1","d2"]`).
- `identical_to`: lista de stems (en otros discos) con contenido idéntico
  (presente solo en los 33 pares).

## 5. Anomalías y notas metodológicas

1. **`extract/f0` ya contenía ambos discos.** Se generó originalmente con
   `--roms roms`, no solo con el Disco 1. Por eso el `all_codes.json` previo
   (81 stems, 71 386 registros) **nunca fue "solo Disco 1"**: era ya el censo
   completo. Se ha reconstruido como fusión explícita a partir de los volcados
   por disco (`all_codes_d1.json` = 36 stems, `all_codes_d2.json` = 45 stems).
2. **"`ud2.bin` idéntico" se confirma a nivel de recurso, no de bytes del
   contenedor.** Los `.msg` de los 31 pares son idénticos; difieren las
   posiciones/etiquetas dentro de `ud2.bin`.
3. **Recurso atípico de solo fuente.** `d2_ud1_000A4000_004` (`data_start=0`,
   `n_recs=0`, `count=278`, 640×448) no tiene directorio de mensajes; se volcó con
   `msgs={}`. Es el gemelo del `d1_ud1_000A4000_004`.
4. **Control `0x4000..0x4019`.** La decodificación de este volcado sigue la
   lectura de referencia del Disco 1 (un control = un token; no consume payload
   variable), para que ambos discos queden en la misma base. La semántica fina de
   los controles se trabaja aparte (ver `docs/re/font-loader.md`).
5. **Nº de mensajes vs. glifos.** En los bancos de tipo fuente (p. ej.
   `..._000A4000_005`, `n_recs=16 647`, `count=557`) los "mensajes" del
   directorio son realmente las entradas de glifo; el `count` es el nº de glifos.

## 6. Reproducción

```bash
# F0: extraer los RMD- del Disco 2 (14 ud1 + 31 ud2 = 45)
python tools/extract_rmd.py --iso "roms/Infinite Undiscovery (Europe) (Disc 2).iso" --out extract/f0_d2 --png

# F1: decodificar Disco 1 (36) y Disco 2 (45)
python (legacy exploration tool, removed) dump --in-dir extract/f0 --only-prefix d1_ --out extract/messages/all_codes_d1.json
python (legacy exploration tool, removed) dump --in-dir extract/f0_d2 --out extract/messages/all_codes_d2.json

# Fusionar disc1 + disc2 (conserva stems, marca origen y duplicados exactos)
python (legacy exploration tool, removed) merge --out extract/messages/all_codes.json --inputs extract/messages/all_codes_d1.json extract/messages/all_codes_d2.json
```

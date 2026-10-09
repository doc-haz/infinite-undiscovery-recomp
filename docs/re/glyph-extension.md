# Extensión de glifos para castellano (F3 — RESUELTO)

> **Estado:** cerrado. Se han localizado las letras base, se ha implementado el
> **re-codificador DXT2/3** (`tools/aif_edit.py`), se han **añadido los 16 glifos**
> del castellano a un atlas de prueba y se ha validado con render + la
> codificación del juego.
>
> **Entregables:**
> - `tools/aif_edit.py` — editor AIF (decode + modificar + **encode** DXT2/3, con
>   round-trip exacto del *tiling*/orden de bytes).
> - `tools/add_accents.py` — compone y añade los 16 glifos a un banco.
> - `extract/f2/atlas005_accents.aif` + `.png` + `atlas005_accents.msg` (atlas de
>   prueba parcheado; `extract/` está fuera de git).
> - Renders: `..._accents_row.png` (los 16), `..._accents_demo.png` (frases),
>   `..._accents_cells.png` (antes/después).
>
> **Conclusión:** los 16 caracteres `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡` **no existen**
> en el atlas latino principal (`d1_ud1_000A4000_005`); se **añaden reutilizando
> celdas libres** (los 19 huecos finales de la rejilla) sin tocar el tamaño del
> `AIF`, actualizando `count` y la tabla de métricas del `.msg`. El texto inglés
> no usa kana/kanji, pero en `005` **no fue necesario** reutilizarlos.

---

## 1. Qué acepta hoy el atlas

La rejilla completa del atlas de `005` (768×768 → 24×24 celdas de 32 px) está
renderizada y etiquetada con el índice de celda. Evidencia:

* Rejilla completa: `extract/f2/grid_d1_ud1_000A4000_005.png`
* Montajes por zonas: `extract/f2/m_005_000_071.png`, `m_005_096_167.png`,
  `m_005_456_527.png`

Contenido observado en `005` (celdas 0–556, `count = 557`):

| Zona | Contenido |
|---|---|
| ASCII | mayúsculas, minúsculas, dígitos, puntuación (`.,:;!?%&*/+=()[]<>"'|_-`) |
| Símbolos | `@`, `#`, `→ ↓ ↑`, `©`-like, etc. |
| Kana / kanji | a partir de la celda ~360 (katakana + kanji) |
| **Castellano extendido** | **ninguno** de `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡` |

Localización exacta de las **letras base** usadas para componer (celda → código
= 301 + celda):

| base | celda | código | base | celda | código | base | celda | código |
|---|---:|---:|---|---:|---:|---|---:|---:|
| `a` | 97 | 398 | `A` | 130 | 431 | `?` | 116 | 417 |
| `e` | 131 | 432 | `E` | 140 | 441 | `!` | 143 | 444 |
| `i` | 128 | 429 | `I` | 124 | 425 | `o` | 100 | 401 |
| `u` | 104 | 405 | `U` | 118 | 419 | `n` | 101 | 402 |
| `c` | 142 | 443 | `N` | 125 | 426 | `O` | 105 | 406 |

*(Hay varias copias de cada letra en el atlas; se eligió una copia «limpia».)*

**Conclusión:** la cobertura latina es ASCII, no latino extendido. Faltan los 16.

---

## 2. Re-codificación DXT2/3 (`tools/aif_edit.py`)

El atlas es un `AIF ` con cabecera de **4096 B** y textura **DXT2/3** en el
*layout* de memoria de la GPU de Xbox 360. `aif.py` (vendor) sólo decodifica;
`aif_edit.py` añade el camino inverso:

1. **Bloques lineales → tiled**: para cada bloque 4×4 `(bx,by)` se escribe en la
   posición `tiled_offset(bx,by,tiled_width,16)` (la misma función que usa el
   decodificador).
2. **Orden de bytes**: cada palabra de 16 bits se intercambia (`DXT` está en
   big-endian de 16 bits en disco y el decodificador de PC espera little-endian).
   Es una involución sobre las parejas de bytes.
3. **Cabecera**: se conserva la original (4096 B) y, tras el *base level*, la
   cola (mipmaps, si los hubiera).

El códec DXT3 reutiliza el *layout* exacto de `aif.py`:

```
[0:8]   16 valores alpha de 4 bits  (byte j = a[2j] | a[2j+1]<<4)
[8:10]  color0 RGB565  (little-endian)
[10:12] color1 RGB565
[12:16] 16 índices de 2 bits (little-endian)
```

`DXT2/3` decodifica siempre en modo 4 colores (alpha explícita), así que la
paleta es `[c0, c1, (2c0+c1)/3, (c0+2c1)/3]`. Los extremos de color se eligen
como el rectángulo envolvente de los píxeles visibles del bloque (para texto
blanco/negro es óptimo en la práctica).

### Validación (round-trip)

```
$ python tools/aif_edit.py selftest
[ok] DXT3 alpha por píxel exacta
[ok] DXT3 blanco/transparente exacto
[ok] fichero sin tocar se re-emite idéntico
[ok] envoltorio (tiling+byteorder) exacto en d1_ud1_000A4000_005.aif
[ok] re-encode DXT3 del atlas completo: PSNR(visible)=62.62 dB
```

* **Envoltorio exacto:** leer el fichero, reconstruir el *base level* desde los
  bloques lineales y volver a escribirlo reproduce el **pixel data original byte
  a byte**. Esto valida el inverso de `tiled_offset` + el *byte swap* (lo crítico).
* **Códec DXT3:** blanco/negro/transparente exactos; re-codificar el atlas
  completo y decodificarlo da **62.6 dB** de PSNR (sólo píxeles visibles). La
  pérdida es la cuantización RGB565 de los grises de antialiasing.
* El fichero parcheado también pasa el round-trip:

```
$ python tools/aif_edit.py roundtrip extract/f2/atlas005_accents.aif
re-emitir idéntico      : True
envoltorio exacto       : True
```

> La verificación que realmente importa para no degradar el atlas: al parchear
> **sólo se re-codifican las 16 celdas nuevas**. `add_accents.py` comprueba que
> los bytes que cambian caen **todos** dentro de esas celdas
> (`bytes fuera de las celdas nuevas: 0`), así que el resto del atlas queda
> intacto (no se recalcula ni se re-cuantiza).

---

## 3. Añadir los 16 glifos

Para cada glifo se compone **letra base + diacrítico** dibujado con
`PIL/ImageDraw`, respetando el estilo del atlas (núcleo blanco con borde negro):

| glifo | base | diacrítico | nota |
|---|---|---|---|
| `á é ó ú` | `a e o u` | acento agudo | |
| `í` | `i` | acento agudo | se borra el punto de la `i` |
| `ü` | `u` | diéresis (dos puntos) | |
| `ñ` | `n` | tilde | |
| `Á É Í Ó Ú` | `A E I O U` | acento agudo | base reducida a 26 px anclada abajo |
| `Ü` | `U` | diéresis | |
| `Ñ` | `N` | tilde | |
| `¿` | `?` | — | `?` girada 180° alrededor del centro de tinta |
| `¡` | `!` | — | `!` girada 180° |

Las mayúsculas se **reducen** (26/32) y se anclan a la línea base para dejar sitio
al acento (es lo que hacen las fuentes reales con vocales acentuadas).

### Celdas usadas

En `005` la rejilla tiene 576 celdas y `count = 557`: las **19 celdas finales**
(celdas 557–575) son un relleno blanco opaco sin usar. Se usan las **16
primeras** (557–572), que dan los códigos **858–873**:

| glifo | código | celda | (fila, col) | glifo | código | celda | (fila, col) |
|---|---:|---:|---|---|---:|---:|---|
| `á` | 858 | 557 | (23,5) | `Á` | 865 | 564 | (23,12) |
| `é` | 859 | 558 | (23,6) | `É` | 866 | 565 | (23,13) |
| `í` | 860 | 559 | (23,7) | `Í` | 867 | 566 | (23,14) |
| `ó` | 861 | 560 | (23,8) | `Ó` | 868 | 567 | (23,15) |
| `ú` | 862 | 561 | (23,9) | `Ú` | 869 | 568 | (23,16) |
| `ü` | 863 | 562 | (23,10) | `Ü` | 870 | 569 | (23,17) |
| `ñ` | 864 | 563 | (23,11) | `Ñ` | 871 | 570 | (23,18) |
| `¿` | 872 | 571 | (23,19) | `¡` | 873 | 572 | (23,20) |

### Actualización del `.msg`

`d1_ud1_000A4000_005.msg` (big-endian):

| campo | antes | después |
|---|---|---|
| `0x34` `count` | 557 | **573** |
| `0x54` `count2` | 557 | **573** |
| `0x28` tabla de métricas | 1152 B (= 576 entradas) | se rellenan las entradas 557–572 |
| `0x30` `total_size` | 1516032 | sin cambio (no crece el fichero) |
| `0x58` `aif_size` | 593920 | sin cambio (mismo tamaño de atlas) |

La **métrica** de cada glifo nuevo es la de su letra base (para `¿`/`¡`, la de
`?`/`!`). En la tabla, el **byte alto** de cada `u16` es el avance en píxeles
(confirmado: `a`=13, `e`=14, `i`=7, `o`=14, `u`=14, `n`=14, `c`=13, `A`=17,
`?`=16, `!`=9). Copiar la métrica del base mantiene el *tracking*.

> En `005` la región de métricas ya reserva 576 entradas (`total_size -
> metrics_off = 1152 B`), así que las nuevas entradas caben sin crecer el
> fichero. `add_accents.py` soporta el caso general: si la tabla queda pegada al
> final y hay que crecer, actualiza `total_size` (0x30).

---

## 4. Prueba de aceptación

### 4.1 Los 16 glifos (render desde el atlas parcheado)

`extract/f2/d1_ud1_000A4000_005_accents_row.png`:

```
á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡
```

Cada glifo se recorta de la celda re-decodificada del atlas parcheado, de modo
que lo que se ve es lo que verá la GPU.

### 4.2 Frases reales

`extract/f2/d1_ud1_000A4000_005_accents_demo.png`:

```
niño
café
¡Olé!
¿Qué?
```

Compuestas con los códigos del juego (p. ej. `niño` = `n(402) i(429) ñ(864)
o(401)`) y el avance de cada métrica. Salen correctas: la `ñ`, los acentos y los
signos de apertura.

### 4.3 Codificación del juego (`tools/game_text_codec.py`)

```
$ python tools/add_accents.py
...
codificación del juego para los 16 códigos nuevos:
  bytes : da 06 db 06 dc 06 dd 06 de 06 df 06 e0 06 e1 06 e2 06 e3 06
          e4 06 e5 06 e6 06 e7 06 e8 06 e9 06 00
  decode: [858, ..., 873]
  round-trip: OK
```

Los códigos nuevos usan el esquema de 15 bits del motor
(`(b1<<7)|(b0&0x7F)`): `858 = 0x035A` → `DA 06`. El *encode/decode* es exacto, es
decir, el motor puede **almacenar y leer** los códigos nuevos sin cambios en el
códec.

### 4.4 Cadena de aceptación completa (por el códec del juego)

`extract/f2/d1_ud1_000A4000_005_accents_test.png` es la prueba definitiva: se
toma la cadena

```
niño café ¡Olé! ¿Qué? áéíóúüñÁÉÍÓÚÜÑ¿¡
```

se convierte a códigos, se pasa por `codec.encode` → `codec.decode` del motor
(round-trip exacto) y se renderiza con el atlas parcheado. **Los 16 caracteres
salen correctos**, incluidos `¿`/`¡` al principio, la `ñ` y las vocales con
tilde/acento/diéresis.

### 4.5 Antes/después de las celdas

`extract/f2/d1_ud1_000A4000_005_accents_cells.png` muestra las 16 celdas
originales (relleno blanco) frente a las nuevas (glifos).

---

## 5. Decisión

| Opción | Veredicto |
|---|---|
| **(a)** Usar acentos existentes | **No**: no existen en `005`. |
| **(b1)** **Reutilizar celdas libres + ampliar `count`/métricas** | **ELEGIDA.** |
| **(b2)** Ampliar el atlas (crecer la textura) | Innecesario en `005`; queda como plan para bancos sin huecos. |
| **(c)** Fold ASCII (sin acentos) | Descartado: evitable y de peor calidad. |

**Motivos:** el atlas `005` tiene 19 celdas libres finales (más 10 transparentes
internas); **no hay que redimensionar** la textura (lo que cambiaría `aif_size`,
`pitch`, la cabecera y el `total_size` del `.msg`). Reutilizar los huecos +
añadir métricas es el cambio mínimo y reversible.

### Cobertura en los 81 bancos

Cada `RMD-` tiene **atlas y charmap propios** (no hay charmap global), así que la
extensión se hace **por banco**. Recuento de celdas libres (transparentes o
relleno uniforme) tras `count`:

```
$ python tools/aif_edit.py survey --dir extract/f0 --need 16
...
bancos=81  con >=16 celdas libres tras count: 38
```

* 81 bancos en total; **38 tienen ≥ 16 celdas libres** tras `count` (suficiente
  para los 16 glifos sin ampliar la textura). `005` tiene 19.
* El resto tiene menos: p. ej. `d1_ud1_000A4000_004` sólo 2, `d1_ud2_1793D000_018`
  0. Para ésos habría que **reutilizar celdas de kana/kanji no usadas por el
  inglés** (el texto inglés no las emplea) o **ampliar la textura** (cambia el
  tamaño del `AIF` y exige actualizar `aif_size`, `pitch` y `total_size`).
* El estilo del glifo **no es idéntico entre bancos** (comprobado por hash: las
  letras base de `005` no coinciden byte a byte con las de `022`/`040`), por lo
  que las letras base deben localizarse **en el atlas de cada banco** (la rejilla
  y el mapa `celda = código − 301` sí son comunes).

---

## 6. Reproducción

```bash
# round-trip del códec (imprescindible antes de fiarse de nada)
python tools/aif_edit.py selftest
python tools/aif_edit.py roundtrip extract/f2/atlas005_accents.aif

# construir el atlas parcheado + renders + actualizar el .msg
python -X utf8 tools/add_accents.py

# recuento de celdas libres de todos los bancos (para decidir dónde añadir)
python -X utf8 tools/aif_edit.py survey --dir extract/f0 --need 16
```

---

## 7. Limitaciones / siguientes pasos

1. La extensión de `005` es una **prueba de concepto** de la técnica; para la
   traducción completa hay que repetirla en **cada banco** que renderice texto
   (con sus letras base y sus huecos).
2. La localización de letras base en bancos distintos de `005` es **manual/asistida
   por render** (`python tools/aif_edit.py montage <bank.aif> <inicio> <n>`);
   automatizarla requiere *template matching* tolerante (los glifos no son
   idénticos entre bancos).
3. `aif_edit.py` sólo implementa el camino de **DXT2/3** para escribir (los 81
   atlas son DXT2/3); también incluye codificadores DXT1/DXT5 por completitud,
   pero no están ejercitados en el corpus.
4. El hook de runtime debe usar los **códigos nuevos** (858–873) de cada banco en
   la tabla `es.json`/`es_codes.json`; el motor no necesita cambios.

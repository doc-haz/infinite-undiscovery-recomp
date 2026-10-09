# Cargador de fuente y decodificador de mensajes (F2 — RESUELTO)

> **Estado:** localizados el parser del `.msg`, el decodificador del texto y el
> mapeo código→glifo, **en el C++ generado por ReXGlue** (fuente fiable).
> Método: análisis estático del código generado a partir de `default.xex`
> (perfil EUROPE/PAL, base plana `VA = 0x82000000 + offset_de_fichero`).
>
> **Novedad frente a `message-subsystem.md`:** el `.msg` **sí** contiene los
> mensajes. No es sólo metadatos de fuente: contiene un **directorio de mensajes**
> `id → (offset, ancho, alto)` y un **pool** con las cadenas codificadas.

## 0. Resumen ejecutivo

El subsistema de texto vive en la **librería `0x826Cxxxx`–`0x826Exxxx`** y gira
en torno a la clase `CMessageTextWnd` (vtable `0x8206AF4C`). La cadena completa
es:

```
// NOTA: el C++ generado usa offsets DECIMALES de struct.  Aquí se indican
// decimal y (hex) para evitar confusión.
CMessageTextWnd
  +84   (0x54) (byte) bandera "necesita recalcular"
  +96   (0x60) (ptr)  recurso de fuente "pequeña"  (códigos < 300)
  +100  (0x64) (ptr)  recurso de fuente "grande" = base del .msg  (códigos >= 300)
        +0x20 records_off   +0x24 pool_off   +0x28 metrics_off   (offsets del .msg)
  +108  (0x6C) (ptr)  segundo recurso asociado a la clave de fuente
  +120  (0x78) (ptr)  puntero al texto YA RESUELTO (pool + offset del registro)
  +124  (0x7C) (ptr)  objeto de fuente concreto (font page/atlas)
  +132  (0x84) (u32)  clave de recurso de fuente  (font id, low 16 bits)
  +136  (0x88) (u32)  **id de mensaje** que se debe mostrar
  +140  (0x8C) contador de avance
  +144  (0x90) contador de líneas
  +148  (0x94) (u16)  flags de estado
  +196  (0xC4) (f32)  ancho resuelto   (= registro.C)
  +200  (0xC8) (f32)  alto resuelto    (= registro.D)
  +248  (0xF8) (int)  glifos emitidos en la pasada
  +252  (0xFC) (ptr)  buffer de vértices/glifo actual
  +264  (0x108)(f32)  avance acumulado
```

Flujo de resolución (por frame, en el slot 51 de la vtable `CMessageTextWnd`,
`sub_826E73E8`):

1. `sub_826D3E20(obj)` → `obj+100 = sub_826D2F88(fontMgr, key)` — obtiene la
   **base del `.msg`** a partir de la clave de fuente `obj+132`.
2. `sub_826D52C0(obj)` — **parsea el directorio del `.msg`**: recorre los
   registros de 16 B buscando `A == obj+136`, y deja en `obj+120` el puntero
   al texto (`pool + B`) y en `obj+196/200` el ancho/alto (`C`, `D`).
3. `sub_826D43F8(obj)` — **decodifica y dibuja**: recorre el byte-stream de
   `obj+120`, decodifica los códigos de carácter, procesa los códigos de
   control (`0x4000..0x4019`) y emite glifos con `sub_826D3268`.
4. `sub_826D3268(obj, code, ...)` — **mapea `código → índice de glifo`**
   (`índice = código - 301` para códigos ≥ 300) y lee las métricas de la tabla
   `metrics_off` del `.msg`.

## 1. Formato del `.msg` (revisado y confirmado)

Cabecera big-endian a partir del inicio del bloque `MessageConvertLib`:

| Offset | Campo | Significado (confirmado) |
|---|---|---|
| `0x00` | magic | `"MessageConvertLib_1.0.0.0"` + NULs |
| `0x20` | `records_off` | inicio del **directorio de mensajes** (típ. `0x80`; `0` si no hay mensajes) |
| `0x24` | `pool_off` | inicio del **pool de texto** codificado |
| `0x28` | `metrics_off` | inicio de la **tabla de métricas** (2 B × `count`) |
| `0x2C` | — | 0 |
| `0x30` | `total_size` | tamaño total del `.msg` |
| `0x34` | `count` | nº de glifos (`= count2`) |
| `0x38`/`0x3C` | `f38`/`f3c` | `512` o `800` |
| `0x40` | `f40` | `28`/`30`/`34` |
| `0x44`/`0x48` | `cell_w`/`cell_h` | `32`/`32` |
| `0x4C` | `bytes_per_code` | `2` |
| `0x50` | `f50` | `301` (con mensajes) o `1` (sólo fuente) |
| `0x54` | `count2` | `= count` |
| `0x58` | `aif_size` | longitud del atlas `AIF ` |
| `0x5C` | `magic2` | `0x0131F508` (constante) |
| `0x60`/`0x64` | `atlas_w`/`atlas_h` | dimensiones del atlas |

### 1.1 Directorio de mensajes `[records_off, pool_off)`

Registros de **16 bytes**, big-endian:

| Offset | Tipo | Campo |
|---|---|---|
| `+0` | u32 BE | `id` — **id de mensaje** (rangos `1..201`, `10000..`, `100001..`, …) |
| `+4` | u32 BE | `offset` — **desplazamiento en bytes dentro del pool** |
| `+8` | f32 BE | `width` — ancho de la cadena renderizada |
| `+12` | f32 BE | `height` — alto (p. ej. `26.0` = una línea) |

El **terminador** del directorio es un registro con `id == 0 && offset == 0`
(el último registro real del `.msg` grande es `A=0x4C4B400, B=1248283`… y el
siguiente es `A=0, B=0`). El bucle del parser es exactamente:

```cpp
r11 = base + base[0x20];          // records
while (!(r11[0]==0 && r11[4]==0)) {
    if (r11[0] == obj[0x136]) break;   // id encontrado
    r11 += 16;
}
```

### 1.2 Pool `[pool_off, metrics_off)` — codificación del texto

Cada mensaje es un **byte-stream** terminado en `0x00`. La codificación es un
esquema propio (decodificado en `sub_826D30B8` y `sub_826D43F8`):

```
b0 = *p++;
if (b0 & 0x80):
    b1 = *p++;
    code = (b1 << 7) | (b0 & 0x7F);   // código de 15 bits
else:
    code = b0;                        // ASCII extendido de 1 byte
```

* `code == 0x0000` → **fin de mensaje**.
* `code >= 0x4000` → **código de control** (pulen mediante una tabla de salto de
  **26 entradas**, `0x4000..0x4019`; el índice es `code - 0x4000`).
* `code < 0x4000` → **código de carácter/glifo**.

Ejemplo real (`d1_ud2_A4DAA000_040.msg`, mensaje `A=1`):
`8a 80 | 02 | 01 | ad 02 ae 02 … ba 02 | 87 80 | 00` →
`400A, 0x02, 0x01, 12D, 12E … 13A, 4007, <fin>`.

### 1.3 Mapeo código → glifo (`sub_826D3268`)

* `code <  300` → tabla = `obj+96`; índice `= code - 1`.
* `code >= 300` → tabla = `obj+100` (la base del `.msg`); índice `= code - 301`.
* Métrica del glifo = byte en `tabla + tabla[0x28] + 2*índice`
  (`tabla[0x28]` = `metrics_off`).

Comprobación: el `.msg` de `040` tiene `count = 150` y sus mensajes usan
exactamente los códigos **301..450** → índices **0..149**. Para el `.msg` grande
(`005`, `count = 557`) el rango equivalente es `301..857`.

**Validación empírica (render del atlas):** pintando la celda
`glyph_index` en la rejilla del atlas `AIF` con
`cols = atlas_w / cell_w`, `rows = atlas_h / cell_h` y orden **row-major**, los
mensajes salen **legibles**:

* `040` A=1 → `セラフィックゲートはようこそ` ("Welcome to the Seraphic Gate")
* `040` A=3 → `マエストロを手に入れた` ("Got the Maestro")

(Con `cols = 16 = 512/32` el resultado es correcto; con otras columnas, ilegible.)
Esto confirma de forma independiente: `índice = código - 301`, rejilla
`atlas_w/cell_w × atlas_h/cell_h`, orden row-major.

> **Conclusión:** el texto **no** usa ASCII/UTF-16 ni códigos de celda directos;
> usa un **alfabeto de códigos de carácter propio** (`1..299` = fuente pequeña,
> `301..` = fuente principal), y cada código se mapea a un glifo del atlas por
> `índice = código - 301`. La tabla de 2 B del `.msg` es la **tabla de métricas
> por glifo**, no offsets.

## 2. Funciones localizadas

| Dirección | Partición | Papel | Confianza |
|---|---|---|---|
| `sub_826D52C0` | 112 | **Parser del directorio `.msg`** (`id → texto`) | **Alta** |
| `sub_826D2F88` | 21 | Fetch del recurso de fuente por clave (tabla stride 28) | Alta |
| `sub_826D3E20` | 26 | Atajo `obj+100 = sub_826D2F88(fontMgr, key)`, devuelve éxito | Alta |
| `sub_826D49B0` | 48 | Fetch secundario (`obj+108`), vía `sub_824A3130` | Media |
| `sub_826D43F8` | 140 | **Decodificador/renderizador** (bucle + tabla de control 26) | **Alta** |
| `sub_826D30B8` | 120 | Contador de avance (misma decodificación, sólo longitud) | Alta |
| `sub_826D3268` | 72 | **Mapa código→glifo** + métricas | **Alta** |
| `sub_826E73E8` | 64 | Slot 51 de `CMessageTextWnd`: orquesta la resolución | Alta |
| `sub_826E4B48` | 42 | Setter de la clave de fuente `obj+132` | Media |
| `sub_826D42E8` | 64 | Reset del objeto de texto (pone a 0 `+96/+100/+120/+132/+136`) | Alta |
| `sub_821EF678` | 81 | Cargador de chunk `AIF ` (variante A) | Alta |
| `sub_821EF618` | 45 | Cargador de chunk `AIF ` (variante B) | Alta |
| `sub_8221D768` | 14 | Dispatcher de chunks `AIF `/`eof_` | Alta |
| `sub_822226D8` | 74 | Dispatcher de chunks de recurso (`AIF `/…/`eof_`) | Alta |

## 3. Cuerpos C++ (evidencia)

### 3.1 `sub_826D52C0` — parser del directorio `.msg` (núcleo)

```cpp
DEFINE_REX_FUNC(sub_826D52C0) {
	// ... prólogo ...
	// mr r31,r3                     ; r31 = CMessageTextWnd*
	// lwz r10,100(r31)              ; r10 = base del .msg
	// cmpwi r10,0 ; bne ...         ; si no hay recurso -> return 0
	// ...
	// lwz r11,116(r31)              ; r11 = cursor (o base)
	// lwz r9,36(r11) ; lwz r11,32(r11)
	// add r9,r9,r10                 ; r9 = base + [base+0x24]  = POOL
	// add r11,r11,r10               ; r11 = base + [base+0x20] = RECORDS
	// ---------------------------------------------------------------
	// bucle de búsqueda del registro por id:
	while (true) {
		uint32_t A = REX_LOAD_U32(r11 + 0);   // id del registro
		if (!(A == 0 && REX_LOAD_U32(r11 + 4) == 0)) {
			uint32_t target = REX_LOAD_U32(r31 + 136);   // id pedido
			if (A == target) break;
			r11 += 16;                                    // siguiente registro
			continue;
		}
		break;   // terminator (0,0)
	}
	// ---------------------------------------------------------------
	// registro encontrado:
	uint32_t off = REX_LOAD_U32(r11 + 4);     // B = offset en el pool
	REX_STORE_U32(r31 + 120, off + pool);     // obj+120 = pool + B  (TEXTO)
	REX_STORE_U32(r31 + 196, REX_LOAD_U32(r11 + 8));   // C  -> ancho
	REX_STORE_U32(r31 + 200, REX_LOAD_U32(r11 + 12));  // D  -> alto
	// ...
}
```

### 3.2 `sub_826D3E20` — de clave de fuente a base del `.msg`

```cpp
DEFINE_REX_FUNC(sub_826D3E20) {
	// mr r31,r3
	// lis r10,-32092 ; addi r10,r10,-32520        ; r10 = &singleton (0x82A380F8)
	// lwz r11,132(r31)                             ; clave de fuente
	// clrlwi r4,r11,16                             ; key & 0xFFFF
	// lwz r3,52(r10)                               ; fontMgr = [0x82A3812C]
	// bl 0x826D2F88                                ; r3 = base del .msg
	// stw r3,100(r31)                              ; obj+100 = base del .msg
	// ...
}
```

### 3.3 `sub_826D2F88` — fetch por índice (stride 28)

```cpp
DEFINE_REX_FUNC(sub_826D2F88) {
	// mr r31,r3 ; mr r30,r4
	// bl 0x826D6858            ; valida que (key & 0xFFFF) <= 10
	// ... si no, return 0
	// clrlwi r11,r30,16        ; key & 0xFFFF
	// addi r11,r11,1           ; (key+1)
	// mulli r11,r11,28         ; * 28
	// lwzx r3,r11,r31          ; return fontMgr[(key+1)*28]  (u32[0] = base .msg)
}
```

### 3.4 `sub_826D43F8` — decodificador y renderizador (fragmento clave)

```cpp
	// r31 = CMessageTextWnd, r30 = obj->text (obj+120), r27 = font (obj+124)
loc_826D47A0:
	// lbz r4,0(r30)            ; r4 = *p++   (primer byte del carácter)
	// addi r30,r30,1
	// cmpwi r4,0 ; bne loc_826D44B4     ; 0 = fin de mensaje
loc_826D44B4:
	// rlwinm. r11,r4,0,24,24   ; if (r4 & 0x80)
	// beq loc_826D474C          ;   carácter de 1 byte
	// lbz r11,0(r30) ; addi r30,r30,1
	// rlwimi r4,r11,7,0,24      ;   r4 = (b1<<7) | (r4 & 0x7F)
	// rlwinm. r11,r4,0,17,17    ; if (!(r4 & 0x4000)) -> carácter normal
	// beq loc_826D474C
	// addi r11,r4,-16384        ; índice de control = r4 - 0x4000
	// cmplwi r11,25 ; bgt ...
	// lbzx r0,rTabla ; ...      ; tabla de salto de 26 entradas (0x4000..0x4019)
	// switch (r11) { ... }
loc_826D474C:                     // carácter normal:
	// clrlwi r11,r21,24 ; cmplwi r11,1 ...
	// clrlwi r20,r4,16          ; r20 = código de carácter
	// ...
	// li r7,1 ; li r6,0 ; mr r5,r20 ; mr r3,r31
	// bl 0x826D3268             ; EMITIR GLIFO (código -> glifo)
```

### 3.5 `sub_826D3268` — mapa código→glifo y métricas (fragmento clave)

```cpp
DEFINE_REX_FUNC(sub_826D3268) {
	// r4 = código de carácter, r5 = ...
	// addis r10,r4,1 ; addi r10,r10,-1 ; sth r10,42(glyph)   ; glyph[42] = code-1
	// lwz r10,100(r3)              ; por defecto: tabla = base del .msg
	// cmplwi r4,300
	// bge  loc_826D32A0
	//   lwz r10,96(r3) ; li r9,1   ; código < 300 -> tabla pequeña
	//   b loc_826D32F0
loc_826D32A0:                        // código >= 300
	// ... si code==16400/16409: tratamiento especial
	// r9 = glyph[42] + 0x10000 - 300 ; sth r9,42(glyph)  ; glyph[42] = code-301
	// li r9,4
loc_826D32F0:
	// glyph[16] = tabla ; glyph[40] = r9 ; glyph[36] = code & 0xFFFF
	// if (obj[236] == 0):
	//     r9  = [tabla + 0x28]              ; = metrics_off
	//     r11 = glyph[42]                   ; = code-301 (o code-1)
	//     r11 = r11*2 + r9
	//     r11 = *(u8*)(tabla + r11)         ; métrica del glifo
	//     f0  = (float)r11
}
```

## 4. Confianza y puntos abiertos

| Afirmación | Confianza |
|---|---|
| `sub_826D52C0` es el parser del directorio `id → (offset,w,h)` | **Alta** (bucle 16 B, `[base+0x20]`/`[base+0x24]`, `+8/+12` como f32) |
| Pool = byte-stream con códigos de 15 bits (`(b1<<7)|(b0&0x7F)`) | **Alta** (idéntico en `sub_826D30B8` y `sub_826D43F8`) |
| `0x4000..0x4019` = códigos de control (26) | **Alta** (tabla de salto de 26 entradas) |
| `índice de glifo = código - 301` (códigos ≥ 300) | **Alta** (validado renderizando el atlas: texto legible) |
| `.msg` = metadatos de fuente **+** directorio de mensajes | **Alta** |
| Objeto +96 = recurso de fuente "pequeña" (códigos < 300) | Media |
| Semántica detallada de cada control `0x4000..0x4019` | Media (casos 3=copia RGB, 10=2 parámetros, 25=callback, …) |
| Por qué algunos códigos del `005` llegan a `0x1FA6` (> `count`) | Baja — probablemente sub-fuentes/valores esporádicos; requiere revisión |

## 5. Implicaciones para la traducción (F5/F6)

* El **punto de hook** ya identificado (`sub_826D1F20`) es el setter de la
  **clave de fuente**, no del texto. El hook correcto para sustituir texto es:
  * `sub_826D52C0` (interceptar `id` en `obj+136` y el puntero resuelto), o
  * `sub_826D43F8`/`sub_826D30B8` (interceptar el puntero `obj+120`).
* Para **escribir castellano** hay que:
  1. usar códigos de carácter existentes (`301..857`) para letras ya presentes;
  2. para `á é í ó ú ü ñ ¿ ¡`, ampliar el atlas `AIF` y la tabla de métricas, o
     reutilizar glifos existentes;
  3. generar el byte-stream con la codificación del §1.2 y colocar el nuevo
     offset + ancho/alto en un registro del directorio (o usar un id sintético).
* El **directorio del `.msg`** (id→offset) es el sitio natural para la tabla
  `es.json` (id → nuevos códigos).

## 6. Comandos de reproducción

```bash
python (legacy exploration tool, removed) func 0x826D52C0   # parser del directorio
python (legacy exploration tool, removed) func 0x826D43F8   # decodificador/render
python (legacy exploration tool, removed) func 0x826D3268   # mapa código->glifo
python (legacy exploration tool, removed) extract/f0/d1_ud1_000A4000_005.msg
```

### Validación visual

Los recortes que validan el mapeo están en
`extract/f2/val_A1_cols16.png`, `val_A2_cols16.png`, `val_A3_cols16.png`
(atlas `040`). El script de render es trivial: celda `code-301`, rejilla
`atlas_w/cell_w` columnas, row-major.

---

## 7. VALIDACIÓN EN VIVO (texto real decodificado)

Aplicando el decodificador a los bancos de `ud2`, el texto sale **legible**
(mezcla EN/JP: el disco es **bilingüe**; los mensajes alternan inglés y japonés):

```
id=2  "Great song. What's it called?"
id=3  これ 僕がつくったんだ曲名は…
id=4  "I wrote it myself. It's entitled…"
id=5  聞かなくちゃよかったね…
d1_ud1_39513800_022 (654 msgs, atlas 896x768)
```

- **71.386 mensajes** volcados a `extract/messages/all_codes.json`.
- El atlas de cada `RMD-` es la **tabla de glifos** ordenada por **primera
  aparición** en el texto (por eso el atlas "se lee" como el texto, deduplicado).
- La fuente principal de glifos latinos es `d1_ud1_000A4000_005` (atlas 768²,
  códigos 301–466); los bancos de mensajes usan su propio atlas mixto.

### Pendiente para traducir
1. Construir el **charmap `código↔carácter`** de cada banco (leyendo/OCR del atlas
   o alineando el texto renderizado con los códigos).
2. Pipeline: decodificar → traducir → re-codificar (reutilizando glifos o
   ampliando `AIF`+métricas para `á é í ó ú ü ñ ¿ ¡`).
3. Hook de runtime: `sub_826D52C0` (id→texto) o `sub_826D43F8` (texto resuelto).

---

## 8. Codec validado (decode + encode)

`tools/game_text_codec.py` implementa `parse_msg`, `read_raw`, `decode` y `encode`.

**Validación round-trip:** para los **71.386 mensajes del censo completo
(81 bancos = Disc 1 + Disc 2; ver `docs/re/disc2-messages.md`)**,
`encode(decode(raw)) == raw + b"\x00"` → **71.386/71.386 exactos, 0 fallos**.
Es decir, dominamos la codificación byte a byte del texto del juego.

> **Corrección:** el volcado original se atribuyó solo al Disco 1, pero
> `extract/f0` ya se generó con ambos discos. El censo real es Disco 1 = 36
> bancos (36 379 registros) y Disco 2 = 45 bancos (35 007 registros); tras
> deduplicar `ud2.bin` (idéntico entre discos) quedan **48 bancos únicos /
> 36 473 registros**.

## 9. El texto VARÍA POR CONTEXTO

Cada `RMD-` es un **banco/contexto** con **fuente y charmap propios** (no hay un
charmap global). Ejemplos confirmados:

| Banco | Atlas | Contexto observado |
|---|---|---|
| `d1_ud1_000A4000_005` | 768×768 | atlas **latino principal** (su "texto" es la cobertura de glifos) |
| `d1_ud1_39513800_022` | 896×768 | **diálogo** (mixto EN/JP; p.ej. "Great song. What's it called?") |
| `d1_ud2_…_040` | 512×320 | mensajes **japoneses** (kana) |
| otros `ud2` | varios | menús, objetos, tutorial, sistema, créditos… |

El **charmap debe construirse POR BANCO** (`translation/charmaps/<stem>.json`) y el
texto decodificado se etiqueta por banco → contexto.

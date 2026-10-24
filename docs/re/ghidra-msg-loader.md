# Cargador de mensajes `RMD-`/`.msg` y cmap código→glifo (Ghidra)

> **Estado:** **RESUELTO en Ghidra.** Localizadas y descompiladas de forma
> independiente la función que **parsea el directorio del `.msg`**, la que
> **mapea código→glifo (cmap)** y la que **decodifica/renderiza** el *pool*.
> Método: proyecto Ghidra `IU_FLAT`, programa `default_flat64.exe`, lenguaje
> `PowerPC:BE:64:64-32addr` (ver §1). Contrastado con el C++ de ReXGlue
> (`extract/f2/recomp/generated/default/`) y con un `.msg` real.
>
> Complementa a `docs/re/message-subsystem.md`, `docs/re/rmd-format.md` y
> `docs/re/font-loader.md`. Aquí todo está **verificado con la decompilación de
> Ghidra** (no sólo con el código generado).

## 0. Resumen ejecutivo

| Objetivo | Función (Ghidra) | Función (ReXGlue) | Dirección | Partición | Rol | Confianza |
|---|---|---|---|---|---|---|
| **Parser del `.msg`** | `FUN_826d52c0` | `sub_826D52C0` | `0x826D52C0` | 112 | busca `id` en el directorio de registros de 16 B y deja en el objeto el puntero `pool+offset` + ancho/alto | **Alta** |
| **cmap código→glifo** | `FUN_826d3268` | `sub_826D3268` | `0x826D3268` | 72 | `índice = código-301` (o `-1`), lee la **métrica** del glifo y rellena el vértice | **Alta** |
| **Decodificador/render** | `FUN_826d43f8` | `sub_826D43F8` | `0x826D43F8` | 140 | recorre el byte-stream del pool, procesa controles `0x4000..0x4019` y emite glifos con el cmap | **Alta** |
| Contador de avance | `FUN_826d30b8` | `sub_826D30B8` | `0x826D30B8` | 120 | recorre el mismo stream y **cuenta** glifos/líneas sin dibujar | **Alta** |
| Fetch de recurso (por clave) | `FUN_826d2f88` | `sub_826D2F88` | `0x826D2F88` | 21 | `base = fontMgr[(key&0xFFFF)+1]` (stride 28) | **Alta** |
| Clave → base del `.msg` | `FUN_826d3e20` | `sub_826D3E20` | `0x826D3E20` | 26 | `obj+100 = FUN_826d2f88(fontMgr, obj+132)` | **Alta** |
| Orquestador (slot 51 vtable) | `FUN_826e73e8` | `sub_826E73E8` | `0x826E73E8` | 64 | llama a `826d3e20` → `826d49b0` → **`826d52c0`** | **Alta** |
| Cargador de chunks contenedor | `FUN_8221d768` / `FUN_822226d8` | `sub_8221D768` / `sub_822226D8` | `0x8221D768` / `0x822226D8` | 14 / 74 | recorren chunks; tag `AIF ` (`0x41494620`) → cargador de atlas | **Alta** |
| Cargador del chunk `AIF ` | `FUN_821ef678` / `FUN_821ef618` | `sub_821EF678` / `sub_821EF618` | `0x821EF678` / `0x821EF618` | 81 / 45 | registran el atlas | **Media-Alta** |
| Registro de objetos | `FUN_82147140` | `sub_82147140` | `0x82147140` | 121 | tabla hash `clave→objeto` (NO es el loader) | **Alta** |

Flujo completo:

```
CMessageTextWnd (slot 51 de vtable 0x8206AF4C)
  FUN_826e73e8  ──► FUN_826d3e20 ──► FUN_826d2f88   (clave de fuente → base .msg)
                └► FUN_826d49b0                     (recurso secundario)
                └► FUN_826d52c0  ◄── PARSER del .msg (id → pool+offset, ancho, alto)
                       └ usa [base+0x20] = records_off, [base+0x24] = pool_off
  FUN_826e73e8 ──► FUN_826d43f8  ◄── DECODIFICA y dibuja el byte-stream
                       ├ FUN_826d3268  ◄── CMAP código→glifo (índice + métrica)
                       └ (FUN_826d30b8 = variante que sólo cuenta)
```

## 1. Nota metodológica clave: el programa de 32 bits trunca

El programa original del proyecto, `default_flat.exe`, está importado como
`PowerPC:BE:32:default`. **El código del juego es Xenon/PPC64** y usa
instrucciones de 64 bits `std`/`ld` (p. ej. `std r31,-0x10(r1)` en el prólogo de
`0x826D52C0`). El SLEIGH de 32 bits **no decodifica** esas instrucciones:

```
826d52c0: mfspr r12,LR
826d52c4: stw   r12,-0x8(r1)
826d52c8: UNDEF                     <-- std r31,-0x10(r1)  (no decodificada)
826d52cc: stfd  f30,-0x20(r1)       <-- el desensamblador "salta" y sigue
```

Por eso las funciones con prólogo estándar sólo tenían 1–2 instrucciones
(`body=[[826d52c0,826d52c7]]`) y la decompilación abortaba con
`Unable to resolve constructor` / `halt_baddata()`. **Este es el verdadero
motivo del "truncado", no los helpers `__savegprlr_*`.**

**Solución aplicada:** importar una copia del PE plano con el lenguaje de 64 bits
con direcciones de 32 bits (`64-32addr`), que sí decodifica `std`/`ld`, y sembrar
las funciones de interés. Las direcciones/VAs **no cambian** (mismo `ImageBase`
`0x82000000`, mapeo plano).

```bash
GH=extract/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless.bat

# 1) copia del PE plano
cp extract/f2/default_flat.exe extract/f2/default_flat64.exe

# 2) importar con PPC64 / 32-bit addresses
"$GH" extract/tools/ghidra_proj IU_FLAT \
  -import extract/f2/default_flat64.exe \
  -processor PowerPC:BE:64:64-32addr -noanalysis -scriptPath (legacy Ghidra scripts, removed)

# 3) sembrar las funciones de interés (lista de direcciones ReXGlue)
"$GH" extract/tools/ghidra_proj IU_FLAT -process default_flat64.exe -noanalysis \
  -postScript SeedFunctions.java "$(pwd)/extract/f2/msg_starts2.txt" \
  -scriptPath (legacy Ghidra scripts, removed)

# 4) descompilar
"$GH" extract/tools/ghidra_proj IU_FLAT -process default_flat64.exe -noanalysis \
  -postScript DecompileAddr.java 0x826D52C0 0x826D3268 0x826D43F8 0x826D30B8 \
  -scriptPath (legacy Ghidra scripts, removed)
```

> El proyecto contiene ahora **dos programas**: `default_flat.exe` (32 bits,
> 24931 funciones) y `default_flat64.exe` (64 bits, funciones sembradas bajo
> demanda). Este último es el que permite la decompilación completa.
> Scripts añadidos en `(legacy Ghidra scripts, removed)/`: `DisasmRange.java`,
> `ProbeRange.java`, `FixDisasm.java`, `Info.java`.

## 2. Parser del `.msg` — `FUN_826d52c0` (`sub_826D52C0`)

Firma real de Ghidra:

```c
undefined8 FUN_826d52c0(int param_1)   // param_1 = CMessageTextWnd* (this)
```

Cuerpo (Ghidra, anotado; `param_1` = objeto de texto):

```c
iVar5 = *(int *)(param_1 + 100);            // +0x64: base del .msg  (recurso "grande")
if (iVar5 == 0) return 0;                   // sin recurso
if (*(int *)(param_1 + 0x74) == 0) *(int *)(param_1 + 0x74) = iVar5;   // +116 cursor
if (*(int *)(param_1 + 0x70) == 0) *(int*)(param_1 + 0x70) = *(int*)(param_1+0x60); // +112 = +96 (fuente pequeña)

iVar6 = *(int *)(*(int *)(param_1 + 0x74) + 0x24) + iVar5;   // iVar6 = base + [base+0x24] = POOL
for (piVar7 = (int *)(*(int *)(*(int *)(param_1+0x74) + 0x20) + iVar5);   // [base+0x20] = RECORDS
     ((*piVar7 != 0 || (piVar7[1] != 0)) && (*piVar7 != *(int *)(param_1 + 0x88)));  // +0x88 = id pedido
     piVar7 = piVar7 + 4) { }               // registros de 16 B

if ((*piVar7 == 0 && piVar7[1] == 0) || (*(int *)(param_1 + 0x88) == 0)) {
    // no encontrado -> pide "message not found", reintenta fetch y vuelve a buscar
    func_0x821a2f10(param_1, 4, 0xf4241);   // sub_821A2F10: marca error (0xF4241 = 1000001)
    FUN_826d3e20(param_1);                  // re-fetch base por clave de fuente
    FUN_826d49b0(param_1);                  // re-fetch recurso secundario
    iVar5 = *(int *)(param_1 + 100);
    /* ... repite la búsqueda ... */
    if ((*piVar7 == 0) && (piVar7[1] == 0)) return 0;
}

*(int *)(param_1 + 0x78)  = piVar7[1] + iVar6;   // +120 = TEXTO = POOL + record.offset   (B)
*(int *)(param_1 + 0xc4)  = piVar7[2];           // +196 = record.C  (f32 = ancho)
*(int *)(param_1 + 200)   = piVar7[3];           // +200 = record.D  (f32 = alto)
/* ... escala con [base+0x44]/[base+0x48] (cell_w/cell_h) y [base+0x44] ... */
return 1;
```

### 2.1 Cabecera del `.msg` que consume el parser

| Desplazamiento | Campo | Confirmado con `.msg` real |
|---|---|---|
| `+0x20` (32) | `records_off` — inicio del directorio de 16 B | `0x80` |
| `+0x24` (36) | `pool_off` — inicio del pool de códigos | `0x41100` |
| `+0x28` (40) | `metrics_off` — tabla de métricas (usada por el cmap) | `0x171D80` |
| `+0x44` (68) | `cell_w` | `32` |
| `+0x48` (72) | `cell_h` | `32` |

Registros de 16 B: `(A=id, B=offset_en_pool, C=f32 ancho, D=f32 alto)`. El
terminador es `A==0 && B==0`.

### 2.2 Validación con un `.msg` real (`d1_ud1_000A4000_005.msg`, 1 516 032 B)

```
records_off = 0x80      pool_off = 0x41100   metrics_off = 0x171D80
count = 557             cell = 32x32         atlas = 768x768
registros: id=1 off=0 w=20 h=26 ; id=2 off=3 w=20 h=26 ;
           id=3 off=6 w=164 h=26 ; id=4 off=35 ...
```

Decodificando el pool en `off` con la regla del §4: `id=1 → [0x12D]`,
`id=2 → [0x12E]`, `id=3 → [0x12F,0x130,…]`. Es decir, el directorio mapea
`id → secuencia de códigos` dentro del pool; los códigos `301..` son los glifos.
(Qué *representa* cada `id` —mensaje o entrada de fuente— sigue siendo la
pregunta abierta de `rmd-format.md`; el **mecanismo** queda fijado aquí.)

## 3. cmap código→glifo — `FUN_826d3268` (`sub_826D3268`)

Firma real de Ghidra:

```c
void FUN_826d3268(int param_1, uint codigo, undefined2 codigo2,
                  undefined8 param_4, undefined1 flag);
```

Cuerpo (Ghidra, anotado). `param_1+0xFC` = puntero al vértice/glifo actual
(registro de 0x30 B); `param_1+0x60` = recurso de fuente "pequeña";
`param_1+100`(`0x64`) = base del `.msg`; `param_1+0x74`(`116`) = base vigente:

```c
int g = *(int *)(param_1 + 0xfc);                 // glifo actual (0x30 B)
*(short *)(g + 0x2a) = (short)codigo - 1;         // índice provisional = código-1

iVar8 = *(int *)(param_1 + 100);                  // tabla por defecto = base .msg
iVar6 = *(int *)(param_1 + 0x74);
*(u8 *)(g + 0x2e) = flag;

if (codigo < 300) {
    iVar8 = *(int *)(param_1 + 0x60);             // tabla = fuente pequeña
    uVar7 = 1;  iVar6 = iVar8;
} else {
    if (codigo == 0x4010 || codigo == 0x4019) {   // dos controles que emiten glifo
        *(u16 *)(g + 0x2a) = 300;
        *(u16 *)(g + 0x2a) = *(u16 *)(g + 0x2a) - 300;   // -> índice 0
        *(u16 *)(g + 0x2c) = codigo2;             // guarda el código secundario
    } else {
        *(u16 *)(g + 0x2a) = *(u16 *)(g + 0x2a) - 300;   // código-301
    }
    uVar7 = 4;
}
*(int *)(g + 0x10) = iVar8;                       // g+16 = tabla de fuente
*(u16 *)(g + 0x28) = uVar7;                       // g+40 = modo (1 o 4)
*(u32 *)(g + 0x24) = codigo & 0xffff;             // g+36 = código

/* MÉTRICA: byte en  tabla + [tabla_de_base+0x28] + 2*índice */
if (*(int *)(param_1 + 0xec) == 0) {
    fVar3 = (float)*(byte *)( (u16)*(g+0x2a) * 2 + *(int *)(iVar6 + 0x28) + iVar8 );
} else { /* ramas con constantes 0x10/0x14/0xffff (fuentes especiales) */ }

/* posicionamiento: x/y, avance, color RGBA (param_1+0x30..0x32), escala; */
**(u32 **)(param_1 + 0xfc)      = *(u32 *)(param_1 + 0x104);
*(float *)(g + 8)               = *(float *)(param_1 + 0x104) + fVar3;   // avance X
*(float *)(g + 0xc)             = *(float *)(param_1 + 0x108) + fVar4;
*(int *)(param_1 + 0xfc)       += 0x30;            // siguiente glifo
*(int *)(param_1 + 0xf8)       += 1;               // nº de glifos emitidos
```

### 3.1 Regla del cmap (lo esencial)

```
índice_de_glifo = (código < 300) ? código - 1 : código - 301
métrica         = *(u8 *)( base_fuente + [base_fuente+0x28] + 2*índice )
                  // [base+0x28] = metrics_off del .msg
```

* Fuente **pequeña** (`+96`/`0x60`) para `código < 300` → índice `código-1`.
* Fuente **grande** = base del `.msg` (`+100`/`0x64`) para `código ≥ 300` →
  índice `código-301`.
* La métrica es **1 byte** leído con **stride 2** (`2*índice`) en
  `metrics_off`: la instrucción es `lbzx`, es decir, se lee **el primer byte**
  de cada entrada de 2 B de la tabla del `.msg`.
* `0x4010` y `0x4019` son **códigos de control que también emiten glifo**
  (se mapean al índice 0 y guardan el código secundario).
* El registro de glifo emitido mide **0x30 B** y el objeto avanza su cursor
  `+0xFC` en 0x30 y `+0xF8` en 1.

Esto **coincide exactamente** con `font-loader.md` (`índice = código - 301`,
`-1` para la fuente pequeña) y con `cell_w/cell_h = 32`.

## 4. Decodificador del byte-stream — `FUN_826d43f8` y `FUN_826d30b8`

`FUN_826d43f8` (`sub_826D43F8`) lee el texto en `piVar12[0x1e]`
(= `+0x78` = `120`, el puntero que deja el parser §2) y decodifica:

```c
while (1) {
    b0 = *p++;                                  // primer byte
    if (b0 == 0) { fin de mensaje; }
    if ((b0 & 0x80) == 0) { code = b0; /* 1 byte */ }
    else {
        b1 = *p++;
        code = (b1 << 7) | (b0 & 0x7f);         // código de 15 bits
        if ((code & 0x4000) == 0) { /* carácter normal */ }
        else switch (code) { case 0x4000..0x4019: ... }   // controles
    }
    // carácter normal:
    FUN_826d3268(obj, code, ..., 1);            // <-- CMAP
}
```

Controles manejados por el `switch` (evidencia del decompilado): `0x4000` (fin),
`0x4001` (color/ajuste RGB con conversión de bytes), `0x4002` (restaurar color),
`0x4003` (copia 4 bytes a `pfVar21/pfVar17`), `0x4004` (reset), `0x4007/0x4008/
0x400c/0x400e` (avanzan 6 B), `0x4009/0x4018` (avanzan 3 B), `0x400a` (2 bytes de
color), `0x4010` (sub-cadena terminada en 0), `0x4011` (reposiciona un rect),
`0x4019` (set de estado). Los `case` sin cuerpo explícito caen a `default`.

`FUN_826d30b8` (`sub_826D30B8`) es la variante **contador** (mide longitud/saltos
de línea) con el **mismo** esquema de decodificación; sólo devuelve 1 al llegar a
`0`:

```c
pbVar4 = *(byte **)(param_1 + 0x78);
if (!pbVar4) return 0;
do { b0=*p++; if(!b0) return 1;
     if (b0 & 0x80) { b1=*p++; p++; if ((b1<<7)&0x4000) switch(...) }
} while(1);
```

## 5. Carga del recurso (`RMD-` → chunk `AIF `)

El `.msg` va precedido por el atlas `AIF `. El **contenedor** se recorre en
`FUN_822226d8` / `FUN_8221d768`:

```c
// FUN_822226d8: cabecera [+0x10] == 0x10140; recorre chunks desde +0x20
while (true) {
    tag = *p;                                   // u32 BE del chunk
    if (tag == 0x656f665f) break;               // "eof_"  -> fin
    if (tag == 0x41494620) {                    // "AIF "  (A I F space)
        cVar = FUN_821ef618(base, p, param_3);  // cargar atlas
        if (!cVar) { FUN_8221d768(...); }
    } else if (tag == 0x616f5f5f) {             // "ao__" -> otro tipo de chunk
        ...
    }
    p += p[3];                                  // tamaño del chunk
}
```

`FUN_821ef678`/`FUN_821ef618` registran el chunk `AIF ` en el gestor de
recursos (`func_0x821d1cd0`). La cabecera `MessageConvertLib_1.0.0.0` **no se
compara por puntero** en el código (son datos); el parser recibe la base ya
resuelta por clave de fuente.

## 6. Orquestador y registro de objetos

`FUN_826e73e8` = **slot 51 de la vtable `CMessageTextWnd` (`0x8206AF4C`)**,
confirmado leyendo la vtable del PE:

```
vtable 0x8206AF4C:  slot 51 = 0x826E73E8
```

Hace: `FUN_826d3e20` (clave→base) → `FUN_826d49b0` (recurso secundario) →
**`FUN_826d52c0` (parser)** → `func_0x826d3ae0`, y otras actualizaciones; si la
clave de fuente cambia, vuelve a pedir `FUN_82147140` (registry).

`FUN_82147140` (`sub_82147140`) es el **registro de objetos** (hash por clave, NO
el loader):

```c
int FUN_82147140(int tabla, uint clave, ulonglong flag) {
    int o = *(int *)(((clave & 0xffff) + 0x2d) * 4 + tabla);
    if (o && (((*(uint *)(o+0x10) ^ clave) & 0xffff0000) == 0)
          && ((1L << (flag & 0x7f)) & *(ulonglong *)(o+0x18))) return o;
    return 0;
}
```

`FUN_826d1f20` (usado por `SetText`) trabaja con `[obj+0x28]` (clave de fuente) y
`[obj+0x10]` (descriptor), llamando a `826d7d48`/`826d7ce8`.

## 7. Confianza y matices

| Afirmación | Confianza | Evidencia |
|---|---|---|
| `FUN_826d52c0` parsea el directorio del `.msg` (`id→pool+offset,w,h`) | **Alta** | decompilado Ghidra + `.msg` real + ReXGlue |
| Cabecera `+0x20`=records, `+0x24`=pool, `+0x28`=metrics, `+0x44/+0x48`=celda | **Alta** | Ghidra + volcado del fichero |
| `FUN_826d3268` es el cmap (`índice = código-301` ó `-1`) | **Alta** | decompilado Ghidra + `count`/rango de códigos |
| La métrica es **1 byte con stride 2** en `metrics_off` | **Alta** | `lbzx` en `FUN_826d3268` (Ghidra y ReXGlue) |
| `FUN_826d43f8` decodifica `(b1<<7)|(b0&0x7f)` y saltos de control `0x4000..0x4019` | **Alta** | decompilado Ghidra (switch) |
| `FUN_826e73e8` = slot 51 de vtable `CMessageTextWnd` | **Alta** | lectura de la vtable en el PE |
| `FUN_822226d8`/`FUN_8221d768` recorren chunks y detectan `AIF `/`eof_` | **Alta** | decompilado Ghidra (tags inmediatos) |
| Qué representa cada `id` del directorio (¿mensaje o entrada de fuente?) | **Media** | abierto en `rmd-format.md`; el mecanismo sí está fijado |
| Por qué el render directo da secuencias ilegibles | **Baja** | requiere traza; ver `rmd-format.md §11–12` |

**Conclusión:** el **cargador/parser** (`FUN_826d52c0`, con `FUN_826d2f88` y
`FUN_826d3e20`) y el **cmap** (`FUN_826d3268`) quedan identificados y
descompilados en Ghidra, coherentes con el C++ de ReXGlue y con los `.msg`
reales. El único punto que sigue abierto no es el mecanismo (que está fijado)
sino **la semántica de los `id`** y por qué el contenido del pool no se lee como
frases con el mapeo directo.

## 8. Comandos de reproducción

```bash
GH=extract/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless.bat

# decompilar parser + cmap + decoder (programa de 64 bits)
"$GH" extract/tools/ghidra_proj IU_FLAT -process default_flat64.exe -noanalysis \
  -postScript DecompileAddr.java 0x826D52C0 0x826D3268 0x826D43F8 0x826D30B8 \
  -scriptPath (legacy Ghidra scripts, removed)

# volcado de cabecera + registros de un .msg real
python (legacy exploration tool, removed) extract/f0/d1_ud1_000A4000_005.msg --codes 32

# contraste con el C++ generado
python (legacy exploration tool, removed) func 0x826D52C0    # parser
python (legacy exploration tool, removed) func 0x826D3268    # cmap
python (legacy exploration tool, removed) func 0x826D43F8    # decoder
```

Ficheros de evidencia generados: `extract/f2/ghidra_dec64.log`,
`extract/f2/ghidra_dec64b.log`, `extract/f2/ghidra_import64.log`.

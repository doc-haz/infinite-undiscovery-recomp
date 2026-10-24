# Subsistema de mensajes en `default.xex` (F2 — en curso)

> **Estado:** `default.xex` extraído/desencriptado y **regenerado a C++** con el
> toolchain del recomp (ReXGlue v0.10.0). Anclajes y clases del subsistema de UI
> localizados. **La función concreta de resolución de mensaje aún no está
> fijada** (ver §6); hace falta análisis interactivo (Ghidra) o traza dinámica.

## 1. Ejecutable

```bash
python tools/vendor/pc-infiniteundiscovery/tools/xdvdfs.py extract \
       "roms/Infinite Undiscovery (Europe) (Disc 1).iso" extract/f2 --only default.xex
python tools/vendor/pc-infiniteundiscovery/tools/xex.py extract \
       extract/f2/default.xex extract/f2/default.exe
```

| Dato | Valor |
|---|---|
| Región | PAL (Europe) |
| SHA-256 `default.xex` | `22893bb8d96a1440ecbdbcae543baeaf89d26588c89c99a2c96fecf611475325` (EUROPE) |
| Base de carga | `0x82000000` |
| Mapeo | **plano**: `VA = 0x82000000 + file_offset` |
| Entrada | `0x821CBA90` |

## 2. Recompilación a C++ (opción elegida)

El repo `infinite-undiscovery-recomp` **no** es el recompilador: es el proyecto
anfitrión. El traductor PPC→C++ es el CLI **`rexglue`** del SDK
[`rexglue/rexglue-sdk`](https://github.com/rexglue/rexglue-sdk) (tag `v0.10.0`,
commit `f5337cdc…`). El recomp espera el CLI en `tools/rexglue/` y el xex en
`assets/`.

```bash
# CLI oficial v0.10.0 (solo se usa para codegen; los hashes de DLL solo
# afectan al build del runtime, no a la generación de código)
curl -L -o rexglue-sdk.zip \
  https://github.com/rexglue/rexglue-sdk/releases/download/v0.10.0/rexglue-sdk-0.10.0-win-amd64.zip

# Preparar el proyecto
cp default.xex <recomp>/assets/default.xex          # SHA = PAL
cp <sdk>/bin/* <recomp>/tools/rexglue/
cd <recomp> && ./tools/rexglue/rexglue.exe --force codegen --ignore-stamp
```

**Resultado:** `generated/default/` con **321 ficheros C++**
(`infinite_undiscovery_recomp.0..156.cpp`, `…_funcs.*.h`, `…_init.cpp`) y
`codegen.partition.json` (mapa `dirección → partición`). Codegen: **10,2 s**.

> El C++ generado es una traducción fiel por función, con comentarios del
> ensamblador original (`// lis …`, `// bl 0x…`). Es "código fuente" del juego
> para analizar. `generated/` y `tools/` están en el `.gitignore` del recomp
> (derivan del `default.xex`).

## 3. Herramienta de navegación

`(legacy exploration tool, removed)` cruza direcciones del `.exe` con el C++ generado:

```bash
python (legacy exploration tool, removed) where 0x824D31FC     # -> función + partición
python (legacy exploration tool, removed) func  0x8252FA40     # -> cuerpo de la función
python (legacy exploration tool, removed) grep  "0x826d5e98"   # -> busca en el código generado
```

## 4. Anclajes y clases

Cadenas/RTTI:

| VA (real) | Qué |
|---|---|
| `0x82A3D49C`, `0x82A3D8DC`, `0x82A3DE64`, `0x82A3E194` | `MessageConvertLib_1.0.0.0` (4 copias, junto a `MessageConvertLib.dll` y nombres `FOT-…` de fuentes: **descriptores de fuente embebidos**) |
| `0x82A3DF14` | RTTI `.?AVCMessageTextWnd@@` |
| `0x82A0E54C` | RTTI `.?AVCMessageWnd@@` |
| `0x82A38FC0` | RTTI `.?AVCWindowMessageTask@@` |

> **Aviso de direcciones:** `tools/disasm.py` mapea el fichero de forma **plana**
> (`VA = 0x82000000 + file_offset`), pero la PE **no** es plana en todas las
> secciones (`.data`: `rawptr 0x9BB000` vs `vaddr 0x9C0000`). En `.rdata` coincide
> (por eso la vtable `0x8206AF4C` es correcta), pero en `.data` las direcciones de
> `disasm.py` van desviadas ~`0x5000`. Las de arriba son las **correctas**
> (`RVA = file - rawptr + vaddr`). Ghidra las calcula bien.

Del RTTI se extrajeron **127 clases** del subsistema de UI/mensajes:
`CMessageTextWnd`, `CMessageWnd`, `CWindowMessageTask`, `CWindowImageTask`,
`CResourceData`, `CWndObject`, `CWndTextureManager`, y familias `CCamp*Wnd`,
`CShop*Wnd`, `CInfoWnd`, `CGameOverWnd`, `COpeningWnd`, `CReportWnd`,
`CConfirmDialogWnd`, `CSelectWnd`, `CSelectorWnd`, etc.

**Vtable de `CMessageTextWnd`** (vía localizador RTTI en `0x8211A040`):
- primaria `0x8206AF4C` (14 slots), secundarias `0x8206AF40` y `0x8206AF38`
  (herencia múltiple);
- constructor `sub_826E6BF0`, destructor `sub_826E6D80`.

## 5. Hallazgo clave: el texto va codificado

Un escaneo de **los dos contenedores del Disco 1** (≈5 GB) buscando ASCII/UTF-16
legible no encontró **ni una frase de diálogo** (solo nombres de shaders y
binario). Junto con que los 81 `.msg` de los `RMD-` tampoco contienen texto
plano, esto **confirma** que el texto del juego va **codificado** (códigos de
glifo), como anticipaba el plan (riesgo R1).

## 6. Lo que falta y por qué

La función exacta que resuelve `id → secuencia de códigos` **no está fijada**.
Intentos y resultado:

| Método | Resultado |
|---|---|
| `disasm.py xref` de las cadenas | Ruidoso (escaneo lineal; falsos positivos por registros obsoletos) |
| Buscar referencias a la dirección de la cadena en el C++ generado | Sin coincidencias (las cadenas son datos; el comparador usa inmediatos) |
| Heurístico "función que lee `+0x24/+0x28/+0x34/+0x58/+0x5C`" | Falsos positivos (arrays/clamps genéricos) |
| Grep de tags `RMD-`/`MRON` como enteros | Sin coincidencias (comparación byte a byte) |

**Siguientes pasos recomendados** (por orden de eficacia):

1. **Ghidra** (Java 17 ya presente) cargando `default.exe` como `PowerPC:BE:32`
   en base `0x82000000`: navegar desde la vtable/RTTI de `CMessageTextWnd` y
   `CResourceData`, y desde el cargador de recursos, hasta el parser del `RMD-`.
2. **Traza dinámica** con el runtime del recomp (F4/F5): hook de logging en la
   carga de `RMD-` y en el render de texto, para capturar el `id` y los códigos
   en vivo.
3. **Mapeo código→glifo por render**: dado que `count`≈nº de glifos y el atlas
   tiene celdas de 32×32, renderizar una hipótesis `celda = f(código)` sobre un
   atlas y comparar con una cadena de UI conocida.

## 7. Ghidra (decompilación interactiva) y el **layout plano**

### 7.1 Hallazgo clave: el juego usa direcciones **planas**

Las direcciones del código generado por ReXGlue (y las que usa el runtime del
recomp) son **`0x82000000 + offset_de_fichero`**, NO `ImageBase + RVA` de la PE.
Comprobación: los bytes de `sub_82140000` están en el **offset de fichero**
`0x140000`, no en la RVA `0x141400`.

La PE extraída por `xex.py` tiene las secciones desplazadas (`SectionAlignment`
0x10000): `.text` `vaddr=0x140000` vs `rawptr=0x13EC00` (delta `0x1400`);
`.data` delta `0x5000`. Por eso `disasm.py` (plano) y ReXGlue coinciden entre sí,
pero **Ghidra**, si carga la PE tal cual, queda desalineado (`+0x1400` en código,
`+0x5000` en datos) y no resuelve referencias.

**Solución aplicada:** parchear una copia de la PE para que
`VirtualAddress == PointerToRawData` en todas las secciones
(`extract/f2/default_flat.exe`) e importarla. Así Ghidra queda **alineada** con
ReXGlue/runtime: `FUN_824d2e58` aparece exactamente en `0x824d2e58`.

```
# conversión flat -> PE (por si se usa la PE sin parchear)
PE_VA = 0x82000000 + (flat_off - rawptr + vaddr)   # +0x1400 en .text, +0x5000 en .data
```

### 7.2 Proyectos y scripts

- `extract/tools/ghidra_proj/IU_FLAT` — proyecto **alineado** (usar este).
- `extract/tools/ghidra_proj/IU_F2` — proyecto con la PE sin parchear (desalineado).
- Scripts en `(legacy Ghidra scripts, removed)/` (Java, porque Ghidra 12 usa PyGhidra y no hay
  Python configurado): `DumpMessageSubsystem.java` (cadenas + refs + descompilado),
  `FindConst.java`, `DecompileAddr.java`, `DumpVtable.java`, `Probe.java`.

```bash
GH=extract/tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless.bat
"$GH" extract/tools/ghidra_proj IU_FLAT -process default_flat.exe -noanalysis \
  -postScript DecompileAddr.java 0x824D2E58 -scriptPath (legacy Ghidra scripts, removed)
```

### 7.3 Lo que revela Ghidra

- Las cadenas mágicas (`MessageConvertLib_1.0.0.0`, `MessageConvertLib.dll`) y los
  nombres RTTI (`CMessageTextWnd`, …) **no tienen referencias de código** (son
  datos; el comparador no las usa por puntero).
- Las constantes `"AIF "`, `"RMD-"`, `"MRON"`, `0x0131F508` **no aparecen como
  inmediatos** en el código analizado.
- La **vtable de `CMessageTextWnd`** (`0x8206AF4C`) se resuelve y sus métodos se
  descompilan (son métodos genéricos de ventana), pero el análisis automático de
  Ghidra es **incompleto** (~172 K instrucciones de ~2 M), así que faltan
  funciones por descubrir.

### 7.4 Siguiente paso recomendado (para cerrar F2)

**Sembrar Ghidra con los límites de función de ReXGlue**
(`generated/default/codegen.partition.json`, ~10 000 funciones, autoritativos):
crear/desensamblar cada función y reanalizar. Con todas las funciones presentes,
navegar desde `CMessageTextWnd`/`CResourceData` hasta el parser del `RMD-` deja de
depender de heurísticos. Alternativa complementaria: **traza dinámica** con el
runtime del recomp (F4).

## 8. Hallazgos concretos del código generado (F2)

Siguiendo la cadena de la clase `CMessageTextWnd` en el **C++ de ReXGlue** (que sí
es completo, a diferencia de Ghidra):

| Dirección | Función | Papel |
|---|---|---|
| `0x826E6BF0` | constructor de `CMessageTextWnd` | fija la vtable `0x8206AF4C` |
| `0x826E6D80` | destructor | |
| `0x826AC6D0` | factoría | `operator new(1272)` + constructor |
| `0x826AC998` | **crear ventana de texto** | factoría + `sub_826D1F20` + posición |
| `0x826D1F20` | **`SetText(win, text_ptr)`** | guarda `text_ptr` en `win+40` |
| `0x826D7D48` / `sub_826D7CE8` | gestión de nodos/fuente asociada | (listas de 24 B) |

`sub_826D1F20` es el **punto de entrada del texto a la ventana**: recibe el
puntero a la cadena codificada y lo deja en `CMessageTextWnd+40`. Es un **candidato
directo a hook (H1/H3)**: interceptándolo se puede sustituir el texto por el
castellano. Pero el puntero de texto ya viene **codificado en glifos**, así que
para *autor* la traducción sigue haciendo falta el mapa código→glifo.

**Siguiente paso concreto:** encontrar los llamadores de `sub_826AC998` /
`sub_826D1F20` (quién decide *qué* texto mostrar, p. ej. desde un `id`) y el punto
donde el código se convierte a celda del atlas. Ese es el lugar donde encaja la
tabla `es.json` (id → códigos) y, en su caso, la ampliación del `AIF` (F3).

### 8.1 Rastreo de llamadores (hecho)

- `sub_826D1F20` (`SetText`) tiene **decenas de llamadores**: es la API central de
  texto de la UI. **Interceptarla captura todo el texto de ventanas** (candidato
  sólido a hook).
- Uno de los que crean ventana con texto, `sub_826AD9C8`, toma el texto de
  **`[this+16]`** y lo pasa por `sub_826AC998` → `sub_826D1F20`. Es decir, el
  texto de un documento/ventana vive en `+16` de su objeto.
- **No localizado aún:** dónde se asigna ese `+16` (la carga del mensaje) y, sobre
  todo, la **conversión código→celda del atlas**. Los heurísticos de offsets
  (`+0x24/+0x28/+0x34/+0x58/+0x5C` y `+0x34/+0x60/+0x64`) dan falsos positivos
  (clamps, copias de struct). El formato del `.msg` sigue sin decodificar del todo.

**Conclusión:** el punto de *hook* está identificado (`sub_826D1F20`); falta el
**mapa código→glifo** para poder *escribir* castellano. Las dos vías para obtenerlo:
(a) localizar el parser/cargador de fuente en el C++ generado, o (b) **traza
dinámica** (F4), que es más directa: registrar el `text_ptr` real al pasar por
`SetText` y volcar los códigos en vivo.

### 8.2 Hipótesis de codificación (a validar)

El `.msg` declara `count`≈nº de glifos (557 para el atlas 768×768) y una tabla de
2 B/glifo. Los códigos observados (`0x02AD…0x03AB`) encajan con `código = índice
de glifo + base` (con `base`≈`0x200`), pero el atlas no es una rejilla uniforme
(los glifos van empaquetados), así que la tabla de 2 B probablemente son
**métricas/posición** del glifo, no una celda. Confirmarlo requiere la traza.

## 9. Decisión de hook

- _Pendiente_, condicionada a §6. Candidatas: H1 (resolución `id → códigos`),
  H2 (sustituir el `RMD-` en memoria al cargar), H4 (índices a glifos).

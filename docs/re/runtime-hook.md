# Hook de traducción en runtime (F5)

> **Estado:** implementado y compilado. Sustituye el texto en caliente mediante
> ReXGlue, sin repackear los assets. Código: `mod/recomp/translation_es.h`
> (copiado a `extract/f2/recomp/src/translation_es.h`, incluido desde `src/main.cpp`).
> Prueba en vivo: `extract/f2/run_es_test.sh`.

## 1. Mecanismo

ReXGlue genera cada función con **alias débil**: el cuerpo original queda accesible
como `__imp__sub_<addr>` y una *definición fuerte* del mismo símbolo la reemplaza.
Es el mismo mecanismo que ya usan `iu_trace_points.h` / `translation_trace.h`.

El hook sobreescribe el **parser del directorio de mensajes** `sub_826D52C0`
(`obj+0x88` = id → `obj+0x78` = pool + offset, `obj+0xC4/0xC8` = ancho/alto):

```cpp
extern "C" void __imp__sub_826D52C0(PPCContext& ctx, uint8_t* base);
extern "C" void sub_826D52C0(PPCContext& ctx, uint8_t* base) {
  __imp__sub_826D52C0(ctx, base);          // resolver el original
  iu::translation::LoadCatalog();          // (una vez)
  iu::translation::TrySubstitute(base, ctx.r3.u32);  // reemplazar obj+0x78
}
```

El parser se ejecuta **cada frame** (orquestado por `sub_826E73E8`, slot 51 de
`CMessageTextWnd`), por lo que la sustitución se reaplica sola; además se **cachea**
el buffer por `(huella_banco, id)` para no reasignar memoria.

## 2. Identificación del banco

Cada `RMD-` es un banco con su propia fuente/charmap. En runtime no está disponible
el nombre del fichero, pero sí la **cabecera del `.msg` cargado** en `obj+0x64`:
`(total, count, atlas_w, atlas_h)`. Esa 4-tupla (mezclada con FNV-1a) es la
**huella** del banco. Se valida además el magic `"Mess"` en `msgbase+0`.

> Consecuencia: el catálogo se indexa por huella, así que bancos idénticos entre
> discos (`ud2.bin`, 33 pares) comparten traducción automáticamente.

## 3. Buffer en memoria *guest*

Se reserva con `rex::system::kernel_memory()->SystemHeapAlloc(n)` y se escribe por
la misma vía que usa el lector recompilado (`base + guest + offset`, ver
`REX_PHYS_HOST_OFFSET`). El buffer contiene el **byte-stream codificado**
(`b0<0x80→code=b0`; `b0>=0x80→(b1<<7)|(b0&0x7F)`; terminador `0x00`), no `u16`.

## 4. Catálogo

`translation/es_catalog.bin`, generado por `tools/gen_es_catalog.py` a partir de
`translation/es_codes.json` (salida de `tools/build_es_codes.py`) y de las huellas
leídas de los `.msg`:

```
u32 magic 'IUC1'; u32 n;
n × { u32 total,count,atlas_w,atlas_h,id,ncodes; u16 codes[ncodes] }
```

Ruta configurable con `IU_ES_CATALOG`; por defecto `translation/es_catalog.bin`
relativa al directorio de ejecución.

## 5. Interruptores

| Variable | Efecto |
|---|---|
| `IU_ES=0` | desactiva el hook (passthrough puro) |
| `IU_ES_SELFTEST=reverse` | invierte los tramos de glifos (prueba de inyección) |
| `IU_ES_SELFTEST=<N>` | fija todos los glifos al código `N` |
| `IU_ES_CATALOG=<ruta>` | catálogo alternativo |
| `IU_TRACE=1` | activa el log de traza (`EUROPE/logs/iu_trace.log`) |

El autotest usa **solo códigos originales** (glifos válidos del propio banco) y
conserva los controles de layout `0x4000..0x4019`, de modo que no puede provocar
lecturas fuera de la tabla de métricas.

## 6. Reproducción

> **IMPORTANTE (escollo del build):** el proyecto **no propaga dependencias de
> headers** al depfile de ninja. Editar `translation_es.h`/`translation_trace.h`
> **no recompila** `main.cpp` (ninja solo relinkea) y el binario queda obsoleto.
> Usa siempre `mod/recomp/build.sh` (copia los headers + `touch main.cpp` + compila),
> y **copia el exe** a `run/` antes de ejecutar (lo hace `run_es_test.sh`).

```bash
# 1) compilar (wrapper que fuerza recompilar main.cpp)
bash mod/recomp/build.sh

# 2) prueba en vivo (autotest de inversión, 90 s)
bash extract/f2/run_es_test.sh 90 reverse

# 3) cuando existan traducciones:
python tools/build_es_codes.py           # es.json + charmaps -> es_codes.json
python tools/gen_es_catalog.py           # -> es_catalog.bin
bash extract/f2/run_es_test.sh 90        # ya sin IU_ES_SELFTEST
```

## 7. Validación en vivo (resultado)

Ejecutado el juego con `IU_ES=1 IU_ES_SELFTEST=reverse IU_TRACE=1` (120 s):

```
ES dbg#1 obj=E5C0A860 msgbase=E94DF000 magic=4D657373 txt=E9520D79 id=152
ES: SELFTEST-REV +257 bytes @ 30BC9000 (128 codigos)
ES SUBST fp=DC0D11A6 id=152 n=128
...
== total sustituciones == 41
```

Conclusiones:

* El override **se instala y ejecuta** (log `ES dbg#1…`).
* `obj+0x64` **es** la base del `.msg` (`magic="Mess"`) → el offset es correcto.
* Se **asignan buffers en memoria guest** (`30BC9000…`) vía `SystemHeapAlloc` y se
  escriben; `TranslateVirtual == host_ptr` (sin aviso).
* **41 mensajes** sustituidos; el juego corre **sin fallar**.

> **Bug corregido:** en el override hay que capturar `obj = ctx.r3` **antes** de
> llamar al original; al volver, `r3` contiene el **valor de retorno** (0/1), no el
> puntero al objeto. Sin esa corrección el hook entraba pero siempre salía por el
> guard (`obj < 0x10000`).

## 8. Pendiente

- Ajustar `obj+0xC4/0xC8` (ancho/alto) si el texto ES es más largo (layout/alineado).
- Preservar **saltos de línea** y controles internos del original al traducir.
- Recarga en caliente del catálogo sin reiniciar (objetivo F5/F6).

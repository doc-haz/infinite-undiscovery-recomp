# Acentos castellanos POR BANCO (WS1 — integrado por hook)

> **Estado:** integrado y compilado. Verificación **offline** completa (el juego
> no se ha ejecutado, por orden del usuario). Código: `mod/recomp/accents_es.h`
> (copiado a `extract/f2/recomp/src/accents_es.h` por `mod/recomp/build.sh`,
> incluido desde `translation_es.h`).

## 0. Problema

Los 16 glifos `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡` **no existen** en el atlas de
ningún `RMD-` salvo el banco `005` (que se amplió a mano en F3). En los demás
bancos `tools/build_es_codes.py` los **plegaba a ASCII** (`¿`→`?`, `á`→`a`…)
porque el atlas no los tenía. Síntoma: `?` donde debería ir `¿` y falta de
tildes en el diálogo.

## 1. Material generado por banco

`tools/add_accents_banks.py` (ampliado) recorre los bancos que necesitan
acentos (deducidos de `translation/es.json` + `translation/parts/*.json`) y,
para cada uno:

1. localiza en **su propio** atlas las letras base necesarias (charmap +
   desempate por caja de tinta contra las copias limpias de `005`);
2. compone los glifos con el mismo código validado en F3 (`tools/add_accents.py`);
3. los escribe en celdas **libres** (transparentes/relleno uniforme). Si no hay
   bastantes, **reutiliza celdas kana/kanji** (texto japonés que la edición PAL
   inglesa no dibuja) — nunca el espacio ni celdas referenciadas por el texto;
4. actualiza la tabla de métricas del `.msg` (el glifo nuevo hereda la métrica
   de su base). **No cambia `count`/`count2`** → la huella del banco no cambia;
5. emite `extract/f2/accents/<stem>_accents.aif` + `.msg`, el manifiesto
   `translation/accents_manifest.json` y el binario de runtime
   `translation/accents_atlas.bin` (304.9 KiB);
6. con `--update-charmaps`, añade los caracteres acentuados al charmap del banco
   para que `build_es_codes.py` emita los códigos nuevos.

Comando:

```bash
python -X utf8 tools/add_accents_banks.py --need-from-es --update-charmaps
```

### Cobertura

* **44/44 bancos** con cadenas que usan acentos/`¿¡` → cubiertos.
* **300 glifos** en total (no los 16×44: solo se añade lo que cada banco usa y
  su atlas puede componer).
* **10 bancos** necesitan reutilizar celdas kana/kanji (el resto cabe en celdas
  libres): `d1_ud2_17325800_002` (8), `1793D000_018` (1), `224F6800_028` (2),
  `254C2000_039` (3), `2E8D1800_003` (5), `510DD000_025` (8), `7B2EA000_042` (6),
  `7EC08000_011` (3), `96119800_017` (1), `d2_ud1_78EA4000_017` (2).
* **2 bases ausentes** en el propio banco, compuestas con la imagen base de
  `005` (mismo tamaño de celda, 32×32): `d1_ud2_1793D000_018` (`í`, base `i`
  inexistente → se usa el tallo `l`) y `d2_ud1_68653000_037` (`ó`, no hay `o`).

## 2. El hook `mod/recomp/accents_es.h`

### Identificación del banco

Cada `RMD-` se carga como un buffer `[AIF ][message data]`. El parser del
directorio de mensajes (`sub_826D52C0`) recibe en `obj+0x64`/`obj+0x60` la base
del `.msg` (magic `"Mess"`). El atlas está **justo antes**:

```
aif = msgbase - aif_size          (aif_size = campo 0x58 del .msg)
```

(verificado en los 81 bancos). El hook se llama desde
`iu::translation::TrySubstitute` (que ya resolvió `msgbase`):

```cpp
iu::ui::OnMsg(base, msgbase);        // texturas (F3/ui_textures)
iu::accents::OnMsg(base, msgbase);   // acentos por banco (WS1)  <-- nuevo
```

`OnMsg`:

1. valida `magic == "Mess"` y `aif_size`;
2. identifica el banco por **firma barata** (ident + formato + tamaño +
   dimensiones del AIF, un `memcmp` de 0x40 B) y **FNV-64 del AIF original**
   (firma fuerte: 6 grupos de firma barata tienen contenido distinto entre
   bancos, así que el hash es necesario);
3. escribe los bloques DXT de cada glifo en las celdas elegidas (mismo
   direccionamiento *tiled* de Xbox 360 que `tools/aif.py`);
4. parchea la tabla de métricas del `.msg`: la métrica del glifo nuevo es la de
   su **celda base leída del `.msg` cargado** (correcto aunque dos discos
   compartan el mismo atlas pero distinto `.msg`).

**Coste:** el primer avistamiento de un `msgbase` hace el FNV (cientos de KB);
después se **cachea por `msgbase`** y los frames siguientes solo reaplican
~16 KiB. No se toca el escáner ni su cadencia de 60 s.

### Fallback de atlas ya sustituido

`ui_textures_es.h` todavía sustituye el atlas `005` completo (entrada del
manifiesto `ui_textures.txt`). En ese caso el AIF en memoria ya **no** es el
original y su FNV no casa. Como la firma barata de `005` es única, el hook
acepta el banco cuando la firma barata identifica **un solo** candidato; si hay
ambigüedad, no parchea (seguro).

## 3. `build_es_codes.py`

`max_code` por banco ahora es `300 + celdas` si el banco está en
`translation/accents_manifest.json` **o** existe
`extract/f2/accents/<stem>_accents.aif`. Con el charmap ampliado
(`--update-charmaps`), los acentos **dejan de plegarse a ASCII**. Se
regeneraron `translation/es_codes.json` y `translation/es_catalog.bin`
(42 760 cadenas, 47 bancos) y se desplegaron a `extract/f2/run/translation/`.

> Comprobación: **ningún** carácter de `á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿ ¡`
> aparece ya en los avisos `sin glifo`.

## 4. Verificación offline

`tools/verify_accents_banks.py` **emula exactamente el hook** (mismo
direccionamiento tiled) y comprueba, para los 44 bancos:

* el FNV-64 del AIF original coincide con el del bin;
* el atlas parcheado por el hook es **idéntico byte a byte** al
  `<stem>_accents.aif` generado;
* las métricas parcheadas son **idénticas** al `<stem>_accents.msg`;
* cada glifo tiene tinta (no es una celda vacía);
* *round-trip* del códec del juego para los códigos nuevos.

```
$ python -X utf8 tools/verify_accents_banks.py
bin: 44 bancos; manifiesto: 44 bancos
  [ok] d1_ud1_000A4000_005   glifos=16 aif== msg== rt=ok
  ...
  [ok] d2_ud1_78EA4000_017   glifos= 3 aif== msg== rt=ok
RESULTADO: TODO OK
```

Comprobaciones adicionales:

* los **300** glifos compuestos **difieren** de su celda base (no es una copia);
* los **300** pares (carácter, código) del manifiesto coinciden con el
  `charmap` que usa `build_es_codes.py` (0 fallos) → los códigos emitidos son
  los correctos;
* al decodificar `es_codes.json` con el charmap, cadenas como
  `"¡Patético flautista!"`, `"¡¡Siempre ten agallas!!"` o `"propósito"`
  recuperan los acentos/`¿¡` (no `?`).

## 5. Lo que NO queda cubierto (honestidad)

1. **Glifos latinos ausentes por banco (F3-b).** Muchos atlas de diálogo no
   tienen todas las letras latinas (`Q`, `V`, `q`, `«`, `»`, `…`…): 731 avisos
   `sin glifo` en 45 bancos. Ejemplo: `d2_ud1_68653000_037` no tiene `B`, así
   que `"¡Bastón…"` sale `"¡astón…"`. Es un problema **pre-existente y
   distinto** de los acentos (STATUS §2). El mecanismo por banco de este hook
   podría reutilizarse para copiar también esas letras desde `005`, pero queda
   fuera del alcance de esta tarea.
2. **10 bancos reutilizan celdas kana/kanji** (~39 celdas). El texto japonés de
   esos `.msg` (no mostrado en la edición PAL inglesa) renderizaría el glifo
   acentuado. Si el juego mostrara japonés en algún menú, esas celdas se verían
   mal.
3. **Sin prueba en juego.** El usuario prohibió ejecutar el juego; la prueba es
   la emulación byte-exacta del hook (arriba), no una captura en vivo.
4. `ui_textures_es.h` sigue sustituyendo el atlas `005` completo (redundante con
   este hook, pero inofensivo: el hook reescribe las mismas celdas con las
   bases nuevas). No se tocó `ui_textures.py` (regla del encargo).

## 6. Reproducción

```bash
# 1) material por banco (atlas ampliados + bin + manifiesto + charmaps)
python -X utf8 tools/add_accents_banks.py --need-from-es --update-charmaps

# 2) verificacion offline (emula el hook)
python -X utf8 tools/verify_accents_banks.py

# 3) catalogo (acentos sin plegar)
python -X utf8 tools/build_es_codes.py

# 4) desplegar datos + compilar + desplegar exe
cp translation/accents_atlas.bin extract/f2/run/translation/
cp translation/es_catalog.bin    extract/f2/run/translation/
bash mod/recomp/build.sh
cp extract/f2/recomp/out/build/local/InfiniteUndiscoveryRecomp.exe extract/f2/run/
```

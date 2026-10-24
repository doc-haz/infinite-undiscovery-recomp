# Cartel de nombre de ZONA (banner de entrada) — investigación y estado

> **Fecha:** 2026-10-09. **Alcance:** localizar/traducir la textura del rótulo que
> aparece **centrado en pantalla al entrar en una zona nueva** (p. ej. «Graad
> Prison»), que en la build ES sigue **en inglés**.
>
> **Conclusión corta:** **NO es una textura AIF pre-renderizada** (barrido OCR
> exhaustivo de los dos discos: 0 resultados). Es **texto compuesto en runtime
> con una fuente SERIF**, y su origen no es (a) el resolvedor `.msg` ni (b) la
> tabla de nombres de zona del `.exe`. Queda localizar el *renderer* concreto
> (plan de sonda en vivo al final).

---

## 1. Evidencia de partida

* `extract/f2/run/camp_test_shots/post_3.png` (2560×1440) muestra el banner
  **centrado**: `Graad Prison`, en una **fuente serif fina** sobre una banda
  horizontal translúcida oscura. En la **misma sesión ES** `post_4.png` muestra
  ya el HUD traducido (`TARGET`→`BLANCO`, `Punto de guardado`, `Examinar`), y
  `post_7.png` / `camp3_shots/s32.png` muestran el nombre de zona **traducido**
  en el mapa/carga (`Prisión de Graad`, fuente **sans**).
  → el banner (serif, centro) es el único rótulo de zona que **no** cambia,
  mientras que el rótulo del mapa (sans) **sí** se traduce por el hook.
* El banner va **solo** (sin ventana de mensaje): parece un *overlay* de entrada
  de zona, no un `CTextWnd`/`CMessageTextWnd` normal.

## 2. Barrido OCR de texturas (lo que se ha DESCARTADO)

Método: recorrer **todo** fichero extraído de `all_d1ud1` (ud1.bin) y
`all_d1ud2` (ud2.bin), localizar cada firma `AIF `, decodificar el bitmap
(todos los formatos) y pasar **tesseract** con dos fondos (blanco/negro),
buscando nombres de zona (`Graad`, `Prison`, `Woods`, `Dragonbone`, `Halgita`,
`Kolton`, `Nulaan/Nolaan`, `Burgusstadt`, `Oradian`, `Luze`, `Prevant`,
`Sapran`, `Zala`, `Fayel`, `Vesplume`, `Bihar`, `Plodhif`, `Pieria`,
`Cobasna`, `Seraphic`, …).

| Conjunto | Alcance | Resultado |
|---|---|---|
| `.aif` sueltos ud1 | 220 ficheros | **0** nombres de zona |
| `.aif` sueltos ud2 | 652 ficheros | **0** |
| AIF incrustados en `.asf/.mron/.bin/.aaf/.acf` ud1 | 13 887 ficheros | **0** (solo el logo `infinite undiscovery`) |
| AIF incrustados ud2 | 6 933 ficheros | **0** |
| Formatos “raros” `0x41/0x55/0x59/0x4C` (ud1) | 139 AIF | **0** |
| Formato `0x0F` (2 312 ud1 / 25 dims ud2) | max **32×…**, casi todos **8 px de ancho** | tiras de gradiente, **sin texto** |

**Validez del método:** el mismo escáner detecta correctamente
`274AE800_000_IMG.aif` = `GAME OVER` y `28EB4000_000_IMG.aif` =
`Now Loading…`. Es decir, si el banner fuera una AIF con texto, se habría
encontrado.

> Reconstruible con `extract/f2/re_analysis/_banner_run.py`
> (salida: `extract/f2/re_analysis/banner_scan/*.json`).
> (Nota: los scripts se movieron a `re_analysis/`; ajusta el `sys.path` si los
> relanzas desde ahí.)

## 3. La fuente del banner SÍ está en el atlas principal (banco 005)

El banner es **serif**. En el atlas de la fuente principal
`d1_ud1_000A4000_005` (768×768, 24×24 celdas de 32 px) **hay una fuente serif
completa** empaquetada en las **filas 20–22** (códigos ≈ **781–852**):

```
fila 20: 蚕 ; — & ? * U n g h . Y o u ' r e   p t y L s f
fila 21: ! S i w c a d , m ? A l k b I v T W j H D P x -
fila 22: z O M N J B q C G F " E R Q — K 9 8 1 0 7 % V Z
fila 23: : / ; 3 &
```

Recomponiendo `Graad Prison` con esas celdas
(`extract/f2/_render_serif_graad.png`) la forma coincide con el banner. Es
decir: **el banner usa glifos serif del atlas del banco 005**.

Esto es importante porque el banco 005 **sí** está en el catálogo ES
(`fp=DC0D11A6`) y el hook lo traduce… **pero el banner no se traduce**, luego el
texto del banner **no llega al resolvedor `sub_826D52C0`**.

## 4. ¿De dónde sale la cadena? (lo que también se ha DESCARTADO)

1. **No es un mensaje `.msg` traducible.** Los nombres de zona sueltos
   (`Graad Prison`, `Graad Woods`, `Dragonbone Shrine`, `Halgita`, `Nolaan`,
   `Burgusstadt`, `Port Zala`, …) existen **solo** en el banco `005`, en los
   ids `100xxx`, `104xxx`, `105xxx` y `78000xxx`. **Todos** están en el
   catálogo/partes ES (p. ej. `100203`/`78000203` → «Prisión de Graad»). Si el
   banner usara cualquiera de ellos, saldría en castellano.
2. **No llega al render de texto instrumentado.** `IU_ES_DIAG` (hook de
   `sub_826D43F8`) no registra ningún id de zona: en un run de ~480 s solo hay
   **140** textos distintos renderizados, y los únicos `ours=0` son
   `10424/10401/10399/10400` (cadenas basura/JP de depuración) y `id=0`.
   Ningún `100150/100203/78000xxx`.
3. **No es la tabla de nombres del `.exe`.** `default.exe` contiene la tabla
   `MapName` en `0x82A01A58` (59 registros de **68 B**: `u32 zoneId` + nombre
   ASCII). El escaneo de **todos** los pares `lis/addi` de `.text` que resuelven
   a esa tabla encuentra **un único lector**: `sub_8247FF00`, el formateador de
   la **ranura de guardado** (`%03d:%02d:%02d %s %s %s`, id de zona ×68 +
   base+4). Ningún renderer de banner la consulta.
4. **No es una AIF (punto 2).** Tampoco una AIF comprimida: la extracción de
   `ud1.bin`/`ud2.bin` se hizo con `--decompress` (SLZ) y el escáner recorre
   también los AIF incrustados en ASF/MRON.
5. **La cadena no está en ningún recurso**, ni en ASCII (`grep -a "Graad
   Prison"`/`"Halgita"`/`"Dragonbone Shrine"` sobre ud1+ud2: 0) ni como
   secuencia de códigos (serif o sans) con `\0`/sin él. Tampoco hay un elemento
   de UI por nombre (`sub_82182070`): barrido streaming de `ud1.bin` y `ud2.bin`
   por `P…Map/Area/Zone/Name…` solo devuelve materiales (`PnormalMap…`).

## 5. Contradicción pendiente / hipótesis viva

Si el banner usa los glifos serif del banco 005, debería pasar por el motor de
texto, y entonces el hook lo traduciría. Como no lo hace, la hipótesis con más
recorrido es:

> **El banner se compone con un *renderer* propio (widget de “título de zona”)
> que NO llama a `sub_826D52C0`**: toma la cadena **en ASCII** (de la tabla
> `MapName` del `.exe` o de la memoria de estado) y la dibuja con los glifos
> serif del atlas 005 mediante un mapeo `ASCII→código de glifo` propio.

Ninguna de las dos partes (lectura de la tabla / mapeo ASCII→serif) se ha
localizado estáticamente, lo que apunta a que el *renderer* está en una ruta
poco transitada (o usa un puntero a la tabla guardado en un objeto, no una
constante).

## 6. Opciones / siguiente paso (sonda en vivo)

No se ha ejecutado el juego (lo está usando el usuario). Para cerrar el caso:

**A. Sonda de texto (barata, ya existe).** Lanzar con `IU_ES_DIAG=1` y entrar en
una zona **nueva** (no cargar partida): revisar
`EUROPE/logs/iu_trace.log` por líneas `ES RD ours=0 id=…` en el instante del
banner. Si aparece un id, añadirlo al catálogo (traducirlo) basta.

**B. Sonda de AIFs cargados (ya existe).** `IU_ES_AIFPROBE=1` al entrar en zona:
lista cada `AIF `/`ao__` que despacha `sub_821EF618/678/822226D8/8221D768`. Si el
banner fuese textura, aparecería aquí y podríamos volcarla y sustituirla por
firma (como el resto).

**C. Volcado en memoria del texto del banner.** Añadir un hook (o extender
`IU_ES_DIAG`) que, al detectar el banner (p. ej. al llegar a cierta pantalla),
recorra la memoria guest buscando la cadena `"Graad Prison"` en **ASCII** o en
**códigos serif** (`0x0345 0x031C …`) y volque el puntero + los bytes vecinos.
Eso identifica de una vez el origen (tabla del exe vs. buffer de estado vs.
literal de código).

**D. (Experimento barato, si se confirma C.)** Como la tabla `MapName` del `.exe`
se carga en memoria guest en `0x82A01A58` y es **escribible**, se puede
**reescribir en runtime** (desde el hook, sin tocar assets) con los nombres en
castellano. Si el banner la lee por cualquier vía, quedaría traducido; si no,
es inocuo. Requiere confirmar primero que el banner lee esa tabla (paso C).

## 7. Qué NO se ha tocado

* `ud1.bin`/`ud2.bin` y cualquier asset: intactos.
* `mod/recomp/*`: sin cambios. No se ha añadido ningún hook especulativo.

## 8. Ficheros de apoyo (no versionados, en `extract/f2/`)

* `_banner_crop.png` — recorte ampliado del banner (`post_3.png`).
* `_render_serif_graad.png` — «Graad Prison» compuesto con los glifos serif del
  atlas 005 (coincide con el banner).
* `_005_row20/21/22/23.png` — filas serif del atlas 005.
* `_small_fonts.png`, `_font005_*.png` — cribado de fuentes.
* `extract/f2/re_analysis/banner_scan/` — resultados JSON del barrido OCR.

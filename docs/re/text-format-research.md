# Investigación web — formato de texto/mensajes de tri-Ace (motor ASKA, Xbox 360)

> **Objetivo:** averiguar si *alguien* ha documentado o decodificado públicamente el
> formato de textos de los juegos de tri-Ace con motor **ASKA** en Xbox 360,
> centrándose en *Infinite Undiscovery* (2008): recursos `RMD-`, bloque `.msg`,
> `MessageConvertLib_1.0.0.0`, atlas de fuente `AIF ` y el layout
> cabecera + tabla `id→offset` + pool de códigos.
>
> **Método:** búsquedas web (websearch + DuckDuckGo HTML), API de GitHub
> (repos/trees/issues), `grep.app`, foros ResHax/ZenHAX, y lectura de los repos
> de RE/port relevantes. **No** se ha ejecutado nada pesado en local.
>
> **Fecha:** 2026-10-08.

---

## 0. TL;DR (conclusión)

1. **No.** Nadie ha publicado una decodificación del bloque `.msg` de ASKA ni del
   layout `RMD-` (cabecera + directorio + pool de códigos). La comunidad de RE de
   tri-Ace/Xbox 360 es **muy pequeña** y solo un proyecto público toca ASKA/IU.
2. El único proyecto público sobre *Infinite Undiscovery*/**ASKA** es
   **[vs-sr-dev/pc-infiniteundiscovery](https://github.com/vs-sr-dev/pc-infiniteundiscovery)**.
   Documenta que un `RMD-` = **atlas `AIF ` + "message data"** y deja la mitad de
   mensaje **sin decodificar** (`docs/formats/resource-payloads.md`, `docs/formats/aif.md`).
3. Los términos **`MessageConvertLib`** y **`MessageConvertLib_1.0.0.0`** no
   aparecen indexados **en ningún sitio** de la web (0 resultados).
4. **No existe** fan-translation de *Infinite Undiscovery*, ni herramienta pública
   de texto/mensaje para IU, Star Ocean 4 ni Resonance of Fate. Solo hay *save
   editors*, *undub* (voces JP, texto EN) y RE de formatos gráficos/compresión.
5. **Hallazgo explotable:** el formato de mensajes de **Radiata Stories** (tri-Ace,
   PS2, 2005) — `RMF1` — **sí está documentado y con editor funcional** en
   **[Martinity/RadiataModdingTool](https://github.com/Martinity/RadiataModdingTool)**.
   Su VM de mensajes usa exactamente opcodes `0x02`/`0x03`/`0x06`
   (*Color* / *Dimensions* / *Signal*) y glifos codificados en palabras de 2 B.
   Es el **análogo documentado más cercano** y encaja con el "byte tipo" que
   describes. Ver §3.
6. El ecosistema **ReXGlue / XenonRecomp** (usado por el port nativo de IU) **no
   tiene capa de texto genérica**; cada juego la implementa. No hay herramienta
   reutilizable de "texto" para 360.

---

## 1. Búsquedas realizadas (trazabilidad)

Motores usados: `websearch`, DuckDuckGo HTML (`html.duckduckgo.com`), API GitHub
(`/search/repositories`, `git/trees`), `grep.app`, y `webfetch` de foros.

| # | Consulta | Resultado relevante |
|---|---|---|
| 1 | `Infinite Undiscovery text format RMD message resource ASKA engine` | → repo `vs-sr-dev/pc-infiniteundiscovery` |
| 2 | `tri-Ace ASKA engine MessageConvertLib message format` | sin relación (R Markdown) |
| 3 | `pc-infiniteundiscovery ASKA engine text format RMD GitHub` | repo + docs |
| 4 | `"MessageConvertLib" tri-Ace` | **0 resultados** |
| 5 | `"MessageConvertLib_1.0.0.0"` | **0 resultados** |
| 6 | `"MessageConvertLib" OR "MessageConvertLib.dll" font message converter` | **0 resultados** |
| 7 | DuckDuckGo `"MessageConvertLib"` | **"No results found"** |
| 8 | `Infinite Undiscovery fan translation text tool romhacking` | sin traducción |
| 9 | `Infinite Undiscovery translation tool xentax zenhax message` | sin herramienta |
| 10 | `"Infinite Undiscovery" translation GBAtemp OR xentax OR romhacking` | solo discusión general |
| 11 | `github tri-ace message font RMD tool` | topic `tri-ace` (4 repos) |
| 12 | `Resonance of Fate text tool translation extract` | solo save editors / wikis |
| 13 | `Star Ocean 4 text tool translation extract tri-Ace` | solo prensa/entrevistas |
| 14 | `"Star Ocean 4" font atlas glyph code mod tool` | sin herramienta de texto |
| 15 | `github "aska" engine tool text OR message OR font star ocean` | doc de ASKA (mobile) |
| 16 | `"RMF1" tri-Ace Radiata Stories message format opcode PutText` | sin doc externa (la fuente es el tool) |
| 17 | `tri-Ace message opcode PutText glyph encoding RMF format reverse engineering` | sin doc externa |
| 18 | `reshax tri-Ace message text format ASKA Star Ocean Resonance of Fate` | threads de compresión, no de texto |
| 19 | `"Star Ocean 4" ".msg" OR "message" file` | sin herramienta |
| 20 | `"Infinite Undiscovery" RMD message resource RMD- format tool` | **0 resultados** |
| 21 | `XenonRecomp ReXGlue translation patch text inject game` | sin capa de texto |
| 22 | `インフィニット アンディスカバリー テキスト 抽出 フォント 解析` | sin resultados |

Conclusión de la traza: **el vacío es real**, no un problema de consulta. Los
buscadores no conocen ni el nombre del middleware (`MessageConvertLib`) ni el
formato `.msg` de ASKA.

---

## 2. Proyectos y enlaces encontrados

### 2.1 ASKA / Infinite Undiscovery (lo único que toca el motor)

- **[vs-sr-dev/pc-infiniteundiscovery](https://github.com/vs-sr-dev/pc-infiniteundiscovery)**
  — "Reverse engineering notes, format documentation and analysis tooling for
  Infinite Undiscovery (tri-Ace / Square Enix, Xbox 360, 2008) and its ASKA
  engine". Python, sin dependencias. **Es la fuente pública principal.**
  - [`docs/formats/resource-payloads.md`](https://github.com/vs-sr-dev/pc-infiniteundiscovery/blob/master/docs/formats/resource-payloads.md):
    tabla de tags. Dice literalmente: *"`RMD-` | `AIF ` | a font atlas, followed by
    message data"* y *"`RMD-` is a message resource, not just an image"*.
    **No describe el layout del mensaje.**
  - [`docs/formats/aif.md`](https://github.com/vs-sr-dev/pc-infiniteundiscovery/blob/master/docs/formats/aif.md):
    formato `AIF ` completo (cabecera 4096 B, tiling Xbox 360, DXT). Menciona el
    atlas de fuente y la cadena `MessageConvertLib_1.0.0.0` *"trailing three RMD-
    payloads"*, pero **no decodifica el `.msg`**.
  - [`docs/aska-across-titles.md`](https://github.com/vs-sr-dev/pc-infiniteundiscovery/blob/master/docs/aska-across-titles.md):
    ASKA está en 9 de 12 títulos de tri-Ace (SO4, RoF, SO5, Anamnesis…). Los
    formatos (`SLZ`, `AIF`, `ASF`, `AAF`, `SNC`…) se comparten **entre juegos** →
    pista de que el mensaje de SO4/RoF podría reutilizar el de IU.
  - Herramientas: `tools/mron.py` (contenedores NORM/MRON), `tools/aif.py`
    (texturas), `tools/snc.py` (scripts de escena), `tools/slz.py` (compresión).
    **No hay parser de mensajes.**
  - `TODO.md` **no lista** el texto/mensajes como problema abierto: el proyecto
    nunca abordó el subsistema de mensajes.

### 2.2 Ports nativos / recompilación (ReXGlue / XenonRecomp)

- **[doc-haz/infinite-undiscovery-recomp](https://github.com/doc-haz/infinite-undiscovery-recomp)**
  — Port nativo x64 de IU vía **ReXGlue v0.10.0 / XenonRecomp**, portable,
  perfiles USA / USA-UNDUB / EUROPE / JAPAN / ASIA, disc1↔disc2, DLC.
  - **No** modifica texto. El perfil **USA-UNDUB** es *voces japonesas + texto
    inglés*: es un **swap de audio**, no una traducción (confirma el usuario del
    issue #2).
  - Acredita a `vs-sr-dev/pc-infiniteundiscovery` como fuente de investigación.
  - Issues abiertos (#1–#4): rendimiento, ultrawide, softlock, sin nada de texto.
- **[doc-haz/iu-save-bridge](https://github.com/doc-haz/iu-save-bridge)** — editor
  de saves (C#), recalcula CRC32 de tri-Ace. Útil para *herramientas de save*, no
  para texto.
- **[vs-sr-dev/android-talesofcrestoria-doc](https://github.com/vs-sr-dev/android-talesofcrestoria-doc)**
  — ASKA en Android (`Tales of Crestoria`): documenta `SLZ`/Zstandard, `ISF`,
  texturas `ETC2`, UI de Cocos Studio… **tampoco documenta el sistema de
  mensajes** (la UI va por Cocos).
- **[freefrank/LostOdysseyRecomp](https://github.com/freefrank/LostOdysseyRecomp)**
  — recomp de *Lost Odyssey* (motor propio, no ASKA). Referencia de flujo de
  trabajo portable, no de formato de texto.
- **[rexglue/rexglue-sdk](https://github.com/rexglue/rexglue-sdk)** ·
  **[hedge-dev/XenonRecomp](https://github.com/hedge-dev/XenonRecomp)** — el
  compilador de PPC→C++ no trae extracción/inyección de texto; el texto es
  responsabilidad de cada juego.

### 2.3 Tribus hermanas de tri-Ace (lo más parecido a una decodificación)

- **[Martinity/RadiataModdingTool](https://github.com/Martinity/RadiataModdingTool)**
  — herramienta de modding de *Radiata Stories* (tri-Ace, PS2, 2005) con
  **Message Editor `.rmf` funcional** y plugins de formato.
  Código: [`core/handlers/rmf_leaf.py`](https://github.com/Martinity/RadiataModdingTool/blob/main/core/handlers/rmf_leaf.py)
  y [`ui/editors/rmf_editor.py`](https://github.com/Martinity/RadiataModdingTool/blob/main/ui/editors/rmf_editor.py).
  **Es el formato de mensajes tri-Ace mejor documentado públicamente.** Ver §3.
- **[trulio2/Valkyrie-Profile-2-Tools](https://github.com/trulio2/Valkyrie-Profile-2-Tools)**
  — *Valkyrie Profile 2* (PS2, 2006): traductor completo + **herramienta de
  glifos** (`vp2_glyphs.py`) que extrae *cada glifo como PNG* y genera texturas
  de reemplazo. Enfoque de traducción **a nivel de atlas de fuente** reutilizable
  conceptualmente. Docs: [`docs/glyphs.md`](https://github.com/trulio2/Valkyrie-Profile-2-Tools/blob/master/docs/glyphs.md).
- **[Rogus/Valkyrie-Profile-2-Translation-Pt-BR](https://github.com/Rogus/Valkyrie-Profile-2-Translation-Pt-BR)**
  — traducción (TypeScript) de VP2; otra fuente de "cómo se traduce un tri-Ace
  con texturas/glifos", aunque no publica el formato de texto en bruto.
- **[vs-sr-dev/snes-talesofphantasia-doc](https://github.com/vs-sr-dev/snes-talesofphantasia-doc)**
  — doc de formato (Namco/tri-Ace temprano); no aporta al mensaje ASKA.

### 2.4 Foros (ResHax / ZenHAX) y agregadores

- [ResHax · "Resonance of Fate | End of Eternity"](https://reshax.com/topic/849-resonance-of-fate-end-of-eternity/)
  — container `P@CK` + `SLZ` + `AAF` (PC). **No** trata texto.
- [ResHax · "Resonance of fate PC"](https://reshax.com/topic/17082-resonance-of-fate-pc/)
  — `SLZ` tipo 02 / "start of the…". Sin texto.
- [ResHax/ZenHAX · "STAR OCEAN -anamnesis- (tri-ace New SLZ?)"](https://reshax.com/topic/15508-star-ocean-anamnesis-tri-ace-new-slz/)
  — compresión; sin texto.
- [ResHax/ZenHAX · "Heaven x Inferno (tri-Ace SLZ type 5)"](https://reshax.com/topic/13237-heaven-x-inferno-tri-ace-slz-type-5/)
  — compresión; sin texto.
- [ZenHAX · "STAR OCEAN -anamnesis- (tri-ace New SLZ?)"](http://zenhax.com/viewtopic.php@t=7149.html)
  — compresión/modelos; sin texto.
- [GitHub topic `tri-ace`](https://github.com/topics/tri-ace) — solo 4 repos
  (`pc-infiniteundiscovery`, `RadiataModdingTool`, `snes-talesofphantasia-doc`,
  `android-talesofcrestoria-doc`). **No hay ningún repo de texto/mensajes.**
- [VelocityRa/awesome-game-file-format-reversing](https://github.com/VelocityRa/awesome-game-file-format-reversing)
  — lista de referencia: **no** tiene entrada de tri-Ace/ASKA ni de mensajes ASKA.
- Comunidad X360: [ReXGlue](https://github.com/rexglue/rexglue-sdk) no publica
  canal de RE de texto; los Discord de REGames/Reverse-Engineering son genéricos.
  **No se ha encontrado ningún Discord "ASKA" público dedicado a texto.**

### 2.5 Otras plataformas / idiomas (herramientas de extracción-inyección)

- **Radiata Stories (.rmf)** — editor de mensajes funcional (§2.3).
- **Valkyrie Profile 2 (PS2)** — traductor + glifos (§2.3).
- **Star Ocean 4 (PC remaster)** — [Nexus Mods](https://www.nexusmods.com/games/starocean4remaster):
  solo QoL/plugins; **ninguna herramienta de texto/fuente**.
- **Star Ocean: The Divine Force / Second Story R** — mods de idioma (p. ej.
  Tailandés) en Nexus, pero sin liberar el formato crudo del motor ASKA/X360.
- **Infinite Undiscovery (X360)** — solo *save editors* (360Haven/XPGamesaves),
  *cheats* de Xenia (`chuckycheese666/Infinite-Undiscovery`) y el recomp. **Nada
  de texto.**

---

## 3. El hallazgo más útil: `RMF1` (Radiata Stories) como análogo directo

El formato `.rmf` de *Radiata Stories* está **implementado y explicado** en
`RadiataModdingTool` (`core/handlers/rmf_leaf.py`). Su estructura:

```
RMF1
├─ header: magic "RMF1", version, glyph_data_size, packet_data_size,
│          packet_count, glyph_data_end
├─ tabla de offsets  u32[packet_count]  (0xFFFFFFFF = hueco)
│     └─ cada offset → un "packet" (mensaje)
├─ packets: glyph_budget (u32), speaker_count (u32), speakers[count],
│           y un flujo de tokens
└─ cola: datos de glifos/fuente + anchos embebidos
```

**Flujo de tokens (VM de mensajes de tri-Ace), palabras de 2 B little-endian:**

```python
b0 = word & 0xFF
b1 = (word >> 8) & 0xFF
if b0 >= 0x10:            # GLIFO
    glyph_index = (b0 - 0x10) + (b1 & 0x3F) * 240
    # b1 & 0x80 -> "tile embebido" (glifo de la fuente incrustada)
else:                     # COMANDO de control
    opcode = b0
    payload_words = (b1 >> 4) & 0xF     # nº de palabras de payload
```

**Tabla de opcodes documentada (PUTTEXT):**

| Opcode | Nombre | Payload | Semántica |
|---|---|---|---|
| `0x00` | End | 0 | fin de mensaje |
| `0x01` | Space | 0 | avance de espacio |
| **`0x02`** | **Color** | 16 | RGBA de texto + sombra |
| **`0x03`** | **Dimensions** | 16 | ancho/alto de celda (U8.8) |
| `0x04` | Position | 16 | ancla (x,y) + cursor relativo |
| `0x05` | Text Style | 0 | estilo en nibble alto de cada glifo |
| **`0x06`** | **Signal** | 8 | `_CbSignal` (sincronización) |
| `0x07` | Text Speed | 8 | velocidad U8.8 |
| `0x09` | Style Save/Load | 8 | snapshot de estilo |
| `0x0A` | Newline | 0 | salto de línea |
| `0x0E` | (grupo bust-up) | var | `0x10+sub` = speaker, cara, emoción… |
| `0x0F` | (grupo flujo) | var | `0x20+sub` = WaitTime, WaitSelect, Sentence… |

**Por qué esto es relevante para tu `<msg>`:**

- Los "bytes tipo" **`0x02` / `0x03` / `0x06`** que describes **coinciden
  exactamente** con los opcodes *Color / Dimensions / Signal* de la VM de mensajes
  tri-Ace. Es muy plausible que el "pool" del `.msg` de ASKA sea un **flujo de
  tokens de mensaje** (glifos + comandos), no un atlas de glifos suelto.
- La forma "cabecera + tabla `id→offset` + pool" también es la de `RMF1`
  (header + tabla de offsets + packets), lo que refuerza que el diseño de
  mensajes se heredó de PS2 a ASKA.

> **Cautela:** `RMF1` es PS2/2005 y ASKA es X360/2008. La *semántica* de opcodes
> puede haber cambiado y el ancho de palabra/endianness también (ASKA es
> big-endian). No es prueba de identidad, sino la **mejor hipótesis documentada**
> y un **modelo de parser** listo para adaptar.

---

## 4. Contraste con los datos de `RMD-` / `.msg` que ya tenemos

### 4.1 Lo que dice la fuente pública (`vs-sr-dev`)

- `RMD-` → payload `AIF ` (atlas de glifos) **seguido** de "message data" que
  empieza por `MessageConvertLib_1.0.0.0`. **No hay layout del `.msg`.**
- `AIF ` está completamente decodificado (header 0x1000, tiling, DXT).
- `MessageConvertLib_1.0.0.0` aparece "trailing three RMD- payloads".
- **No menciona** la tabla de registros de 16 B `(A,B,C,D)`, ni el pool, ni los
  bytes de tipo `0x02/0x03/0x06`. Es decir: **tus datos van por delante de la web**.

### 4.2 Lo que ya tiene este repo (para no duplicar)

`docs/re/font-loader.md` (§1) ya describe el `.msg` **resuelto** con esta lectura:

- header BE (`records_off=0x20`, `pool_off=0x24`, `metrics_off=0x28`, `count`,
  `aif_size`, `atlas_w/h`…);
- **directorio de mensajes** `[records_off, pool_off)`: registros de **16 B BE**
  `id, offset, width(f32), height(f32)` — el `D` que observas (26/51/77) es
  **alto de línea / tamaño de fuente**;
- **pool** `[pool_off, metrics_off)`: **byte-stream terminado en `0x00`**, con
  decodificación variable:
  ```c
  b0 = *p++;
  if (b0 & 0x80) { b1 = *p++; code = (b1 << 7) | (b0 & 0x7F); }  // 15 bits
  else            code = b0;                                       // 1 byte
  ```
  - `code >= 0x4000` → **control** (tabla de 26 entradas, `0x4000..0x4019`);
  - `code < 0x4000` → **glifo**; índice `= code - 301` (códigos ≥ 300) o
    `code - 1` (códigos < 300);
- **mapa confirmado** por render: `celda = code - 301`, rejilla
  `atlas_w/cell_w × atlas_h/cell_h`, row-major (texto japonés legible).

**Discrepancia a aclarar:** tu descripción habla de un pool de **palabras de 2 B
con byte "tipo" `0x02/0x03/0x06`**. La lectura local (`font-loader.md`) es un
stream **variable** con **controles en `0x4000..0x4019`**. Las dos cosas pueden
convivir si el pool mezcla códigos de glifo de 15 bits y comandos, pero conviene
verificar si los `0x02/0x03/0x06` son:

- **(a)** el *low byte* de un comando big-endian `0x4002/0x4003/0x4006` (que en
  tu volcado aparecerían como bytes `02/03/06`), o
- **(b)** opcodes de la familia `RMF1` (Color/Dimensions/Signal).

En ambos casos, **casar el flujo de ASKA con la tabla de opcodes de `RMF1`** es el
experimento más barato y prometedor.

---

## 5. Herramientas reutilizables (con enlace y utilidad)

| Herramienta | Enlace | Qué aporta a nuestro caso |
|---|---|---|
| `pc-infiniteundiscovery` | [repo](https://github.com/vs-sr-dev/pc-infiniteundiscovery) | `mron.py` (extrae `RMD-` descomprimiendo), `aif.py` (exporta el atlas a PNG), `slz.py`, `snc.py`. Base para extraer el `.msg` y el `AIF ` sin escribir parser propio. |
| `RadiataModdingTool` (`rmf_leaf.py`) | [repo](https://github.com/Martinity/RadiataModdingTool) | **Modelo de parser de mensajes tri-Ace**: header + offset table + packets + tokens 2 B + opcodes + decodificación de glifos. Adaptable a ASKA. |
| `rmf_commands.json` | [asset](https://github.com/Martinity/RadiataModdingTool/blob/main/ui/assets/rmf_commands.json) | Tabla de comandos/opcodes ya volcada; punto de partida para mapear los del `.msg`. |
| `Valkyrie-Profile-2-Tools` (`vp2_glyphs.py`) | [repo](https://github.com/trulio2/Valkyrie-Profile-2-Tools) | Flujo **traducción por atlas de glifos**: extraer cada glifo a PNG, editar, reinyectar. Aplicable al `AIF ` de `RMD-` (F3). |
| `android-talesofcrestoria-doc` | [repo](https://github.com/vs-sr-dev/android-talesofcrestoria-doc) | Confirma ASKA en otras plataformas y patrones `SLZ`/`AIF`. No tiene mensajes. |
| `iu-save-bridge` | [repo](https://github.com/doc-haz/iu-save-bridge) | CRC32 de tri-Ace (relevante si hay que re-firmar recursos). |
| `LostOdysseyRecomp` | [repo](https://github.com/freefrank/LostOdysseyRecomp) | Referencia de port nativo (no de formato de texto). |
| Xenia compat issues | [#779 IU](https://github.com/xenia-project/game-compatibility/issues/779) · [#389 SO4](https://github.com/xenia-project/game-compatibility/issues/389) | Etiqueta `tech-engine-aska`; estado de emulación, no formato. |
| `awesome-game-file-format-reversing` | [repo](https://github.com/VelocityRa/awesome-game-file-format-reversing) | Índice de referencia (no contiene ASKA/mensajes). |

---

## 6. Recomendaciones (a partir de la investigación)

1. **No esperar una decodificación ajena**: no existe. La web confirma que el
   `.msg` de ASKA está sin documentar; nuestro `font-loader.md` es hoy la mejor
   descripción pública (aunque no publicada).
2. **Adoptar la tabla de opcodes de `RMF1` como hipótesis de trabajo** para el
   pool del `.msg`. Verificar si los bytes `0x02/0x03/0x06` observados son
   *Color/Dimensions/Signal* (`0x4002/0x4003/0x4006` en BE de 15 bits) y si
   `End=0x00`, `Newline=0x0A`, `Space=0x01` aparecen con la misma semántica.
   Un `strings`-like del pool con esos opcodes debería producir frases.
3. **Reutilizar `RadiataModdingTool` como andamiaje de parser** (offset table +
   packets + tokens) y adaptar endianness/ancho a BE de ASKA.
4. **Buscar el middleware en el binario, no en la web**: `MessageConvertLib` no
   existe en Internet; su semántica (font→atlas+cmap) debe salir del `default.xex`
   (ya recompilado a C++ en este repo). Cruza `sub_826D43F8`/`sub_826D3268` con
   los opcodes de `RMF1`.
5. **Para traducir**: la vía del repo es correcta — directorio `id→offset` en el
   `.msg`, generar el byte-stream codificado y cubrir `á é í ó ú ü ñ ¿ ¡`
   ampliando el `AIF ` + tabla de métricas (o reutilizando glifos). El modelo de
   `vp2_glyphs.py` (atlas→PNG→reinyección) es el precedente público.

---

## 7. Apéndice — búsquedas negativas (para que conste)

- `"MessageConvertLib"` → 0 resultados (websearch y DuckDuckGo).
- `"MessageConvertLib_1.0.0.0"` → 0 resultados.
- `"Infinite Undiscovery" RMD message resource RMD- format tool` → 0 resultados.
- GitHub topics `tri-ace` / `aska-engine` → 4 repos, ninguno de texto/mensajes.
- Sin entrada de Infinite Undiscovery/Star Ocean 4/Resonance of Fate en
  Romhacking.net **de traducción** (solo discusión/general).
- Sin herramienta de texto para Star Ocean 4 (Nexus: solo QoL) ni RoF
  (ResHax/ZenHAX: solo containers/compresión).
- ReXGlue/XenonRecomp: sin utilidad de extracción/inyección de texto.
- Sin Discord público dedicado a ASKA/texto.

> **Nota metodológica:** *ausencia de evidencia no es evidencia de ausencia*; el
> foro XeNTaX original está caído (solo el backup
> [XeNTaXBackup](https://github.com/XeNTaXBackup/XeNTaXBackup.github.io) y
> ResHax/ZenHAX conservan parte). Aun así, ni el backup ni los foros vivos
> devuelven nada para los términos clave.

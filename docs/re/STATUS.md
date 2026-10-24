# Estado del proyecto (traspaso)

> Resumen operativo del estado. Documentación técnica en `docs/re/` (formato,
> códec, hook, longitudes, charmap, texturas). Plan maestro: `PLAN-TRADUCCION-…md`.

## Lo que YA funciona (verificado en el juego)

- **Texto**: los 71 386 mensajes de los 81 `RMD-` decodificados (`extract/messages/text/`),
  **charmap por banco** (`translation/charmaps/`), 100 % de glifos de la fuente principal.
- **Hook en runtime** (`mod/recomp/translation_es.h`, override de `sub_826D52C0`):
  sustituye el texto en vivo, identifica el banco por huella del `.msg`, inyecta en
  memoria *guest*, cachea por `(huella,id)`.
- **Pipeline**: `tools/build_es_codes.py` une `translation/es.json` + `translation/parts/*.json`,
  aplica **reflow** (`0x4000`/`0x4005`), **auto-fold de acentos** y genera
  `translation/es_catalog.bin`. `tools/validate_translation.py` valida longitud.
- **Traducción**: **6 568 cadenas** en 46 bancos (menús, sistema, objetos, nombres +
  diálogo de la historia principal), con glosario (`translation/glossary.md`).
- **Menús (banco `..._005`) YES se ven en castellano, sin recuadros** (p. ej. menú de
  Options completo: "Mensajes, Cám. vertical, Vol. música, Aceptar, Atrás…").

## Bugs resueltos (importantes)

1. **Ninja no recompilaba `main.cpp` al cambiar headers** → usar `mod/recomp/build.sh`
   (copia headers + `touch main.cpp`). Y **copiar el exe a `run/`** antes de ejecutar.
2. **`ctx.r3` en el override**: capturarlo ANTES de llamar al original (al volver es el retorno).
3. **`all_codes` truncado**: el decodificador plano no maneja payloads de control
   (`0x4007` con bytes `0x00`). La estructura del original se obtiene con
   `tools/decode_messages.py` (`MsgBank`). Saltos de línea: `0x4000` (menú) y `0x4005` (diálogo).
4. **Auto-fold de acentos**: usar dict con claves `str` (¡no `str.maketrans`!) → los
   acentos se pliegan a ASCII mientras no haya glifo. Elimina los recuadros blancos.
5. **Códigos fuera del atlas**: el charmap tenía añadidos offline (acentos 858+, de WS1)
   que el atlas del juego NO tiene → recuadros. Al codificar solo se usan códigos
   `<= 300+count` (los que existen).
6. **Assets**: un agente parcheó `ud1.bin` in-place y rompió el arranque; se **revirtió**.
   **No parchear los assets del juego in-place.**

## Lo que FALTA

### 1. Rótulos de menú = TEXTURAS — **INTEGRADAS por hook** (ver `docs/re/ui-textures.md`)
`docs/re/second-text-path.md`: no hay segunda ruta de texto; son bitmaps.

**Traducidas e integradas** (`tools/ui_textures.py` + `mod/recomp/ui_textures_es.h`):
- `atlas_ui_es.aif` (1 024²): pestañas del CAMP + rótulos de combate
  (`Ventaja aliada/enemiga`, `Subir nivel`, `OBJETIVO`, `GUARDIA`, `CRÍTICO`,
  `Bonif. situación`, `CONEXIÓN`), `Pausa`, `Cargando...` y fila de stats.
- `gameover_es.aif` (`FIN DEL JUEGO`), `nowloading_es.aif` (`Cargando...`),
  `reload_es.aif` (`Recargar.`).
- **Acentos en TODOS los bancos (WS1, integrado):** los 16 glifos
  (`á é í ó ú ü ñ Á É Í Ó Ú Ü Ñ ¿¡`) se añaden **por banco** con
  `tools/add_accents_banks.py` (bases propias + celdas libres; 10 bancos
  reutilizan celdas kana/kanji). **44/44 bancos** con cadenas acentuadas
  cubiertos (300 glifos). El hook `mod/recomp/accents_es.h` (incluido desde
  `translation_es.h`, llamado en `TrySubstitute`) parchea en memoria el atlas
  `RMD-` (`aif = msgbase - aif_size`) y las métricas del `.msg` desde
  `translation/accents_atlas.bin`; identifica el banco por firma barata +
  FNV-64, cachea por `msgbase` (barato). `build_es_codes.py` ya **no pliega** a
  ASCII los bancos con acentos. Verificación **offline** byte-exacta
  (`tools/verify_accents_banks.py`, 44/44 OK). Detalle:
  `docs/re/accents-banks.md`.
  > Sigue pendiente el problema **distinto** de glifos **latinos** ausentes en
  > algunos bancos (F3-b, §2): p. ej. `d2_ud1_68653000_037` no tiene `B`.
- **Menú principal** (`NewGame`, `Continue`, `OPTIONS`, `XboxLive`, `PressStart`…):
  **RESUELTO y VERIFICADO EN JUEGO**. No son `.aif` sueltos: son AIFs
  **incrustados** en `0001F800_000_MESH.asf` (`@0x2F51F0` = Easy/Normal/Hard/
  VeryHard/Options/New Game/Load Game/Xbox LIVE/PRESS START; `@0x3D21F0` =
  Infinity/Continue).  La clave: el escáner por firma llegaba **tarde** (la
  textura ya estaba subida).  Ahora se sustituyen **al cargar el ASF**, en
  `OnAif` enganchado a `sub_821EF618`/`sub_821EF678` (registro de cada chunk
  `AIF `), antes de crear la textura.  Ver `docs/re/ui-textures.md` §3.3.
  Captura: `extract/f2/run/title_test_shots/shot_9.png` («Nueva partida»,
  «Continuar», «Opciones», «Pulsa START»).
- **`TARGET` → `BLANCO`**: el motor dibuja la palabra **glifo a glifo** (seis
  sprites del atlas, tabla `000A4000_003_TTD.bin`), no una banda única.  Se pinta
  **una letra por sprite** → sin cortes.  `docs/re/ui-textures.md` §4b.
- **Layout del título**: ES **centrado** en la misma caja que el original y
  **tamaño armonizado global** por estilo (`Continuar` ya no destaca).
- **Pantallas de ayuda del CAMP** (1280×720 DXT1, **51 EN**): `tools/ui_help_screens.py`
  quita el texto horneado con *inpainting* (`cv2`) y pinta el castellano;
  **51/51 rótulos** + **335 rótulos** de panel (barras de ayuda, cabeceras,
  listas de opciones, cajas de diálogo). Integradas al manifiesto (58 texturas)
  por firma+FNV-64. Inventario sistemático con `tools/scan_aif_text.py`
  (sueltos + incrustados); ver `docs/re/ui-textures.md` §9.
- **Inventario de texturas CERRADO** (21 692 ficheros, 10 316 AIF, 4 291 con
  OCR): tras el triaje (`aif_text_report.py` + inspección visual) **no queda
  ninguna textura de UI nueva** por traducir; los 4 291 son falsos positivos
  (materiales/normal maps), atlas de glifos, logos y el aviso de copyright
  (descartado). Detalle en `docs/re/ui-textures.md` §9.1b.

**Integración:** hook `ui_textures_es.h` (incluido desde `translation_es.h`):
manifiesto `translation/ui_textures.txt` (firma 0x30 B + FNV-64) y sustitución en
memoria guest por **2 vías**: `OnMsg` (sustituye el `.msg` de métricas **y** el
atlas que le precede → acentos 005) y `Tick` (**un hilo** que escanea regiones
con `memchr`; nunca bloquea el hilo del juego). **NO** se parchean assets. Ciclo
completo en `mod/recomp/qa.sh`.
- **Cuelgue resuelto:** la 1.ª versión escaneaba con `VirtualQuery` por página
  (458 752 llamadas, ~17 s) **en el hilo del juego** → negro al arrancar. Ahora
  itera por regiones + `memchr` en un `std::thread` (~0.5 s, otro `tid`). Se
  eliminó la vía `OnAif`/`sub_821EF678` (no era el cargador de `AIF `).
- **Acentos 005:** el catálogo emite los códigos 858–873 (`build_es_codes.py`
  cambió `max_code` a `300+celdas` **si** existe el atlas ampliado); en runtime
  se sustituyen el atlas **y** el `.msg` (métricas; original sin parchear en
  `extract/f0/d2_..._005.msg`). 17 255 usos.
- **Rótulos:** encaje revisado (avance real por glifo, no ancho de tinta).
  `TARGET` ya no se corta: se dibuja **glifo a glifo** (6 sprites) y se pinta
  **una letra por sprite** (`BLANCO`); ver `docs/re/ui-textures.md` §4b.
  `OBJETIVO` (8 letras) **no cabe** en los 6 sprites (2 letras por caja salen
  pequeñas/recortadas) → se mantiene `BLANCO`.
- **Encabezados `~ <título> ~` del CAMP:** el título se centra con el **ancho del
  registro `.msg`** (calculado sobre el inglés); al ser `Equipo` más corto que
  `Equipment`, el `~` de cierre se alejaba.  El hook ahora **recalcula el ancho
  ES** (`FixWidth`, escalando por métricas) en `obj+0xC4` y en el registro; ver
  `docs/re/ui-textures.md` §4c.
- **`Pulsa START`:** caja de pintado = ink de `PRESS START` con centro 432
  (centro de pantalla ≈1280).

### 1b. Soporte multi-mando (iconos/imagen por mando) — **IMPLEMENTADO, offline OK**
`docs/re/controller.md`.  El mod detecta el mando y muestra **iconos de botón
(PS5: ✕○□△, L1/R1, L2/R2) e imagen del mapeo** acordes:
- **Detección** (`mod/recomp/controller_es.h`): CVAR `es_controller`
  (`auto|xbox|ps5|steam`, env `IU_ES_CONTROLLER`); en `auto` lee el `runtime.log`
  del runtime (línea `SDL OnControllerDeviceAdded … VendorID(0x…)`) y mapea
  `0x045E=xbox`, `0x054C=ps5`, `0x28DE=steam`.  Barato (≤1 lectura/500 ms),
  resuelve a `xbox` a los 12 s si no hay mando.
- **Iconos** (`tools/controller_skins.py`): el atlas `000A4000_002` (fila y≈639 +
  pila izquierda x=25,71,117,163,209,255) se reescribe para PS5; Steam usa los
  botones de Xbox.
- **Imagen** `26EF5000` (960×540 DXT5): se redibuja el mando (PS5/Steam)
  conservando las **líneas azules** y las **posiciones** (rótulos alineados);
  Xbox no se sustituye.
- **Integración**: manifiesto **IUT2** con columna de mando; el hook ordena las
  variantes antes que `*`, las difiere mientras no haya detección, y marca como
  hechas las del mismo origen.  Sin parchear assets.
- **Verificación offline**: `tools/verify_controller.py` (detección + manifiesto
  + tamaño/firma/decode de variantes) → `RESULTADO: OK`.
  Comparativas: `extract/f2/ui_es/controller_{maps,buttons}_cmp.png`.


### 2. Glifos latinos faltantes por banco (F3-b) — 421 ocurrencias
Cada `RMD-` tiene un atlas con solo los glifos de su inglés; a algunos bancos de
**diálogo** les faltan letras (Q, V, W, q…). **Cómo**: extender el atlas de cada banco
copiando el bitmap de `..._005` (que es superset) + acentos, y actualizar `count`/métricas;
integrarlo por **hook en runtime** (los chunks están comprimidos).

### 3. Ajuste fino de longitudes (4 167 avisos, muchos triviales)
Muchos son etiquetas diminutas (`a`→`una`, `%`); los reales son cadenas de diálogo
con más líneas que el original. **Cómo**: abreviar y aceptar `--strict` en
`tools/validate_translation.py` como puerta de calidad.

### 4. Traducción restante
Descripciones de objeto `300xxx`–`500xxx`, títulos `601xxx`–`675xxx`, BGM/eventos;
y repasar los `dialog_*` con avisos de anchura.

## F8 — Opciones del mod dentro del recomp (hecho)

- **Sin segundo ejecutable.** Las opciones del mod viven en la UI del propio
  recomp:
  - **Asset Setup Wizard** (configuración inicial) y
  - **Content Profile Manager** (relanzar y cambiar parámetros: `--profile_manager`,
    o **F10** / acción del overlay Community Debug en juego).
  Ambos muestran un panel **«Traducción:»** con los conmutadores
  **Texto en castellano** / **Texturas en castellano**.
- Persistencia en `run/setup.json` y `run/<PERFIL>/config.json`
  (`es_translation`, `es_textures`, `es_selftest`), leídos en `Initialize()`
  y consumidos por el hook vía `iu::portable::EsTranslation()/EsTextures()`.
- **CVARs del mod** (`mod/recomp/translation_cvars.h`, categoría *Translation*):
  `es_translation`, `es_textures`, `es_selftest`, `es_catalog`,
  `es_textures_manifest`, `es_controller` (auto|xbox|ps5|steam). Aparecen en el
  overlay **Community Debug** y en la CLI.
- Prioridad `CVAR > entorno > asistente (setup.json) > defecto`: los scripts con
  `IU_ES=…` siguen valiendo.
- Ver `docs/re/launcher.md`.

## Comandos

```bash
# pipeline + catálogo
python tools/validate_translation.py
python tools/build_es_codes.py
# compilar + ejecutar + capturar
bash mod/recomp/build.sh
bash mod/recomp/qa.sh            # build + catalogo + run + captura
# (el catalogo debe estar en extract/f2/run/translation/es_catalog.bin)
```

## Entorno
- Recomp en `extract/f2/recomp` (build: `mod/recomp/build.sh`).
- Assets legales del usuario en `extract/f2/run/EUROPE/assets/disc1`.
- El juego se ejecuta desde `extract/f2/run` (el hook busca `translation/es_catalog.bin`
  relativo al cwd; **copiar el catálogo allí** o definir `IU_ES_CATALOG`).
- **NO versionar assets del juego** (`extract/`, `*.bin`, `*.iso`… ya en `.gitignore`).

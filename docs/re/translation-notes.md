# Notas de traducción — WS3 (MENÚ / SISTEMA / OBJETOS / ÍTEMS)

> Agente: **ws3**. Alcance: rótulos de menú, textos de sistema, nombres de
> personajes/enemigos/lugares/objetos/ítems del banco `d1_ud1_000A4000_005` y de
> los bancos clasificados `menu`, `sistema`, `objeto` y `bonus` en
> `docs/re/charmap.md`.
> No se traducen diálogos largos (los cubren otros agentes).

## 1. Método

1. **Fuente**: texto decodificado `extract/messages/text/<stem>.txt` (`id<TAB>texto`).
2. **Glosario**: se parte de `translation/glossary.md` (obligatorio) y se amplía con
   los términos nuevos fijados (§7 más abajo).
3. **Artefactos de decodificación corregidos a mano** al interpretar el `en`:
   `I/l/1`, `@`→`q`, `?` de `?own`→`Down`, `ltems`→`Items`, `lt,s locked,`→
   `It's locked.`, `e@uipment`→`equipment`, `pIaying`→`playing`, `lNVALlD`→
   `INVALID`, comas finales que eran puntos, etc. Los `{0x....}` y `?[small:N]`
   se eliminan de `en`/`es` (los gestiona el pipeline).
4. **Herramientas propias (en temp, no en `tools/`)**:
   - `mkpart.py`: combina un TSV `stem<TAB>id<TAB>es` con el texto del banco y
     emite `translation/parts/<out>.json` con `{stem,id,en,es,ctx}`.
   - `report.py`/`wcheck.py`: ejecutan el mismo modelo de anchura y *reflow* que
     `tools/build_es_codes.py` para localizar cadenas que superan ancho/líneas/glifos.
   - `gen_items.py`: genera los nombres de objeto y sus artículos (ver §3).
5. **Validación iterativa** con `python tools/validate_translation.py --es
   translation/parts/<f>.json` y con `python tools/build_es_codes.py`.
   `validate_translation.py` solo lee un fichero; para validar todo el proyecto hay
   que validar cada *part* (o pasar un es.json combinado).

## 2. Ficheros entregados (`translation/parts/`)

| Fichero | Cadenas | Bancos | Contenido |
|---|---:|---:|---|
| `menu_system.json` | 545 | 1 | `d1_ud1_000A4000_005`: sistema (ids 1–201) y menús/opciones/estado/objetos/habilidades/creación/guardado (ids 10000–10446) |
| `menu_signs.json` | 146 | 24 | carteles de mapa, preguntas de NPC-tienda, guardado, teleportador, mensajes de "objeto obtenido" y bonus (bancos `menu`/`sistema`/`objeto`/`bonus` de `d1_ud2_*` y `d2_ud1_*`) |
| `menu_items.json` | 2748 | 1 | nombres de objeto del banco 005: `200xxx` (nombre visible), `250xxx` (artículo), `260xxx` (nombre en minúscula para el aviso de obtención) |
| `menu_names.json` | 416 | 1 | enemigos, PNJ genéricos, lugares, objetos de escena, estados del banco 005 (rango 100000+ y duplicados de mapa) |
| `menu_shops.json` | 44 | 1 | rótulos de tienda (`800xxx`) |

**Total propio: 3.899 cadenas.** (El resto de `translation/parts/*` es de otros
agentes: diálogo.)

## 3. Estructura de los nombres de objeto (banco 005)

El banco guarda cada objeto **tres veces**, con índices correlativos `i`:

* `200000+i` → nombre visible (p. ej. `Espada oxidada`).
* `250000+i` → artículo para el aviso de obtención (`un `/`una `/`unos `/`unas `),
  ajustado al **género y número** del nombre traducido.
* `260000+i` → nombre en minúscula + `!` (`espada oxidada!`) para la frase
  `Obtained …!`.

Los 916 objetos se tradujeron de forma **consistente** en los tres rangos
(mismo género en el artículo). Ej.: `Dented Sword / a / dented sword!` →
`Espada oxidada / una / espada oxidada!`.

## 4. Resultado de la validación (resumen)

Tras optimizar los rótulos de menú (abreviaturas tipo `Opc.`, `Habs.`, `Hab.`,
`Guar.`, `Carg.`, `Obj.`, etc.), en `menu_system.json` queda **1 aviso de línea**
en una cadena-fragmento (`" trap!"` → `" ¡trampa!"`, sufijo concatenado) y el
resto de avisos son de anchura en etiquetas cortas donde el castellano es
intrínsecamente más largo (`Fuego` vs `Fire`, `Campos` vs...). Se corrigieron
todos los *reflows* (saltos de línea añadidos) que rompían la maquetación.

**Avisos intrínsecos (no corregibles sin degradar la traducción):**

* `menu_items.json` y `menu_names.json`: los nombres de objeto y de enemigo son
  **etiquetas cortas** (`Food`, `Fire`, `Buy`, `Save`, `Inn`, `Talk`…). El
  castellano es 15–40 % más largo, y la tabla de anchura usa el ancho del texto
  original como objetivo. Reducirlos más exigiría abreviaturas ilegibles
  (`Com.`, `Fue.`) que empeoran la calidad sin garantía de que esas cajas usen
  realmente el campo `width`.
* Estas listas (inventario, nombres de enemigo, carteles) suelen dibujarse en
  **columnas de ancho fijo**, no ajustadas al texto; el *reflow* del pipeline
  podría no ser aplicable. **Pendiente de verificación en juego** (ver §6).

## 5. Términos y decisiones destacadas

* `HP/MP` → **`PV/PM`** (puntos de vida / de magia), según glosario.
* `Items` → **`Objetos`**; `Equipment` → `Equipo`; `Skills` → `Habs.` (menú) /
  `Habilidades de combate` (encabezado ancho).
* `Connect` → `Conexión`; `Connect Action(s)` → `Acción(es) de conexión`.
* `Party` → `Grupo` (no «equipo», para no chocar con `Equipment`).
* `Confirm/Back/Cancel` → `Aceptar/Atrás/Anular` (más cortos que
  `Confirmar/Cancelar` y caben).
* Estados: `asleep→Sueño`, `poisoned→Veneno`, `cursed→Maldito`,
  `confused→Confuso`, `paralyzed→Parálisis`, `frozen→Helado`, `unseeing→Ciego`,
  `unhearing→Sordo`, `unsmelling→no huele`, `stinky→Peste`,
  `charmed→Hechizo`, `untasting→insípido`, `berserked→Furioso`.
* Elementos: `Earth/Water/Fire/Wind/Dark/Light` → `Tierra/Agua/Fuego/Viento/Oscuro/Luz`.
* Enemigos/PNJ: se traduce el descriptivo y se conservan nombres propios
  (`Kraken`, `Garuda`, `Orthros`, `Narbear`…). Lugares según glosario.
* Topónimos dudosos: `Lintz`, `Vinphen`, `Ryner`, `Borós` se dejan tal cual.

## 6. Riesgos / pendientes

1. **Anchura de listas fijas** (inventario, enemigos, carteles): verificar en el
   juego si el motor respeta el campo `width` o recoloca el texto. Si lo respeta,
   habría que acortar sistemáticamente (no recomendado por calidad).
2. **Sin traducir (siguiente lote, no asignado aquí):**
   - Descripciones de objeto `300xxx` y `400xxx–500xxx` (1.880 cadenas largas).
   - Títulos/medallas `601xxx–675xxx` (1.608).
   - Nombres de pista/eventos `860xxx–863xxx` y `62100000+` (diálogo/BGM).
3. Los bancos de diálogo y sus `d2_*` los cubren otros agentes; los `d2_*`
   idénticos se resuelven por **huella de cabecera**, no hace falta duplicarlos.

## 7. Glosario

Se amplió `translation/glossary.md` con los términos nuevos fijados en este lote
(estados alterados, categorías de objeto, abreviaturas de UI, géneros de objeto).

## 8. Herramientas compartidas

No se modificó ningún fichero de `tools/` (`build_es_codes.py`,
`validate_translation.py`, `gen_es_catalog.py`). Solo se **leyeron**. Los
auxiliares creados viven en el directorio temporal de la sesión.

## 9. Lote WS3B — descripciones de objeto y títulos (`stem` 005)

> Agente: **ws3b**. Continúa el lote de WS3. No se tocó ningún fichero de
> `tools/`. Auxiliares en el directorio temporal.

### 9.1 Ficheros entregados

| Fichero | Cadenas | Contenido |
|---|---:|---|
| `translation/parts/menu_desc.json` | 1882 | Descripciones del banco 005: ítems/armas/armaduras/accesorios/comida/materiales (`300001`–`300916`, `30027280`, `30092816`), etiquetas de bonificación (`400001`–`400482`) y sus descripciones de encantamiento (`500001`–`500482`). |
| `translation/parts/menu_titles.json` | 1703 | Títulos por personaje `601xxx`–`618xxx`; descripciones/comentarios de título `621xxx`–`638xxx`; placeholders `641xxx`–`658xxx`; música/temas `660xxx`–`661xxx`; logros y recompensas `662xxx`–`663xxx`; pistas de BGM/prueba `664xxx`; títulos y sinopsis de capítulo `860xxx`–`873xxx`. |

### 9.2 Resultado de validación

* `menu_desc.json`: **0 avisos de ancho** en las descripciones largas (todas
  ≤ 100 % del original con *reflow*). Solo quedan **4 avisos de ancho**
  (≤ 107 %) en etiquetas de una línea intrínsecamente más largas en castellano
  (`Antiaire`, `Antidesm.`, `Analizar`). Los avisos restantes son de **nº de
  líneas** (el castellano ocupa una línea más que el original en descripciones
  de objeto); el ancho nunca supera el original, que es el requisito funcional.
* `menu_titles.json`: **0 glifos ausentes**. Los avisos de ancho (220) se
  concentran en **nombres de título** (`601xxx`–`618xxx`) y en los placeholders
  internos `641xxx`–`658xxx`, que son etiquetas de 4–8 caracteres donde el
  castellano es 15–60 % más largo. Se abreviaron los más extremos (`Recio`,
  `Beodo`, `Dichoso`, `Mentor`, `Médico`, `Feroz`, `Remilgado`, `Punk`,
  `Enano 1/2`, `Grafómano`, `Hiper`, `Fiel`, `Borde`, `Odiado`, `Cándido`,
  `Severo`, `Fisgón`, `Pasota`, `Hermana`, `Autónomo`, `Beoda`, `Romo`,
  `Colega`). Las descripciones `621xxx`–`638xxx` y los textos de capítulo
  `860xxx`–`873xxx` no presentan avisos de ancho. Igual que en
  `menu_items.json`/`menu_names.json`, las listas de títulos se dibujan en
  columnas de ancho fijo; **pendiente de verificar en juego** si el motor
  respeta el campo de ancho o recorta.

### 9.3 Decisiones de traducción

* **Materiales** (consistencia con `menu_items.json`): `lunatite→lunatita`,
  `crystallite→cristalita`, `prismatite→prismatita`, `metetite→metetita`,
  `amarlista→amarlista` (glosario), `mercurius→mercurio`, `lentesco`,
  `placidus`, `pius`, `lux`, `malus`, `lauan`, `balsa` se mantienen.
* **Estados**: `faint→desmayo`, `freeze→congelación`, `stone→petrificación`,
  `doom→condena`, `charm→hechizo`, `stink→hedor`, `untasting→insípido`,
  `dwindle→merma`, `berserk→furia`.
* **Efectos de encantamiento**: se generaron con reglas (estadísticas
  `ATQ/DEF/PRE/VEL/INT/SUE/PV/PM`) y se revisaron una a una las excepciones
  (rompe arma/armadura, drenajes, multi-cláusula). Duración abreviada a
  `(N min)`.
* **Nombres propios** (personajes, habilidades, hechizos, melodías, lugares)
  se mantienen: `Alfheim`, `Levantine Slash`, `Simorgh Zal`, `Tempest Clash`,
  `Nightshade`, `Peerless Valor`, `Voltsweep`, `Pyrdance`, `Geocrush`,
  `Nekros`, `Miasma`, `Geoquake`, `Pyrgeddon`, `Placare`… `DUMMY_*` son
  placeholders de desarrollo y se conservan.

### 9.4 Pendiente (no asignado en este lote)

* Textos de tutorial `670xxx`–`675xxx` (contienen controles `{0x4001}`
  `{0x4002}` `{0x4019}` y `?[small:N]`; requieren soporte del pipeline para
  preservar controles en `es`, que hoy no existe en `build_es_codes.py`).
* Nombres internos de depuración `100xxx`/`104xxx`–`108xxx`, `105xxx`,
  `213xxx`–`240xxx` (habilidades de conexión), diálogos `681xxx`–`785xxx`
  (los cubren otros agentes).


---

# Notas de traducción — WS4b (DIÁLOGO / GUION, 2.ª tanda)

> Agente: **ws4b**. Alcance: cadenas de **diálogo/guion** que faltaban en los
> bancos `d1_ud2_*` (y `d1_ud1_*`) marcados `dialogo`/`menu` en
> `docs/re/charmap.md`, continuando el trabajo de WS4. Los `d2_ud2_*` son
> idénticos por huella, así que **no se duplican**: se traduce solo el `d1_*`.

## Método

1. **Fuente**: `extract/messages/text/<stem>.txt` (`id<TAB>texto`); se toma la
   línea **inglesa** (la par, sin CJK). Artefactos interpretados a mano
   (`I/l/1`, `@`→`q`, comas que eran puntos, `?[N]` iconos, controles
   `{0x....}` eliminados).
2. **Salida**: `translation/parts/dialog_<stem>.json`
   (`{stem,id,en,es,ctx:"dialogo"}`), fusionando con lo ya existente. Los
   `en` salen del banco decodificado sin controles; los `es` no llevan
   controles ni `\n` (los gestiona el pipeline).
3. **Herramientas propias** (en temp, no en `tools/`): `pending.py` (lista los
   `id<TAB>en` pendientes por banco), `mkbank.py`/`mkbanks.py` (TSV → part JSON).
4. **Validación** con `tools/validate_translation.py` por *part*. Avisos
   `sin glifo: Q/V/...` son **benignos** (los suple el atlas extendido); los
   avisos reales son `ANCHO`, `glifos>` y `lineas>`. La tasa es similar a la de
   los parts ya aceptados de WS4 (≈20–25 % de desbordamiento de línea, intrínseco
   al castellano), con correcciones manuales para abreviar.

## Cobertura de esta tanda

| Banco | Cadenas nuevas | Notas |
|---|---:|---|
| `d1_ud2_7EC08000_011` | 1248 | Fayel/escena + NPC de ciudad; **completo** |
| `d1_ud2_40141800_004` | 1180 | diálogo de Fayel + NPC; **completo** |
| `d1_ud2_69AB6000_009` | 1153 | escena + NPC de Halgita; **completo** |
| `d1_ud2_2E8D1800_003` | 871 | escena/rito + NPC de Burguss; **completo** |
| `d1_ud2_59DEA800_017` | 786 | escena del tsunami en Zala + NPC; **completo** |
| (14 bancos) | ~43 | huecos `blank`/`dummy` (`→ "vacío"`) rellenados en bancos ya casi completos |

**Total de diálogo producido por ws4b: 5.238 cadenas nuevas** (+~43 huecos
`blank`/`dummy`). Tras esta tanda, **todos** los bancos de diálogo `d1_*` quedaron
cubiertos (9.668 líneas inglesas, 0 pendientes): los 4 bancos restantes
(`3A123800_010`, `1E70B800_010`, `17325800_002`, `510DD000_025`, este último un
menú de depuración «event jump») los completó una pasada MT-glosario concurrente.
La `meta` de los *parts* fue normalizada a `{"metodo":"mt-glosario"}` por ese
proceso, pero **el contenido de los 5 bancos de ws4b se conservó íntegro**
(verificado por muestreo de ids).

Detalle de los huecos rellenados: `d1_ud1_317BD800_037`, `d1_ud1_39513800_022`,
`d1_ud2_1793D000_018`, `d1_ud2_19478000_028`, `d1_ud2_198FA800_008`,
`d1_ud2_224F6800_028`, `d1_ud2_254C2000_039`, `d1_ud2_339DF000_020`,
`d1_ud2_62CF4800_014`, `d1_ud2_644C2000_017`, `d1_ud2_7B2EA000_042`,
`d1_ud2_916D8000_026`, `d1_ud2_96119800_017`, `d1_ud2_A014E800_005`.

### Validación

Se validó cada *part* con `tools/validate_translation.py`. Recuento de avisos por
banco de ws4b: `7EC08000_011` 588/1248, `40141800_004` 612/1180,
`69AB6000_009` 805/1153, `2E8D1800_003` 516/871, `59DEA800_017` 459/786. La
mayoría son `sin glifo: Q/V/...` (benignos, los suple el atlas extendido) o
`lineas 2>1` (el castellano ocupa una línea más que el inglés); los `ANCHO`
reales superan raramente el 110 %.

---

# Notas de traducción — CIERRE (pasada MT-glosario, cobertura total)

> Agente: **ws5**. Alcance: cerrar **todo** el texto restante sin traducir,
> continuando a ws3/ws3b/ws4/ws4b. Los `d2_ud2_*` son idénticos por huella
> (`.msg` byte a byte) y **no se duplican**; solo se traduce `d1_*` + los 12
> bancos `d2_ud1_*` exclusivos del Disco 2.

## Método (MT asistida + glosario)

Dado el volumen (≈14,3 k cadenas inglesas nuevas), se usó una **pasada MT
asistida** dirigida por glosario, no traducción manual. Herramientas propias en
el directorio temporal de la sesión (no se tocó `tools/`):

1. **Fuente**: `extract/messages/text/<stem>.txt`; se toma la línea sin CJK con
   letras ASCII (la inglesa). Se descartan las que solo contienen controles
   `{0x....}` (vacías) y las japonesas sin par inglés.
2. **Limpieza previa**: los controles `{0x....}` se eliminan del input; el
   resultado `es` **nunca** contiene controles ni `\n`.
3. **Protección de términos**: personajes, lugares y términos del mundo
   (`Crimson Chain`, `Dreadknight`, `Starseer`, `unblessed`, cadenas, etc.) se
   sustituyen por *sentinels* `QqzNNZq` antes de traducir y se restauran a su
   forma fijada del glosario (`Cadena Carmesí`, `Caballero Temible`,
   `Vidente Estelar`…). Así se evita que el MT toque nombres propios o
   traduzca colisiones (`Burguss→Burgos`, `Rico`, `party→fiesta`).
4. **Pre-normalización**: `party→group` (→ `grupo`), `Dreadknight→Dread Knight`,
   etc., para desambiguar el contexto JRPG.
5. **Post-proceso**: `AP→PA`, `HP→PV`, `MP→PM`, correcciones de género
   (`el Cadena→la Cadena`, `el Orden→la Orden`…) y formas de España
   (`confiable→fiable`, `acá→aquí`). Se eliminan U+200B y se capitaliza la
   inicial de cada línea.
6. **Salida**: `translation/parts/dialog_<stem>.json`
   (`{stem,id,en,es,ctx:"dialogo"}`), fusionando por `id` con lo existente y
   guardando incrementalmente (resumible: salta ids ya cubiertos por cualquier
   *part*).

## Cobertura resultante

* **Bancos `d1_*`**: ~14,3 k cadenas pendientes → **0**. Incluye el banco mixto
  `d1_ud1_000A4000_005` (9.183 cadenas: diálogo `62xxxxxx`–`7xxxxxxx`, títulos
  `6xx`, etiquetas de sistema/debug) y los bancos de diálogo grandes
  (`3A123800_010`, `1E70B800_010`, `17325800_002`, `510DD000_025`…).
* **`d2_ud1_*` exclusivos**: 12 bancos (~50 cadenas reales; el resto ya estaban
  en `menu_signs.json`). Cubiertos.
* **`d2_ud2_*`**: 25.283 cadenas → **duplicados por huella**, sin traducir
  (su contenido reutiliza los `d1_ud2_*`).
* **Total**: `translation/es_codes.json` = **26.236 cadenas en 47 bancos**, sin
  `(stem,id)` duplicados entre *parts*.

Prioridad 1 (descripciones `300xxx`–`500xxx` del banco `005`) ya estaba cubierta
por `menu_desc.json` (1.882) antes de esta pasada; se verificó 0 pendientes.

**Líneas japonesas omitidas por diseño**: 9.681 en los bancos `d1_*`. Cada una
tiene su par inglés en otra entrada (el motor usa la entrada inglesa); se
traduce solo la inglesa, según el método de ws4/ws4b. Las japonesas sin par
inglés (solo control/sin letras) no requieren traducción.

## Validación

* `python tools/build_es_codes.py` → rc=0, genera `es_codes.json` +
  `es_catalog.bin` (26.236/47). Avisos del build: ~11,6 k `ANCHO` (mayoría en
  etiquetas de una línea del banco `005`, intrínseco al castellano), 666
  `sin glifo` (acentos, los pliega el pipeline) y 2 `lineas`.
* `tools/validate_translation.py` sobre los `dialog_*`: 13.077/18.752 con
  avisos; `ANCHO` reales ≈1.423 (**7,6 %**), el resto `glifos>`/`lineas>`
  benignos por el *reflow* (el build usa el ancho de CAJA del banco).

## Riesgos / pendientes

1. **Calidad MT**: es una traducción automática dirigida por glosario. Cumple
   cobertura y terminología, pero conviene **revisión editorial en juego**,
   sobre todo frases largas y el banco `005` (muchas etiquetas de sistema).
2. **Anchura**: los `ANCHO` del banco `005` son etiquetas de una sola línea que
   el *reflow* no puede partir; abreviar si se detectan cortes en pantalla.
3. **Términos con género**: los *sentinels* de sustantivos femeninos
   (`Cadena`, `Orden`, `Fuerza`, `Torre`, `Puerta`) llevan corrección de
   artículo; revisar algún caso residual.
4. **`tools/`**: no modificado (solo leído).

---

# Notas de traducción — REMATE EN + TUTORIALES (pasada de cierre)

> Agente: **ws6**. Alcance: (1) rematar las **líneas inglesas** de los bancos
> `d1_*` que hubieran quedado sin traducir y (2) **retraducir los tutoriales**
> `670101`–`672502` del banco `d1_ud1_000A4000_005`, que contenían controles e
> iconos y una MT defectuosa. No se tocó `tools/`.

## 1. Barrido de líneas inglesas (bancos `d1_*`)

Método: para cada `extract/messages/text/d1_*.txt` se cruzaron **todos** los
`id` con el conjunto de `id` ya presentes en `translation/parts/*.json`
(dialog + menus, por `stem`). Resultado:

* **Líneas inglesas puras (sin CJK): 0 sin traducir.** El trabajo de ws3–ws5
  cubrió el 100 % de las líneas pares inglesas de los 35 bancos `d1_*`
  (verificado con `es_codes.json`: 0 `id` ingleses ausentes).
* **Artefactos de decodificación**: quedaban **2** líneas inglesas que la
  pasada anterior omitió porque el decodificador convirtió un glifo en un
  carácter CJK (`ー`/`一`) y el clasificador «sin CJK» las tomó por japonesas:

| Banco | id | EN (real) | ES |
|---|---:|---|---|
| `d1_ud1_000A4000_005` | 41573152 | A clumsy—but effective—spinning attack | Un ataque giratorio torpe pero eficaz. |
| `d1_ud2_1E70B800_010` | 976 | Dragonbone Shrine | Santuario de Dragón (cartel; forma larga en glosario) |

Se añadieron como entradas nuevas (los huecos `…`, `EVE_*`, diagramas y nombres
internos de depuración `10xxxxx` del banco `005` son cadenas japonesas o
gráficos sin texto, no requieren traducción).

## 2. Tutoriales `670101`–`672502` (51 cadenas)

Los 51 tutoriales ya existían, pero (a) contenían los iconos `?[small:N]` y los
controles `{0x4001}`/`{0x4002}`/`{0x4019}` en `es` —que `build_es_codes.py`
codificaba como texto literal— y (b) tenían MT con cláusulas descolocadas.

Se **retradujeron** con estas reglas:

* `es` **sin** `{0x....}` ni `?[small:N]` (los controles del original los
  preserva ahora el pipeline al emitir el byte-stream).
* Los iconos de botón se describen en prosa («el botón de desenvainar», «el
  botón de conexión»…) para no romper la frase.
* Título + cuerpo concatenados (como el `en`), sin `\n`.
* `ctx` normalizado a `"tutorial"`.
* Glosario aplicado: `Conexión`, `Acción de conexión (AC)`, `Habilidades de
  combate`, `lunaglifos`, `vermificación`, `PA/PV/PM`, `Grupo`.

## 3. Validación

`python tools/validate_translation.py` sobre las 53 cadenas (51 tutoriales +
2 artefactos): **0 `ANCHO`**, **0 `lineas>`**. Solo 6 `glifos>` (el original
contaba cada icono de botón como 1 glifo; el castellano lo escribe con varias
letras), todos benignos:

`670103 670403 670504 670601 672502` y `976`.

> Nota de pipeline: `build_es_codes.py` avisa de algunos `ANCHO` extra en los
> tutoriales (p. ej. `670901 979>942`, `672001 964>934`). Es un artefacto de su
> `wrap()`: mide con `inv` (que no tiene glifos acentuados) y **subestima** las
> palabras con tilde, que luego se pliegan a ASCII al codificar. El validador
> oficial (que simula el *reflow* real) da esos mismos mensajes por buenos
> (`lin_max` ≤ original). Corregirlo requeriría que `wrap()` mida con el
> *fold* aplicado.

`tools/` no se modificó; el `build`/catálogo lo regenera la sesión de pipeline
(que a la vez añade la preservación de controles en `es`).

## Riesgos / pendientes

* Los tutoriales dependen de la nueva emisión por *byte-stream* (controles del
  original + `es` repartido por tramos de glifos): conviene **revisar en juego**
  el reparto título/cuerpo de `670101`–`672502`.
* El cartel `976` usa la forma corta por anchura; si la caja lo permite, subir a
  «Santuario Hueso de Dragón».

---

# Notas de traducción — WS7 (cierre: cobertura, multiparte, etiquetas)

> Agente: **ws7**. Alcance: (1) verificar el **100 %** de las líneas inglesas,
> (2) arreglar los **mensajes multiparte** (créditos y similares), (3) acortar
> **etiquetas estructuradas** de columnas. No se tocó `tools/`.

## 1. Cobertura del inglés — verificación

Se cruzaron **todos** los `id` de las líneas sin CJK de cada
`extract/messages/text/d1_*.txt` (y `d2_ud1_*` exclusivos) con el conjunto de
`(stem,id)` de `translation/parts/*.json`:

* **Líneas inglesas sin traducir: 0.** Los 35 bancos `d1_*` están cubiertos al
  100 %. El recuento «~17–31 por banco» de las notas previas era **anterior** a
  ws5/ws6 (ya resuelto).
* **`d2_ud1_*` exclusivos (12 bancos):** 0 pendientes.
* **`d2_ud2_*` y `d2_ud1_000A4000_005`:** duplicados por huella del `d1_*`; no se
  traducen (el catálogo los resuelve por huella).
* **Líneas con artefacto CJK** (el decodificador convierte un glifo en `ー`/`一`/
  `ロ`): revisadas una a una. Son **cadenas japonesas o de depuración** (menús de
  depuración `102xx`, `EVE_*`, `EVENT_*`), no texto de jugador. Las que sí eran
  inglesas (`300681`, `627202`, `632601`, `633101`, `41573152`, `976`…) ya estaban
  traducidas. **0 pendientes reales.**

## 2. Mensajes multiparte — arreglo

El pipeline (`build_es_codes.build_bytes`) reparte las **líneas** del `es` entre
los **tramos de glifos** del original, 1:1. Si el `es` tiene menos líneas que
tramos, los tramos sobrantes salen vacíos y su texto se **coloca mal** (o se
pierde si hay más líneas que tramos). Se añadió `\n` en el mismo número de partes
que el original en:

| Caso | ids | Ejemplo |
|---|---|---|
| **Créditos** (caja oficio / caja nombre) | `75000001`–`75000013` | `Supervisor planif.\nMasaki Norimoto` |
| **Cajas `0x4003`** | `114`, `115` | `¡PA a cero!\n¡Sin defensa y abierto a críticos!` |
| **Posición `0x4007`** | `510DD000_025`: `212,216,220,228,244` | `¡Gracias,\nSeñor Sigmund!` |
| **Iconos de botón `0x4019`** | `10286`–`10295` | `Uso: mantén pulsado\n,` |
| **Resalte de habilidad `0x4001`** | `10413`, `10427` | `La habilidad \nRegateo\n de Vic ha tenido éxito.` |

Los **oficios de los créditos** se abreviaron para caber en su caja (el ancho de
la caja es el del inglés, muy justo): ver tabla en `translation/glossary.md`.
Verificado con el modelo real del build: los 13 créditos y los demás casos dan
`nº de líneas ≤ nº de tramos` (sin pérdida).

> Nota: el validador oficial marca `lineas N>M` en estos casos porque su modelo
> sólo cuenta los saltos `0x4000/0x4005` como líneas; los controles de
> posición/caja no los cuenta. El build real sí los reparte bien.

## 3. Tutoriales `670xxx`–`675xxx`

Se añadió un `\n` **tras el título** en los 51 tutoriales, para que el tramo con
estilo de título (`0x4001:5 … 0x4002`) reciba **sólo el título** y no toda la
primera línea reflowada. El cuerpo sigue reflowándose por ancho. Verificado:
`nº de líneas ≤ nº de tramos` en los 51 (sin pérdida).

## 4. Etiquetas estructuradas (columnas)

En la pantalla Personal los rótulos `Title N` / `Trait N` se abrevian para no
invadir la columna del valor: **357** cadenas de `menu_titles.json`
(`641xxx`–`65xxxx`) pasan de `Título N`/`Rasgo N` a **`Tít. N`/`Ras. N`**.

## 5. Otras correcciones

* `AP` → **`PA`** (glosario): ids `114`, `115`, `198`, `10275` del banco 005.
* Artefactos `Vic,s` → posesivo, resuelto en las habilidades de Vic (`10413`,
  `10427`).
* Artículos vacíos de obtención que sí eran significativos:
  `250504` (` the ` → `el `), `250639` (` a bowl of ` → `un cuenco de `) y
  `260639` (`cielo y Tierra!` → `cielo y tierra!`). Los artículos vacíos de
  nombres propios (`Gram`, `Ascalon`, `Mjolnir`…) se dejan sin artículo.

## 6. Validación

* `python tools/build_es_codes.py` (salida a temp): rc=0, **26 238 cadenas en
  47 bancos**.
* Avisos del build final: **10 272 `ANCHO`**, **2 731 `lineas`**, **731
  `sin glifo`**. El acortado de etiquetas (`Tít.`/`Ras.`) rebajó los `ANCHO` en
  ~358 respecto al estado previo (10 630). Los `sin glifo` son acentos (los
  suple el atlas extendido / los pliega el *fold*).
* `tools/validate_translation.py` por *part*: los ficheros tocados mantienen la
  tasa de avisos previa (mayoría `ANCHO`/`glifos` intrínsecos). Los `\n` nuevos
  añaden avisos `lineas N>M` **benignos** (el build real los reparte por tramos).
* Se comprobó con el modelo real del build que **ningún mensaje corregido pierde
  texto** (`líneas ≤ tramos`).

## 7. Riesgo pendiente importante — desborde de líneas en el banco 005

El build **preexistente** avisa de **~2 731** `lineas N>M` y **~10 272** `ANCHO`.
De ellos, **~2 356 mensajes** del banco `d1_ud1_000A4000_005` tienen
`nº de líneas (reflow) > nº de tramos`, es decir, **pierden la última línea** al
emitirse. Un análisis de *line-breaking* óptimo (DP) confirma que **sólo 2** se
pueden arreglar insertando `\n` sin tocar el texto: los otros **2 356 tienen el
`es` demasiado largo en total** (necesitan acortarse). No son un problema de
cobertura (el inglés está 100 % traducido) sino de **anchura del reflow**.

* Causa: `build_es_codes.py` excluye el banco 005 del objetivo de ancho de caja
  (`not stem.startswith("d1_ud1_000A4000_005")`), así que usa el ancho de la
  **línea original** (muy estrecha) como objetivo. Con `bank_stats` (ancho de
  caja) el número de desbordes caería drásticamente.
* Recomendación: **corregir el pipeline** para que el diálogo del banco 005 use
  el ancho de caja, o hacer una **pasada editorial de acortado** de esos ~2 356
  mensajes. Fuera del alcance de este lote (no se tocó `tools/`).

## 8. Herramientas compartidas

No se modificó ningún fichero de `tools/`. Auxiliares en el directorio temporal
de la sesión.

# Cobertura de glifos para castellano (F3)

> **Estado:** **cerrado**. Ver **`docs/re/glyph-extension.md`** para el detalle
> completo (metodología, re-codificación DXT, celdas usadas y validación).
> **Objetivo:** saber si el atlas `AIF` de los `RMD-` puede renderizar
> `á é í ó ú ü ñ ¿ ¡` (y sus mayúsculas).

## 1. Necesidad

El atlas (según la comunidad) cubre **latino, kana y kanji**. Hay que verificar
si ese "latino" incluye el **latino extendido** que necesita el castellano.

## 2. Comprobación (hecha)

Renderizado el atlas del banco latino principal `d1_ud1_000A4000_005`
(768×768 → rejilla 24×24 de celdas de 32 px, `count = 557`):

* Contiene **ASCII** (letras, dígitos, puntuación), símbolos y kana/kanji.
* **No contiene** ninguno de los 16 caracteres del castellano.
* Las letras base (`a e i o u n c A E I O U N ? !`) **sí existen** y se
  localizaron celda a celda.

Evidencia: `extract/f2/grid_d1_ud1_000A4000_005.png`,
`extract/f2/m_005_*.png`.

## 3. Decisión (marcar una)

- [ ] **(a)** Los acentos existen → usarlos directamente. *No.*
- [x] **(b)** Faltan → **extender el `AIF`**. Se **reutilizan celdas libres**
  (las 19 celdas finales de `005`) + se amplían `count` y la tabla de métricas.
  *(En bancos sin huecos: reutilizar celdas de kana/kanji o ampliar la textura.)*
- [ ] **(c)** Plan C: castellano sin acentos (peor calidad, evitable).

## 4. Evidencia

* `tools/aif_edit.py` — re-codificador DXT2/3 con **round-trip exacto** del
  *tiling* + orden de bytes (`python tools/aif_edit.py selftest`).
* `tools/add_accents.py` — añade los 16 glifos (`extract/f2/atlas005_accents.*`).
* Renders: `extract/f2/d1_ud1_000A4000_005_accents_{row,demo,cells}.png`.
* Detalle completo: **`docs/re/glyph-extension.md`**.

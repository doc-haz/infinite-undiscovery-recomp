# Longitud, reflow y anti-desbordamiento (UI)

> **Problema:** el castellano es ~15–30 % más largo que el inglés. Si la traducción
> supera el tamaño del original, **desborda** la caja de texto o la pantalla (se ve
> "por fuera"). Requisito del proyecto: **la traducción nunca debe ser más ancha ni
> más alta que el original**.

## 1. Modelo de anchura (deducido y validado)

- Cada `.msg` tiene una **tabla de métricas** en `metrics_off` (2 B por glifo).
- El **avance** de un glifo = **byte bajo** de su métrica; índice = `código - 301`
  (para códigos ≥ 300).
- El ancho renderizado de una línea ≈ `Suma(avance de los glifos)`.
- El campo `width` (C) del registro del directorio ≈ **0.88 × Σavance de la línea
  más ancha** (consistente en todos los mensajes de una línea; 0.87–0.89).
  Por eso la comprobación es **relativa e independiente de escala**:
  `Σavance(ES) ≤ Σavance(EN)`.

## 2. Salto de línea

- **`0x4000` = fin de línea** (8 817 usos solo en el banco `..._005`).
- El **espacio** es un glifo normal (p. ej. código `301` en `..._005`).
- El campo `height` (D) del registro = nº de líneas × alto de línea (p. ej. 26).

Ejemplo real, `..._005` id 152 ("aviso legal"): 128 códigos, **dos `0x4000`** →
3 líneas (Σ = 335 / 645 / 390), C = 574, D = 77 (≈ 3×26).

## 3. Reglas del pipeline

`tools/build_es_codes.py` implementa:

1. **Objetivo de ancho** = `max Σavance` de las **líneas del original**
   (no del total: hay que caber por línea, no solo en total).
2. **Reflow**: reparte la traducción en palabras para que **ninguna línea** supere
   ese objetivo, insertando `0x4000` entre líneas. Respeta `\n` explícitos.
3. **Límite de líneas**: nº de líneas ES ≤ nº de líneas EN (si no, desborda en alto).
4. Las cadenas que no caben se reportan (y deben **abreviarse**).

`tools/validate_translation.py` comprueba lo mismo de forma independiente
(ancho/glifos/líneas/glifos ausentes) y puede fallar el build con `--strict`.

## 4. Ejemplo (aviso legal id 152)

| | Ancho línea máx | Líneas |
|---|---:|---:|
| Original (EN) | 645 | 3 |
| Traducción **sin** reflow | 1252 | 1 | ← **desbordaba** |
| Traducción **con** reflow | ≤ 645 | 2 | ✅ |

## 5. Herramientas

```bash
# 1) validar (opcional --strict para bloquear el build)
python tools/validate_translation.py

# 2) generar codigos + catalogo (aplica reflow)
python tools/build_es_codes.py
```

Ver `docs/re/runtime-hook.md` (inyección) y `translation/es.json`.

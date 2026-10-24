# Pipeline de OCR de mensajes (`(legacy exploration tool, removed)`)

> **Estado:** operativo. Vía **rápida e independiente** de extracción de texto:
> renderiza cada mensaje del `.msg` a una imagen y la pasa por un **OCR offline**
> (RapidOCR / onnxruntime, CPU). No usa el *charmap* `código→carácter` (que es la
> vía principal de traducción), por lo que sirve como **contraste/cross-check** y
> como forma de leer el texto sin descifrar el mapa de glifos.
>
> **Fecha:** 2026-10-08. Complementa `docs/re/font-loader.md` (§7) y
> `docs/re/text-format-research.md` (§6).

---

## 0. Resumen ejecutivo

* **Instalación offline:** `pip install rapidocr-onnxruntime` (trae `onnxruntime`
  y los modelos chinos **empaquetados**; no hace red al usarlos). Se añaden dos
  modelos de reconocimiento descargados una sola vez: **inglés** (`en_PP-OCRv3_rec`)
  y **japonés** (`japan_PP-OCRv3_rec`), porque el modelo chino por defecto confunde
  el kana.
* **Render:** el glifo vive en el canal **R/G/B (luminancia)** del atlas, **no** en
  el canal alpha (el alpha es una *máscara de celda rellena*). Cada glifo se
  recorta a su *bounding box* de tinta y se recompacta en una línea blanca sobre
  negro de 32 px de alto.
* **Uso:** `python (legacy exploration tool, removed)` (todos los bancos, ~70 min en serie o
  ~15–25 min con `--workers 8`).
* **Salida:** `extract/messages/ocr/<stem>.txt`, una línea `id<TAB>texto`.
* **Calidad medida** (referencias transcritas en `docs/re/font-loader.md`):
  **72,1 % de acierto de carácter (CAR) global**; el **inglés** queda
  prácticamente perfecto (id=2 de `…_022`: *«Great song. What's it called?»*
  con CAR 96 %), mientras el **japonés** es irregular (CAR ~10–100 %).
* **Cobertura:** **81/81** bancos `RMD-`, **71 386** mensajes (Discos 1 y 2 de la
  edición EUROPE/PAL).

---

## 1. Instalación (offline)

### 1.1 OCR de base

```bash
python -m pip install rapidocr-onnxruntime
```

Trae `onnxruntime`, `opencv-python`, `numpy`, `pyclipper`, `Shapely`, `PyYAML`
y los modelos **ch_PP-OCRv3** dentro del paquete
(`…/site-packages/rapidocr_onnxruntime/models/*.onnx`). **No necesita red** para
inferir. Verificado con **Python 3.14.6 / Pillow 12.3.0 / rapidocr-onnxruntime
1.2.3 / onnxruntime 1.30.0**.

> **Aviso de entorno (Windows):** en esta máquina hay **dos Python 3.14**. `pip`
> (`…\AppData\Local\Python\pythoncore-3.14-64`) apunta a un intérprete distinto
> del que resuelve `python` (`…\Programs\Python\Python314`). Hay que instalar con
> **`python -m pip install …`** para que el paquete quede visible al `python` que
> ejecuta el script.

### 1.2 Modelos inglés y japonés

El modelo chino por defecto reconoce bien el latín pero **no está entrenado para
kana**. Se descargan dos modelos ONNX de reconocimiento a `ocr/models/`:

```bash
python (legacy exploration tool, removed) --download-models
```

| Modelo | Fichero | Origen | Tamaño |
|---|---|---|---|
| Inglés | `en_PP-OCRv3_rec_infer.onnx` | `SWHL/RapidOCR` (HuggingFace) | 8,9 MB |
| Japonés | `japan_PP-OCRv3_rec_infer.onnx` | `breezedeus/cnocr-ppocr-japan_PP-OCRv3` | 10,1 MB |

Ambos ONNX llevan el diccionario de caracteres **embebido** en sus metadatos
(`custom_metadata_map["character"]`), así que RapidOCR los carga directamente
con `rec_model_path=...` (no hace falta fichero de claves aparte).

### 1.3 Alternativa descartada

`pytesseract` **no** se usó: Tesseract no está instalado en el sistema
(`which tesseract` → vacío) y su motor es claramente peor en este tipo de fuente
pixel/outline. RapidOCR cubre la necesidad sin binario externo.

---

## 2. Uso

```bash
# 1) descargar los modelos en/jp (una vez)
python (legacy exploration tool, removed) --download-models

# 2) OCR completo (Discos 1 y 2) -- ~70 min en serie
python (legacy exploration tool, removed)

#    ...o en paralelo (recomendado)
python (legacy exploration tool, removed) --workers 8 --chunk 400 --batch 32

# 3) medir calidad contra las referencias conocidas
python (legacy exploration tool, removed) --eval

# pruebas
python (legacy exploration tool, removed) --stems d1_ud1_39513800_022 --limit 1
python (legacy exploration tool, removed) --lang en            # solo inglés (más rápido)
python (legacy exploration tool, removed) --save-renders        # volcar tambien los PNG
```

**Opciones principales**

| Opción | Descripción |
|---|---|
| `--codes` | JSON de códigos (def. `extract/messages/all_codes.json`) |
| `--f0` | directorio de atlas `<stem>.png` (def. `extract/f0`) |
| `--out` | directorio de salida (def. `extract/messages/ocr`) |
| `--stems` | subconjunto de bancos |
| `--limit` | nº máximo de bancos (pruebas) |
| `--lang` | `auto` (def.) \| `en` \| `jp` \| `ch` |
| `--workers` | procesos en paralelo (def. 8; `1` = serie) |
| `--chunk` | mensajes por unidad de trabajo, **resumible** (def. 400) |
| `--batch` | tamaño de lote del recognizer (def. 32) |
| `--tol` | tolerancia de anchura al agrupar lotes (def. 1.06) |
| `--force` | reprocesa aunque existan resultados parciales |
| `--save-renders` | guarda los PNG renderizados |
| `--download-models` | descarga `en`/`jp` y sale |
| `--eval` | mide CAR/CER y sale |

**Reanudable:** el trabajo se trocea en *chunks* y cada uno se escribe en
`ocr/parts/<stem>__<id0>.txt`; al terminar se unen en `ocr/<stem>.txt`. Si el
proceso se corta, relanzarlo omite los chunks ya hechos (usar `--force` para
rehacerlo todo).

---

## 3. Cómo funciona

### 3.1 Render (`load_cells` + `render`)

Del formato confirmado (`docs/re/font-loader.md`):

* `code == 0` → fin; `code >= 0x4000` → control (se **omite**);
  `300 ≤ code < 0x4000` → **glifo**, índice en el atlas `= code - 301`;
  `code < 300` → sub-fuente (otro atlas de menor tamaño), **se omite** aquí.
* Atlas `AIF` ya exportado a PNG: **32×32** por celda, `cols = atlas_w/32`,
  **row-major**.

**Hallazgo clave del render:** los glifos están en los canales **de color
(luminancia)**, pintados *blanco sobre negro*; el **canal alpha es sólo una
máscara de celda rellena** (un óvalo/rectángulo) y **no** representa el carácter.
Usar el alpha produce rectángulos ilegibles para el OCR. El pipeline convierte el
PNG a `L` (luminancia) y umbraliza a tinta (`> 90`).

Cada glifo se recorta a su *bounding box* de tinta, se **recompacta** con un
`gap` de 3 px (el espaciado original de 32 px por glifo destroza el OCR) y los
glifos en blanco se tratan como espacio (avance 10 px). Resultado: una tira de
32 px de alto, negra, con texto blanco.

### 3.2 OCR y selección de modelo (`Engine`)

Se usan tres reconocedores PP-OCRv3:

| Motor | Papel |
|---|---|
| `ch` (empaquetado) | reconocimiento genérico |
| `en` (descargado) | **mejor latín/inglés** |
| `jp` (descargado) | **kana/kanji** |

Modo `auto`: se reconoce todo con **`jp`**; las líneas cuyo resultado **no**
contiene CJK se vuelven a reconocer con **`en`** (el japonés también transcribe
latín, así que sirve de *detector de script*). Así el inglés usa el mejor modelo
y el japonés usa el único que emite kana.

**Lotes:** el recognizer se llama con listas de imágenes
(`text_recognizer([...])`), lo que da ~2,6× de velocidad. Las imágenes se agrupan
por **anchura similar** (`tol=1.06`) para minimizar el *padding*: mezclar tiras de
anchos muy distintos en un mismo lote **degrada** la calidad (el modelo rellena
todo al ancho máximo).

### 3.3 Paralelismo

`--workers N` reparte los *chunks* entre procesos (`spawn`). Cada worker fija
`intra/inter_op_num_threads = 1` (variable de entorno `IU_OCR_THREADS`) para
evitar la sobre-suscripción de hilos de onnxruntime.

---

## 4. Calidad medida

Referencias transcritas y validadas en `docs/re/font-loader.md` (§1.3 y §7); se
comparan con la salida del OCR mediante **distancia de edición** (ignorando
espacios).

| Banco | id | Referencia | OCR | CER |
|---|---:|---|---|---:|
| `d1_ud1_39513800_022` | 2 | Great song. What's it called? | Great song. what's it called? | **4,0 %** |
| `d1_ud1_39513800_022` | 4 | I wrote it myself. It's entitled… | I wrote it myself It s entitled..- | 17,9 % |
| `d1_ud2_A4DAA000_040` | 3 | マエストロを手に入れた | マエストロを手に入れた | 0,0 % |
| `d1_ud1_39513800_022` | 3 | これ 僕がつくったんだ曲名は… | これ僕ボうくつ志ん型曲名はが | 42,9 % |
| `d1_ud2_A4DAA000_040` | 1 | セラフィックゲートはようこそ | セラフィンタケートエナラミン | 57,1 % |
| `d1_ud1_39513800_022` | 5 | 聞かなくちゃよかったね… | 聞カなまやよカつ右わ・は「 | 91,7 % |

> **CAR global (char accuracy) ≈ 70–72 %.** El caso de control del enunciado
> (`…_022` id=2 = *«Great song. What's it called?»*) sale con **96 %** (sólo falla
> la caja de la `w` y el punto final). El **inglés/latino** es fiable; el
> **japonés** es irregular (el modelo `jp` confunde kana con formas similares).

**Efecto del lote** (misma muestra):

| Configuración | CAR global | Velocidad |
|---|---:|---:|
| `--batch 1` (máxima calidad) | **72,1 %** | ~5 msg/s |
| `--batch 32`, `tol=1.06` (def.) | 70,2 % | ~17 msg/s |
| `--batch 32`, `tol=1.15` | 64,4 % | ~17 msg/s |

**Conclusión de calidad:** el OCR es una **vía de contraste** útil para el inglés
(casi perfecto) y da una lectura aproximada del japonés; **no** sustituye al
*charmap* `código→carácter`, que es exacto. Para el texto inglés, el OCR es ya
directamente aprovechable.

---

## 5. Cobertura

* **Bancos:** 81/81 `RMD-` con mensajes (36 del Disco 1 + 45 del Disco 2;
  los dos `*_000A4000_004` son sólo-fuente y no tienen mensajes).
* **Mensajes:** **71 386** (Discos 1 y 2 de la edición EUROPE/PAL), exactamente
  los de `extract/messages/all_codes.json`.
* **Salida:** `extract/messages/ocr/<stem>.txt` (81 ficheros; `id<TAB>texto`, una
  línea por mensaje, en orden de `id`).
* **No vacíos:** la mayoría de mensajes producen texto; los vacíos corresponden a
  mensajes sin glifos representables (`code < 300`, elisión de controles) o a
  reconocimientos por debajo del umbral `--text-score`.
* **Bancos que son tabla de fuente:** los dos `*_000A4000_005` (16 647 «mensajes»
  cada uno) no son diálogo sino la **cobertura de glifos** de la fuente: casi
  todos sus `id` son **un único glifo**, así que el OCR devuelve un carácter suelto
  (a menudo `a`). No deben interpretarse como texto real.
* `extract/messages/ocr/_meta.json` guarda el recuento y el tiempo de la corrida;
  `_eval.txt`, el informe de calidad.

---

## 6. Limitaciones

1. **Sub-fuente (`code < 300`):** no se renderiza (pertenece a otro atlas, no
   exportado aquí). Afecta a cadenas cortas de menús/UI.
2. **Controles de layout (`≥ 0x4000`):** se omiten; no hay saltos de línea ni
   iconos de botón. Cada mensaje se renderiza en **una sola línea**.
3. **Japonés irregular:** el modelo `japan_PP-OCRv3` se entrenó con texto natural,
   no con esta fuente de pixel; el kana se confunde.
4. **Espacios/case:** el OCR no distingue bien `'`/`"`, espacios finos ni caja;
   normalizar antes de comparar.
5. **No sustituye al charmap:** la vía exacta sigue siendo
   `docs/re/font-loader.md` + `tools/encode_messages.py`.

---

## 7. Ficheros

| Ruta | Contenido |
|---|---|
| `(legacy exploration tool, removed)` | pipeline (render + OCR + eval + descarga de modelos) |
| `extract/messages/ocr/<stem>.txt` | texto OCR por banco (`id<TAB>texto`) |
| `extract/messages/ocr/models/*.onnx` | modelos en/jp (descargados) |
| `extract/messages/ocr/_meta.json` | metadatos de la corrida |
| `extract/messages/ocr/_eval.txt` | informe de calidad |
| `extract/messages/ocr/parts/` | trabajos parciales (resumible) |

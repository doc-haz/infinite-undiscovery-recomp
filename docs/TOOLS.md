# Tools reference

Complete list of the pipeline scripts (all under `tools/`). They are **offline
development tools**: you don't need them to play — only to rebuild the
translation from the original discs. Grouped by pipeline phase.

## F0 — Extract resources

| Script | What it does |
|---|---|
| `extract_rmd.py` | Isolates the `RMD-` message-resource entries from `ud1.bin`/`ud2.bin` (decompressing `SLZ`), splitting each into `<label>.aif` (font atlas) + `<label>.msg` (message data) and writing a manifest. |

## F1 — Decode the message format

| Script | What it does |
|---|---|
| `decode_messages.py` | The `RMD-`/`.msg` decoder (`MsgBank`): records table + code pool, with control codes. The reference decoder (exact round-trip). |
| `write_texts.py` | Decodes **all** `.msg` banks to readable text (`extract/messages/text/`). |
| `game_text_codec.py` | The game's text codec (ASKA): maps characters ↔ game codes for a given charmap. |

## F2 — Charmaps & glyph identification

| Script | What it does |
|---|---|
| `build_charmaps.py` | Generates `translation/charmaps/<stem>.json` (code → character) **per bank**. |
| `glyph_ocr.py` | Proposes the character of each glyph cluster by comparing with normalised system-font templates. |
| `atlas_glyphs.py` | Shared library for extending the `RMD-` atlases (style signatures, masks). |
| `usage_stats.py` | Usage frequency of each code in the messages (global and per bank) — used to pick the **most-used** glyph variant. |

## F3 — Fonts & accents

| Script | What it does |
|---|---|
| `add_accents.py` | Adds the 16 Spanish glyphs to a single `RMD-` atlas (reference implementation). |
| `add_accents_banks.py` | Adds the Spanish glyphs **per bank** (own bases + free cells) and emits `translation/accents_atlas.bin` + `accents_manifest.json`. |
| `extend_atlas_banks.py` | Completes a bank's atlas with the missing Latin glyphs (F3-b). |
| `patch_rmd_accents.py` | Applies an already-patched `RMD-` bank (AIF + `.msg`). |
| `verify_accents_banks.py` | Offline verification of the per-bank accent patches (round-trip). |
| `verify_accents_render.py` | Offline acceptance test of the accents (rendered proof). |

## F4–F5 — Translation catalog

| Script | What it does |
|---|---|
| `build_es_codes.py` | Merges the translation parts, applies **reflow** (control codes) and no longer folds accents for patched banks; emits `translation/es_codes.json` + `es_catalog.bin`. |
| `gen_es_catalog.py` | Generates the binary catalog consumed by the runtime hook (adds aliases for the accented `.msg` fingerprints). |
| `validate_translation.py` | Checks that translations do not exceed the original's size (width/alerts). |
| `fix_punct.py` | Fixes mislabeled punctuation in a bank charmap. |

## F6 — Textures

| Script | What it does |
|---|---|
| `ui_textures.py` | Renders the translated **labels that are textures** (bitmaps) in AIF and writes the manifest consumed by the hook. |
| `ui_help_screens.py` | Translates the **CAMP help screens** (1280×720 DXT1) with inpainting + repaint. |
| `scan_aif_text.py` | Systematic OCR inventory of AIF textures **with text** (loose + embedded in containers). |
| `scan_embedded_aif.py` | Finds the main-menu textures by **content** (embedded AIFs). |
| `aif_edit.py` | AIF (Aska Image File) editor: decode, modify pixels, re-encode preserving the wrapper. |
| `aif_text_report.py` | Summary report of the AIF-with-text inventory. |

## Controllers

| Script | What it does |
|---|---|
| `controller_skins.py` | Generates per-controller UI variants (Xbox / PS5 / Steam) of the button icons. |
| `verify_controller.py` | Offline verification of the multi-controller support (detection + manifest variants). |

## Typical end-to-end run

```bash
python tools/extract_rmd.py --roms roms --out extract/f0      # F0
python tools/write_texts.py                                   # F1
python tools/build_charmaps.py                                # F2
python tools/add_accents_banks.py --need-from-es --update-charmaps   # F3
python tools/build_es_codes.py                                # F4 (catalog)
python tools/ui_textures.py render --out extract/f2/ui_es     # F6
python tools/ui_textures.py manifest --out extract/f2/ui_es --deploy extract/f2/run/translation
python tools/ui_help_screens.py render                        # F6 (CAMP help)
```

> The reverse-engineering itself (locating the message subsystem, the parser,
> the decoder and the charmap loader) was done with **Ghidra** + **Capstone**;
> those one-off exploration scripts were removed once the format was fully
> reverse-engineered and validated.

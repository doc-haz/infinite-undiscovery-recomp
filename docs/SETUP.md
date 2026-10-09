# Setup & reproduction guide

How to reproduce this project from scratch on a clean machine: environment,
dependencies, reconstructing the translation from the discs, and building the
patched recomp.

> **You must provide your own legal copy of *Infinite Undiscovery* (Disc 1 +
> Disc 2).** No game assets are included in this repository.

---

## 1. One-shot bootstrap (automated)

On Windows, the bootstrap installs the Python dependencies, initialises the
submodules and clones + patches the upstream recomp:

```powershell
powershell -ExecutionPolicy Bypass -File bootstrap.ps1
```

It also checks the external tools below and tells you what to install. It does
**not** download game assets (see step 3).

---

## 2. Software prerequisites

| Tool | Why | Notes |
|---|---|---|
| **Python 3.10+** | pipeline (`tools/`) | `bootstrap.ps1` installs the pip deps |
| **git** | submodules + cloning the recomp | |
| **7-Zip** | extracting some archives | `C:\Program Files\7-Zip\7z.exe` |
| **Tesseract OCR** | texture/glyph OCR (`scan_aif_text.py`) | |
| **CMake ≥ 3.25 + Ninja + Clang/LLVM + MSVC + Windows SDK** | building the recomp | all included with **Visual Studio** (use `vcvars64.bat`) |
| **ReXGlue SDK v0.10.0 (built from source)** | the recomp runtime | see §5 |

Python packages (installed by the bootstrap): **Pillow, numpy, opencv-python**
(see `requirements.txt`).

---

## 3. Put your discs in `roms/`

```
roms/Disc1.iso      (or an extracted folder)
roms/Disc2.iso
```

`roms/` is git-ignored.

---

## 4. Reconstruct the translation (pipeline)

Run from the repository root. See `docs/TOOLS.md` for what each script does.

### Quick start — one command

```bash
bash run_all.sh              # full pipeline + build + deploy
bash run_all.sh --no-build   # pipeline + deploy only
bash run_all.sh --no-extract # skip F0 (reuse an existing extract/f0)
```

### Step by step

```bash
# F0 — isolate the RMD- message resources from the discs
python tools/extract_rmd.py --roms roms --out extract/f0

# F1 — decode every .msg bank to readable text
python tools/write_texts.py

# F2 — per-bank charmap (code -> character)
python tools/build_charmaps.py

# F3 — add the Spanish glyphs (accents) per bank + build accents_atlas.bin
python tools/add_accents_banks.py --need-from-es --update-charmaps

# F4 — build the translation catalog
python tools/build_es_codes.py            # -> mod/translation/es_catalog.bin

# F6 — render the translated textures and build the manifest
python tools/ui_textures.py render --out extract/f2/ui_es
python tools/ui_textures.py manifest --out extract/f2/ui_es --deploy extract/f2/run/translation
python tools/ui_help_screens.py render    # CAMP help screens
```

The translation data lives in `mod/translation/`; the runtime reads it from
`translation/` **relative to the game's working directory** (`run/`), so deploy
`mod/translation/` → `run/translation/`.

---

## 5. Build the patched recomp

1. **Build the ReXGlue SDK v0.10.0 from source** (tag `v0.10.0`), following the
   recomp's `docs/RELEASE-BUILD.md`. The recomp verifies the SDK DLL hashes
   (`cmake/verify_rexglue_dlls.cmake`); the pinned hashes correspond to a
   specific from-source build, so **re-pin them to your own build** (see the
   analysis in `docs/build.md`) and regenerate `RELEASE_HASHES_SHA256.txt`.
2. **Apply the mod** (or use the bootstrap, which does this):
   ```bash
   cd extract/f2/recomp
   git apply /path/to/mod/recomp/local-changes.patch
   cp /path/to/mod/recomp/*.h src/
   ```
3. **Build** with the wrapper (it copies the headers and forces `main.cpp` to be
   recompiled, because ninja does not track header dependencies here):
   ```bash
   bash mod/recomp/build.sh
   ```
4. **Deploy** the built exe to `run/` and put your discs' assets under
   `run/<PROFILE>/assets/disc{1,2}/` (see the recomp's setup wizard), then run
   `run/InfiniteUndiscoveryRecomp.exe`.

---

## 6. Known gotchas

- **Headers are not tracked by ninja** → always build via `mod/recomp/build.sh`
  (it `touch`es `main.cpp`); and **copy the exe to `run/`** before launching.
- **ReXGlue hashes**: a fresh from-source SDK build produces different DLL bytes
  than the pinned hashes → re-pin them (details and evidence in `docs/build.md`).
- **No asset patching**: the mod is applied entirely at runtime; never modify
  `ud1.bin`/`ud2.bin`.
- In the runtime text hook, capture `ctx.r3` on entry (it holds the object
  pointer; on return it holds the return value).

---

## 7. Repository map

```
mod/        the mod (recomp hooks + translation data) — see mod/README.md
tools/      offline pipeline — see docs/TOOLS.md
docs/       setup (this file), TOOLS, PLAN, PR, and docs/re/ (reverse-engineering)
```

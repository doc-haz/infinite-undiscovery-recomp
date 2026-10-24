# Audio assets of Infinite Undiscovery (Xbox 360, EU/PAL) — fan-dub feasibility

> **Scope of this document.** Read-only investigation of the game's audio: where the
> files live, what codec they use, and whether the voice/dialogue could be replaced by
> re-recorded audio (a *fan dub*) and, in particular, whether that could ever work on a
> **real / original Xbox 360** and not only on the PC recompilation that this Spanish
> translation project is built on.
>
> Nothing on disk was modified or deleted; the game executable was never launched. All
> measurements below come from the unmodified extracted containers
> (`extract/f2/run/EUROPE/assets/disc1/ud1.bin`, `ud2.bin`) and the extraction trees
> `extract/f2/all_d1ud1/` (from `ud1.bin`) and `extract/f2/all_d1ud2/` (from `ud2.bin`).

---

## 0. Verdicts (short)

| Question | Verdict | Why |
|---|---|---|
| Is the voice/dialogue audio locatable and readable? | **Yes, completely** | ~22 600 sounds in named `AAC ` containers; names are the original `.wav` filenames |
| What codec is it? | **XMA2** (Xbox 360 hardware codec) | format tag `0x0166`, `0x8000`-byte blocks, 512-sample frames; ffmpeg reports `codec_name=xma2` |
| Can we **decode** it? | **Yes** | ffmpeg 6.1, vgmstream, ToWav, and the ReXGlue/Xenia software decoder all decode it |
| Can we **encode** XMA2? | **No, not with free tools** | There is no open-source XMA encoder. Microsoft ships one only in the Xbox GDK/XDK (`xma2encode.exe`, `xmaencode.exe`, XACT), licensed and under NDA |
| Can a fan dub be done on the **PC recomp**? | **Yes, feasible** | The recomp decodes XMA in software (Xenia-derived `XmaDecoder`) and hooks functions at runtime; Spanish PCM/WAV can be substituted without ever encoding XMA and without touching `ud1.bin`/`ud2.bin` |
| Can a fan dub be done on **real hardware** by replacing on-disk audio? | **Only if you can produce XMA2** | The console's XMA decoder is hardware; the engine submits an `XMA2WAVEFORMATEX`. Re-encoding is the blocker, plus disc/XEX constraints |
| Does a precedent exist? | **Yes — an "undub" already exists for this game** | The recomp explicitly supports a `USA-UNDUB` build (Japanese voices, English text). That undub *splices existing XMA2*, so it needed no encoder — which is why a *Spanish* dub is harder than the existing undub |

**Bottom line:** a Spanish fan dub is realistically achievable **on the PC recomp**
(runtime substitution). On a **real Xbox 360** it is technically possible but blocked in
practice by the absence of a free XMA2 encoder; the only clean on-disk route requires
Microsoft's licensed XMA2 encoder and a rebuilt disc/XEX on an RGH/JTAG (or emulator).

---

## 1. Where the audio lives

### 1.1 Disc layout

Both EU/PAL discs are **XGD2** Xbox 360 DVDs. The game filesystem (XDVDFS) holds only
four files; all game content is inside two monolithic containers:

```
$ python tools/vendor/pc-infiniteundiscovery/tools/xdvdfs.py list "roms/Infinite Undiscovery (Europe) (Disc 1).iso"
d D-----   1779903         2048  /$SystemUpdate
- -----N   1779904      7229440  /$SystemUpdate/su20076000_00000000
- -----N    696582     11055104  /default.xex
- -----N    701981   2207584256  /ud1.bin
- -----N   1783936   2800330752  /ud2.bin
```

* `default.xex` is an **encrypted, signed XEX2** image.
* `ud1.bin` / `ud2.bin` are sequences of self-describing **NORM/MRON** archives laid end
  to end on 2048-byte boundaries, with unrelated blobs (video, compressed blocks, audio)
  filling the gaps. There is no global table of contents.

Two kinds of audio exist:

1. **`SOND` resources inside the archives** — sound effects, monster/NPC foley, and the
   line-by-line voice acting. These are the files present in the extracted trees.
2. **Bare `AAC ` containers in the gaps between archives** — the **music**. These are *not*
   archive entries and therefore are *not* in `all_d1ud1/` or `all_d1ud2/`.

### 1.2 Resource-type census of the extracted trees

```
$ cd extract/f2/all_d1ud1 && ls | python -c "... count by tag ..."
5718 ANIM  2594 SIG  1505 MESH  1275 SOND  972 COLL  908 SEEK  463 EPAC
 217 WEAP  141 IMG  109 MTEX   98 SKAC   54 APAC   11 TTD   11 MAIF
  10 AREA    5 RMD    4 NODE    4 SCE     4 MINI    4 TTEX     (14107 files)

$ cd extract/f2/all_d1ud2 && ls | python -c "... count by tag ..."
3340 EPAC 1256 MTEX  997 APAC  474 MESH  379 ANIM  283 SOND  203 COLL
 197 SIG   105 MAIF   84 AREA   63 SEEK   40 NODE   40 MINI   33 SCE
  32 IMG    31 RMD    25 TTEX    2 LNS     1 TTD               (7585 files)
```

`SOND` = "sound bank (AAC)". Audio only appears under this tag (plus the gap music).

### 1.3 The `SOND` containers: counts, sizes, volume

| Container | `SOND` entries | empty (16 B) | real banks | stored on disc | decompressed (extracted) | `SOND` sounds | duration |
|---|---:|---:|---:|---:|---:|---:|---:|
| disc 1 `ud1.bin` | 1275 | 445 | **830** | 219,564,200 B (209.4 MiB) | 361,069,120 B (344.3 MiB) | **22,243** | **257.7 min** |
| disc 1 `ud2.bin` | 283 | 0 | **283** | 59,611,252 B (56.9 MiB) | 63,736,704 B (60.8 MiB) | **358** | **30.0 min** |
| **total** | 1558 | 445 | 1113 | 279,175,452 B (266.2 MiB) | 424,805,824 B (405.1 MiB) | **22,601** | **287.6 min (4.79 h)** |

> Note the difference between "stored on disc" and "decompressed": **349 of the 1275
> `ud1.bin` `SOND` resources are SLZ-compressed** inside the archive (see §1.6). The
> extraction tree holds the decompressed `AAC ` containers.

Channels/rates of the `SOND` corpus (mono = one stream, stereo = two):

```
ud1.bin SOND : 22,243 sounds, 22,236 mono / 7 stereo, 84 looping, 506 distinct sample rates
ud2.bin SOND :    358 sounds,    288 mono / 70 stereo, 102 looping,  69 distinct sample rates
```

Sample rates cluster around 48 kHz but are **per-sound** (47 999, 48 128, 47 820, …): the
engine detunes each sound. Bitrate is ~768 kbit/s for a typical mono voice line
(2 bytes/sample × 48 kHz), which is what XMA2 delivers.

### 1.4 The music is in the gaps, not in `SOND`

Walking a gap as a run of self-describing `AAC ` containers recovers the soundtrack:

```
$ python tools/vendor/pc-infiniteundiscovery/tools/aac.py find \
    extract/f2/run/EUROPE/assets/disc1/ud1.bin --offset 0x28EBD800 --length 143654912
  +0x00000000    3842048 bytes    1 sound   BGM_01_SIGUMUND.wav
  +0x003AA000    4333568 bytes    1 sound   BGM_02_CAPEL.wav
  ...
  +0x0871E000    1974272 bytes    1 sound   BGM_78_RAIN_OF_MOON_B.wav
containers 38, 38 sounds, 87.2 minutes
covered 0x8900000 of 0x8900000 bytes (100.0%)
```

Across both discs the music is **79 tracks / 189 minutes**, in the `ud1.bin` and `ud2.bin`
gaps. This matters for the dub: **a mixed-language dub is natural here** — the music and
sound effects can be left untouched; only the dialogue WAVEs inside `SOND` banks need
replacing.

### 1.5 What people actually say (rough dialogue estimate)

Names are grouped by role. Counting sounds whose names mark them as voice/dialogue
(`_SP_` speech, `_FV_` field voice, `BTL`/`BATTLE` battle lines, `SITU` situational):

```
category                    count    minutes
dialogue+voice (SP/FV/BTL/SITU)  ~3,600    ~40
sfx: combat/move             5,920     91.0
sfx: footstep                5,412     44.1
sfx: system/gimmick            289     11.8
other (foley/ambient/enemy)  ~7,800   ~100
```

So the **re-recordable dialogue is on the order of a few thousand lines / ~40 minutes**;
the bulk of the 22 601 sounds is movement foley and monster/NPC sound effects
(`ENM_*_WALK_*`, `CAPEL_*_RUN_*`, `FIELD_*` ambience). This is an estimate from filename
conventions, not a definitive transcript — treat it as an order of magnitude.

---

## 2. Codec

### 2.1 The `AAC ` container (Aska Audio Container)

`AAC ` here is **not MPEG AAC**. It is tri-Ace's own container; the retail binary contains
the debug string `AAC version problem  BGM ID=%d`. Layout (big-endian), from
`tools/vendor/pc-infiniteundiscovery/tools/aac.py` and confirmed by direct inspection:

```
+0x00  4  "AAC "
+0x04  4  total size of the container (exact, header included)
+0x08  8  zero, or 0xFFFFFFFF twice
+0x10  4  directory entry count
+0x14  4  directory offset (= 0x30)
+0x18  4  directory region size
+0x1C  4  playback-table offset
+0x20  4  version 0x00010003, constant
+0x24 12  zero
        then:  DIR  -> dirn entries (0xA0 each) -> WAVE chunks -> PLBK records
```

Real header of `extract/f2/all_d1ud1/000A4000_023_SOND.bin` (113 sounds, 4,173,824 B):

```
00000000: 41 41 43 20 00 3f b0 00 00 00 00 00 00 00 00 00   AAC .?..........
00000010: 00 00 00 71 00 00 00 30 00 00 a0 20 00 3f 70 00   ...q...0... .?p.
00000020: 00 01 00 03 00 00 00 00 00 00 00 00 00 00 00 00   ................
00000030: 44 49 52 20 00 00 a0 20 00 00 00 00 00 00 00 00   DIR ... ........
00000040: 00 00 00 71 00 00 00 00 00 00 00 00 00 00 00 00   ...q............
00000050: 00 00 00 71 00 00 00 00 ...                       (dirn entries follow)
```

* `+0x04 = 0x003FB000 = 4,173,824` equals the file length exactly.
* `+0x10 = 0x71 = 113` entries, `+0x14 = 0x30` (DIR right here), `+0x1C = 0x3F7000`
  (playback table, just past the last sound).

A `dirn` entry (0xA0 bytes) carries a NUL-padded name (up to 0x80 bytes) and, at +0x90,
the **absolute container offset of its `WAVE`** and an id:

```
$ ... aac.py info ... ; Python entry dump
name=SYSTEM_001_DECISION.wav   wave_off=0xB000  id=1
name=SYSTEM_002_CANCEL.wav     wave_off=0xD000  id=2
name=SYSTEM_003_PAUSE.wav      wave_off=0xF000  id=3
PLBK @0x3F7000 size=0x70 id=1 trim=-200
```

The names are the **original source `.wav` filenames** — a complete human-readable audio
inventory shipped with the game (`BGM_24_BATTLE_SCENE.wav`, `AYA_SP_0001.wav`,
`DOOR_001_WOOD_S_OPEN.wav`, …).

### 2.2 A `WAVE` chunk

Each sound is a `WAVE` chunk whose header describes an XMA2 stream:

```
$ xxd -s 0xB000 -l 0x80 extract/f2/all_d1ud1/000A4000_023_SOND.bin
0000  57 41 56 45 00 00 20 00 00 00 00 00 00 00 20 00   WAVE ............
0010  04 00 00 01 00 01 01 65 00 00 00 00 00 00 00 00   .......e........
0020  00 00 10 00 00 00 bc 00 00 00 01 80 00 00 48 00   ..............H.
0030  00 00 80 00 00 00 4a 00 00 00 46 a0 00 00 00 01   ......J...F.....
0040  00 00 00 00 00 00 01 80 00 00 00 00 00 00 48 00   ..............H.
0050  73 74 72 6d 00 00 00 30 ...                        strm...0
```

| Offset in `WAVE` body | Field (this sample) | Meaning |
|---|---|---|
| +0x00 | `0x04000001` | `0x04` in top byte + entry id |
| +0x04 | `0x00010165` | version, constant |
| +0x08 | 0 / `0x995A7C80_00000015` | unexplained |
| +0x10 | `0x00001000` = 4096 | size of the audio data |
| +0x14 | `0x0000BC00` = 48128 | sample rate |
| +0x18 | `0x00000180` = 384 | play begin (XMA encoder delay; loop point when larger) |
| +0x1C | `0x00004800` = 18432 | play end (samples) |
| +0x20 | `0x00008000` = 32768 | **block size = XMA2's `bytesPerBlock`** |
| +0x24 | `0x00004A00` = 18944 | total samples encoded (multiple of 512) |
| +0x28 | `0x000046A0` = 18080 | second sample count |
| +0x2C | `1` | **block count = ceil(data_size / 0x8000)** |
| +0x50 | `strm`, size 0x30 | channel count at `WAVE+0x60` (1 here, 2 for stereo) |
| **+0x1000** | audio bytes | **every sound's XMA2 begins exactly 0x1000 into the chunk** |

The `strm` sub-chunk restates the rate and play range and adds the channel count (1 or 2).

### 2.3 It is XMA2 — direct evidence

The first bytes of the first `WAVE`'s audio (`WAVE + 0x1000 = 0xC000`):

```
0000  38 00 01 00 08 03 fc 03 80 00 15 b4 73 17 45 bd  8...........s.E.
0010  24 21 af 9b c6 38 52 5c 80 f3 e1 09 17 6f d5 a4  $!...8R\.....o..
```

Wrapping exactly these bytes in the RIFF header an XMA2 decoder expects (format tag
`0x0166`, an `XMA2WAVEFORMATEX` built from the fields above) and asking ffmpeg:

```
$ ffprobe ... xma_test/000_SYSTEM_001_DECISION.xma
codec_name=xma2
sample_rate=48128
channels=1
bit_rate=770048

$ ffmpeg -i xma_test/009_SYSTEM_008_SONG_CHANGE.xma -f null -
        (decodes with no errors)
```

The tool that produces this wrapper (`tools/vendor/.../aac.py`) re-encodes nothing — it
copies the payload through and fills the wrapper from the `WAVE` header. The
`XMA2WAVEFORMATEX` it writes:

```
wFormatTag=0x0166 (XMA2)   nChannels   nSamplesPerSec   nAvgBytesPerSec=rate*ch*2
nBlockAlign=ch*2           wBitsPerSample=16            cbSize=34
NumStreams   ChannelMask   SamplesEncoded   BytesPerBlock(=0x8000)
PlayBegin    PlayLength    LoopBegin        LoopLength     LoopCount   EncoderVersion(=4)  BlockCount
```

This is the canonical `XMA2WAVEFORMATEX`; the game builds the same structure when it hands
the stream to the console audio stack.

### 2.4 Tooling for XMA

| Tool | Decode | Encode | Notes |
|---|---|---|---|
| **ffmpeg 6.1** | ✅ `xma1`, `xma2` decoders | ❌ | Verified locally: `-encoders` lists no `xma`; only `adpcm_ms`, `wmav1`, `wmav2` |
| **vgmstream** | ✅ | ❌ | Open source, read-only |
| **ToWav / xma_parse (hcs64)** | ✅ | ❌ | Closed-source decoders |
| **ReXGlue / Xenia `XmaDecoder`** | ✅ | ❌ | Software XMA decoder used by the PC recomp (Xenia source header, BSD) |
| **Microsoft GDK/XDK** `xma2encode.exe`, `xmaencode.exe`, XACT (XNA) | ✅ | ✅ | The **only** known XMA2 encoder; licensed Xbox developer tooling, under NDA |

The MultimediaWiki `XMA` page confirms the encoder situation: *"`xmaencode.exe` from the
Xbox 360 XDK provides encoding"*, and the modern GDK documents an **XMA2 encoder tool**
(`xma2encode`) that converts PCM ↔ XMA2. No open-source XMA encoder exists, which is the
single practical obstacle for any on-disk fan dub.

---

## 3. Feasibility of replacing the audio (fan dub)

### 3.1 Are the containers fixed-size or resizable?

They are **resizable**, but resizing cascades through several layers because everything is
referenced by byte offsets and sizes:

1. **Inside `AAC `** — the header's total size (+0x04), the `playback_offset` (+0x1C), each
   `dirn` entry's absolute `WAVE` offset, and every `WAVE`'s `data_size`, `block_count` and
   chunk `step` must be recomputed. The audio always starts `0x1000` into its `WAVE`.
2. **Inside the MRON archive** — the 32-byte entry table stores `size` and `offset`
   (relative to the archive) for each resource; entries are contiguous in offset order, so
   changing one `SOND` size shifts all later entries. The archive's own length is
   `align_up(max(offset+size), 2048)`.
3. **Inside `ud1.bin`/`ud2.bin`** — archives are laid end to end on 2048-byte boundaries,
   so enlarging one archive pushes into the next. A correct rebuild relocates subsequent
   archives (still 2048-aligned).
4. **The disc image** — the XDVDFS file sizes for `ud1.bin`/`ud2.bin` must be regenerated.

A tool that rebuilds the whole chain is entirely doable (the formats are documented and the
vendored `mron.py`/`aac.py` already parse them); it is bookkeeping, not a codec problem.

**Caveat / uncertainty.** The vendored reverse-engineering notes state `ud1.bin`/`ud2.bin`
have *"no global table of contents"* and the tools walk archives by their self-describing
sizes. If that is literally how the engine locates resources, relocating archives is safe.
However I could **not** prove the engine has no index: the first `0x16000` bytes of each
`ud1.bin` are unexplained high-entropy data (entropy ≈ 5.97–7.67 bits/byte, different per
container), and I found no copies of the music-gap offsets inside `default.exe`. If that
blob is an encrypted index, a naive repack could break lookup. This should be tested on the
recomp before trusting a full repack.

### 3.2 Are there checksums / signatures the game validates?

* **`AAC ` container and MRON archive: no checksums.** The formats carry only sizes,
  offsets, ids and constants — no CRC/hash field anywhere in the parsers.
* **`ud1.bin`/`ud2.bin`: not signed by the retail game.** Only `default.xex` is a signed,
  encrypted XEX2 image. Game data files are plain content.
* **The disc's XGD2 security sector** covers the disc for the DVD drive's anti-piracy
  checks, but it is not a per-file content hash of `ud1.bin`/`ud2.bin`.
* **The PC recomp, by contrast, *does* hash content**: `asset_setup.cpp` verifies
  `ud1.bin`/`ud2.bin` against a SHA-256 whitelist (`ExpectedBinHash`, lines 72–90 and
  359/420). Patching the containers would fail that check unless the fork adds the new
  hashes (or the check is bypassed). This is exactly why this project's translation uses a
  **runtime hook** instead of patching assets (see `docs/re/runtime-hook.md`).

### 3.3 Streaming vs. memory

* `SOND` banks are resources inside MRON archives; the engine loads a bank and plays the
  `WAVE` its directory points at. The whole bank fits in the container and its data is
  referenced by offset, so an individual dialogue `WAVE` can be swapped independently of
  the music and of the other sounds in the same bank.
* The **music** is a set of bare `AAC ` containers in the gaps, likely streamed/heavy.
  A dialogue-only dub need not touch them.

**Mixed-language dub is plausible:** replace only the `SP`/`FV`/`BTL`/`SITU` dialogue WAVEs,
keep everything else. The per-bank repack is the only friction.

### 3.4 The realistic replacement paths

| Path | Requires | Works on recomp? | Works on real HW? |
|---|---|---|---|
| **A. Re-encode recordings to XMA2**, repack the containers | Microsoft XMA2 encoder + a repack tool | Yes (if you also allow the new hashes) | Yes (with XGD rebuild / RGH) |
| **B. Runtime-substitute PCM/WAV** in the audio layer | A hook in the runtime audio/decoder path | **Yes — this is the natural route** | No (game code on 360 has no such hook) |
| **C. Patch the engine to accept PCM**, store PCM in the container | PowerPC RE + a code cave, or a patched XEX | Possible (C++ weak-alias hooks) | Very hard (XEX patch + audio-code RE) |
| **D. Splice existing XMA2** from another release | Existing XMA2 in the target language | Yes | Yes — **this is how the undub works** |

Path **D** is why the existing *undub* (Japanese voices) succeeded: the Japanese release's
audio is already XMA2, so it can be spliced in without any encoder. **A Spanish dub has no
existing XMA2 source**, so it must go through path **A** (encoder) for on-disk use, or path
**B** for the recomp.

---

## 4. Could it work on an ORIGINAL Xbox 360?

### 4.1 The hardware constraint: XMA is decoded by the console

On a real Xbox 360 the game hands the XMA2 bitstream to the audio stack, which is decoded
**in hardware**. Unlike the PC recomp — which replaces the decoder with Xenia's software
`XmaDecoder` (`rexglue/.../rex/audio/xma/decoder.h`) and can therefore be made to accept
anything — the console *requires a valid XMA2 bitstream in the buffer*. You cannot simply
drop PCM where the game expects XMA. **A real-hardware dub must produce XMA2.**

XMA2 encoding is the crux:

* the only encoder is Microsoft's (`xma2encode.exe` / `xmaencode.exe` / XACT), part of the
  Xbox GDK/XDK, licensed and NDA-bound;
* the format is based on WMA Pro and the bitstream is not produceable by any free encoder;
* consequently, **a legally/practically clean fan dub for original hardware needs access to
  Microsoft's encoder**, which a homebrew project normally cannot obtain.

### 4.2 The three scenarios

**(a) Real retail console from a burned disc.**
A stock console will not boot burned media at all without a modified DVD drive firmware
(LT-type), an ODD emulator, or RGH/JTAG. Even with a flashed drive, rebuilding the image
means regenerating the XDVDFS filesystem with new `ud1.bin`/`ud2.bin` sizes while preserving
the XGD2 security sector and the video-compatibility area, so the drive's stealth checks
still pass. Combined with the XMA2 encoder requirement, this is the least realistic path.

**(b) RGH/JTAG (or Xenia) with patched files.**
RGH/JTAG ignores XEX signatures and can run the game from a hard disk / extracted folder, so
there is no disc-image and no signature problem. That removes two obstacles but **not the
XMA2 encoder**: the on-disk audio still has to be XMA2. Alternatively one could patch the
XEX (PowerPC code cave) to submit **PCM** to XAudio2 instead of XMA — Xbox 360 XAudio2 does
accept PCM — but that requires reverse-engineering the audio-submission code and writing
PPC patches, a far larger effort than the recomp's C++ hooks. In short, on RGH the realistic
route is still path **A** (XMA2 encoding) or the undub-style splice.

**(c) Emulator / PC recomp.**
The PC recomp decodes XMA in software, and this project already hooks recompiled functions
at runtime with weak-alias overrides (`docs/re/runtime-hook.md`, e.g. the text parser
`sub_826D52C0`). The same mechanism can intercept the audio path and play Spanish WAV/PCM
from a folder (keyed by `dirn` name such as `AYA_SP_0001.wav`) without ever encoding XMA and
without touching `ud1.bin`/`ud2.bin`. This is where a fan dub is genuinely feasible.

### 4.3 Why the existing undub is not a counter-argument

The community already ships an **undub** for Infinite Undiscovery (Japanese voices + English
text), and the recomp supports it explicitly: `asset_setup.cpp` recognises a `USA-UNDUB`
edition with its own XEX hashes and container sizes. But that undub **reuses the Japanese
release's own XMA2 audio** — a splice, not a re-encode. It proves the containers can be
repacked and that modified `ud1.bin`/`ud2.bin` run on hardware (on RGH), but it does **not**
show that new audio can be encoded. For Spanish, encoding is unavoidable on real hardware.

---

## 5. Reproduction appendix

All commands are run from the repository root. Tools used are the vendored
`tools/vendor/pc-infiniteundiscovery/tools/`.

```bash
# 1. Disc filesystem
python tools/vendor/pc-infiniteundiscovery/tools/xdvdfs.py list "roms/Infinite Undiscovery (Europe) (Disc 1).iso"

# 2. Container census (already vendored in docs/census.txt)
python tools/vendor/pc-infiniteundiscovery/tools/mron.py scan \
    extract/f2/run/EUROPE/assets/disc1/ud1.bin --csv /tmp/ud1entries.csv

# 3. One SOND bank: structure, names, sizes, self-checks
cd extract/f2/all_d1ud1
python ../../../tools/vendor/pc-infiniteundiscovery/tools/aac.py info 000A4000_023_SOND.bin --limit 15
#   -> size 4173824, version 0x00010003, entries 113, all checks pass
#   -> SYSTEM_001_DECISION.wav mono 48128 Hz 0.383 s ...

# 4. Export sounds as RIFF-wrapped XMA2 and confirm the codec
python ../../../tools/vendor/pc-infiniteundiscovery/tools/aac.py xma \
    000A4000_023_SOND.bin /tmp/xma_test
ffprobe -show_streams /tmp/xma_test/009_SYSTEM_008_SONG_CHANGE.xma | grep codec_name
#   -> codec_name=xma2
ffmpeg -i /tmp/xma_test/009_SYSTEM_008_SONG_CHANGE.xma -f null -   # decodes, exit 0

# 5. ffmpeg has decoders but no encoder
ffmpeg -decoders | grep xma      # xma1, xma2
ffmpeg -encoders | grep -i xma   # (nothing)

# 6. Music is in the gaps, not in SOND
python tools/vendor/pc-infiniteundiscovery/tools/aac.py find \
    extract/f2/run/EUROPE/assets/disc1/ud1.bin --offset 0x28EBD800 --length 143654912
#   -> containers 38, 38 sounds, 87.2 minutes

# 7. Proof some SOND banks are SLZ-compressed on disc
python -c "f=open('extract/f2/run/EUROPE/assets/disc1/ud1.bin','rb'); f.seek(830203392); print(f.read(16))"
#   -> b'SLZ\x04\x00\x00\x00 \x00\x15\xfe\x82\x00\x16\xe8p...'  (SLZ wrapper)
python -c "print(open('extract/f2/all_d1ud1/317BD800_000_SOND.bin','rb').read(16))"
#   -> b'AAC \x00\x16\xe8p\xff\xff...'                            (same sound, decompressed)
```

Additional local facts:

* `assets/disc1/ud1.bin` = 2,207,584,256 B, `ud2.bin` = 2,800,330,752 B (EU/PAL, disc 1).
* `default.exe` strings include `AAC version problem  BGM ID=%d`; no `MRON` or `XAudio`
  strings are present in the decrypted image, and the gap offsets from the census do not
  appear there.
* Recomp runtime audio is the Xenia-derived `XmaDecoder` under
  `extract/f2/rexglue/win-amd64/include/rex/audio/xma/`, with an SDL3 audio driver.

---

## 6. What I could not determine / open questions

1. **Whether the engine uses a global index** for archives (the `0x16000` high-entropy
   prefix of each `ud1.bin`). If it does, a full repack needs to regenerate it. Testable on
   the recomp.
2. **The true dialogue duration.** The ~40 min figure comes from filename heuristics
   (`SP`/`FV`/`BTL`/`SITU`); the boundaries between "voice" and "vocal SFX" are fuzzy.
3. **Whether the PC recomp equivalent on real hardware can be hooked** (path C) without
   re-encoding XMA — this would require PowerPC audio-code RE that was out of scope here.
4. **`WAVE+0x08`** (zero vs `0x995A7C80_00000015`) and the exact semantics of the `PLBK`
   record are still unexplained in the vendored notes; irrelevant to codec choice but
   relevant if a repack tool wants to preserve them.
5. Whether Microsoft's XMA2 encoder output can be dropped in **without** altering the
   engine's expectations at `WAVE+0x08`/`PLBK`; the encoder produces standard XMA2, but the
   game's own header fields (play_begin=384, block size 0x8000, block count) must be
   recomputed by the repack tool.

---

## Resumen (ES)

**Ubicación.** Todo el audio está en contenedores `AAC ` (formato propio de tri-Ace, *no*
MPEG AAC) dentro de los archivos `ud1.bin`/`ud2.bin`. Las voces y efectos viven en recursos
`SOND` (22 601 sonidos, ~288 min en total; ~40 min de diálogo/voz según los nombres). La
música (79 pistas, ~189 min) está **fuera** de los archivos, en los huecos entre archivos
MRON, como contenedores `AAC ` consecutivos.

**Códec.** Es **XMA2** (códec por hardware de Xbox 360): etiqueta de formato `0x0166`,
bloques de `0x8000`, tramas de 512 muestras. Se decodifica perfectamente con ffmpeg
(`codec_name=xma2`), vgmstream o el decodificador software de ReXGlue/Xenia. **No existe
codificador XMA2 libre**: sólo lo trae el GDK/XDK de Microsoft (`xma2encode.exe`,
`xmaencode.exe`, XACT), con licencia y NDA.

**Viabilidad del doblaje.**
* **En el recomp (PC): SÍ y sin cambiar assets.** El recomp ya decodifica XMA por software y
  este proyecto engancha funciones en runtime (igual que la traducción de texto). Se puede
  sustituir el audio por WAV/PCM en español sin codificar XMA y sin tocar `ud1.bin`/`ud2.bin`.
* **En hardware original: sólo si se puede generar XMA2.** La consola decodifica XMA en
  hardware, así que el bitstream debe ser XMA2 válido. Habría que re-codificar (encoder de
  Microsoft) y reconstruir los contenedores + el XDVDFS; en RGH/JTAG además se evita el
  problema de firma, pero **el codificador sigue siendo el cuello de botella**. Un disco
  grabado en consola sin modificar es la vía menos realista.
* **Precedente:** existe un *undub* (voces japonesas) que el recomp admite (`USA-UNDUB`).
  Ese undub **empalma XMA2 ya existente** del lanzamiento japonés, por eso no necesitó
  codificador. Un doblaje al español no tiene XMA2 de origen, así que en hardware real
  obligatoriamente requiere el codificador XMA2.

**Caveats.** No hay checksums en `AAC `/MRON; sólo el `default.xex` va firmado. El recomp
verifica por SHA-256 los `ud1.bin`/`ud2.bin` (por eso el mod de traducción usa hooks y no
parchea assets). Queda sin confirmar si el motor localiza los archivos con un índice global
(el prefijo `0x16000` de cada `ud1.bin` es de alta entropía y no se pudo identificar).

**Recomendación.** Para un fan dub en español, el camino realista es el **recomp**: gancho en
runtime sobre la capa de audio para reproducir WAV/PCM por nombre de sonido
(`AYA_SP_0001.wav`, …), dejando música y efectos intactos. La vía "parchear los `.bin`" sólo
tiene sentido en hardware moderno (RGH/JTAG) y depende de conseguir un codificador XMA2.

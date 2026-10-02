# Infinite Undiscovery Recomp

A native PC recompilation project for **Infinite Undiscovery**.

The goal is pretty simple: extract the portable release, run it, point the setup wizard at your own game media, and let the EXE handle the rest.

No installer. No manual extraction scripts. No game files included.

---

## Current status

The project is now at the point where the game can be launched through a portable EXE with an integrated Asset Setup Wizard.

The wizard can currently:

- detect Infinite Undiscovery media
- detect NTSC-U / PAL automatically
- detect Disc 1 / Disc 2
- extract the required game data
- keep both discs separated correctly
- validate and install the A / B DLC vouchers
- store everything locally next to the EXE
- keep saves, shaders, cache and configuration portable
- skip setup on later launches when the assets are already configured
- switch between English and Spanish

The project does **not** include any copyrighted game files.

You must provide your own legally obtained game media and DLC.

---

## Download

Download the latest portable ZIP from the GitHub Releases page.

Then:

1. Extract the ZIP anywhere you want.
2. Run the EXE.
3. Select your game media when the wizard asks for it.
4. Let the setup finish.
5. Play.

That's it.

The release is intentionally portable.

There is no installer and there are no plans to require one.

---

## Portable by design

Infinite Undiscovery Recomp keeps its runtime data inside the same folder where the EXE is located.

Depending on the detected version of the game, the program creates a local region folder such as:

```text
NTSC-U\
```

or:

```text
PAL\
```

Each region keeps its own data separately.

Example:

```text
InfiniteUndiscoveryRecomp\
├─ InfiniteUndiscoveryRecomp.exe
├─ rexruntime.dll
├─ rexgpu-xenos.dll
│
├─ NTSC-U\
│   ├─ assets\
│   ├─ saves\
│   ├─ shaders\
│   ├─ cache\
│   ├─ logs\
│   └─ config.json
│
└─ PAL\
    ├─ assets\
    ├─ saves\
    ├─ shaders\
    ├─ cache\
    ├─ logs\
    └─ config.json
```

The recomp does not rely on Documents, AppData, Saved Games or old project save paths.

If you move the whole folder, your local setup moves with it.

---

## Asset Setup Wizard

The Asset Setup Wizard is integrated directly into the EXE.

You do not need to run:

- Python scripts
- BAT files
- PowerShell setup scripts
- xdvdfs.py
- xex.py
- external extraction tools

The wizard handles the setup flow for normal users.

It currently supports:

- Disc 1
- Disc 2
- NTSC-U
- PAL
- optional DLC
- English
- Español

English is the default language.

---

## Supported game versions

The current project supports:

- **NTSC-U**
- **PAL**

The region is detected automatically from the selected media.

You do not need to choose the region manually.

NTSC-U and PAL can coexist in the same portable installation without sharing saves or assets.

---

## Multi-disc support

Infinite Undiscovery is a two-disc game.

The setup system keeps Disc 1 and Disc 2 assets separated correctly.

Both discs can be prepared and stored in the portable installation.

### Important

Live disc switching during an actual playthrough still needs full gameplay validation.

The project does not claim that in-game disc swapping is fully solved until it has been tested from start to finish in real gameplay.

---

## DLC

The game has two supported Marketplace Content packages:

- A Voucher
- B Voucher

The wizard validates the DLC before installing it.

The DLC belongs to Infinite Undiscovery Title ID:

```text
535107DB
```

Already-installed valid DLC is reused on later launches instead of being reinstalled every time.

Invalid, foreign or conflicting DLC should still be rejected.

The project does not distribute DLC files.

You must provide your own copy.

---

## Saves

Saves are local to the portable installation.

The project does not automatically load or migrate saves from older development builds, emulator folders or previous runtime paths.

For example:

```text
NTSC-U\saves\
```

and:

```text
PAL\saves\
```

remain independent.

This is intentional.

---

## Known limitations

The project is still under active development.

Current limitations include:

- full-game validation is still ongoing
- live Disc 1 → Disc 2 gameplay switching still needs full validation
- DLC voucher effects still need complete in-game verification
- some runtime/game-specific issues may still appear later in the game

If you find a reproducible issue, open an issue and include as much detail as possible.

---

## Building from source

This repository does **not** include proprietary game assets or generated game code.

You need your own legal copy of Infinite Undiscovery.

The project uses the ReXGlue / XenonRecomp toolchain.

General flow:

1. Clone the repository.
2. Provide the required external toolchain / SDK.
3. Run the code generation step using your own game executable.
4. Configure the project with CMake.
5. Build the desired preset.

The normal release user does not need to do any of this.

This section is only for developers.

More technical setup details are available in:

`ASSET_SETUP.md`

---

## Project history

A development timeline from the first proof of concept to the portable release is available in:

`PROJECT_HISTORY.md`

---

## Credits

This project builds on a lot of work from the Xbox 360 recompilation community.

Special thanks to:

- **Magna** — for major help with development, testing, portability, the Asset Setup Wizard, localization and getting the portable build into a usable state.
- **Premium** — for development help and support throughout the project.
- **ASKA research repository** — for research and technical reference used during development.
- **freefrank** — for the work on Lost Odyssey Recomp and XenonRecomp, which were major references for how I wanted the project to work.
- **ReXGlue / XenonRecomp contributors** — for the tooling and groundwork that make projects like this possible.

More exact repository links and acknowledgements will be added as the project documentation is finalized.

---

## Legal

Infinite Undiscovery is property of its respective copyright holders.

This project is not affiliated with or endorsed by Square Enix, tri-Ace or Microsoft.

No copyrighted game files are included in this repository or in the release package.

You must provide your own legally obtained copy of the game and any optional DLC.

---

## License

The project code is licensed under the BSD-3-Clause license. See `LICENSE` for details.

Game files and external dependencies retain their respective rights and licenses. The project license does not grant any rights to those materials.

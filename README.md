<p align="center">
  <img src="docs/images/banner.png" alt="Infinite Undiscovery Recomp banner" width="100%">
</p>

<h1 align="center">Infinite Undiscovery Recomp</h1>

<p align="center">
  Native Windows x64 recompilation with portable asset setup.
</p>

Infinite Undiscovery Recomp is a native Windows x64 recompilation of **Infinite Undiscovery** (Xbox 360, 2008), built around the **ReXGlue / XenonRecomp** ecosystem.

The project includes an integrated native setup system for importing game data from a legally obtained copy of the game.

No game files, DLC, ISOs, saves or other copyrighted game assets are distributed with this project.

---

## Download

The current public Release Candidate is:

**Infinite Undiscovery Recomp v1.0.0-rc1**

Release page:

https://github.com/doc-haz/infinite-undiscovery-recomp/releases/tag/v1.0.0-rc1

All releases:

https://github.com/doc-haz/infinite-undiscovery-recomp/releases

Download the portable ZIP, extract it to a writable folder and run:

`InfiniteUndiscoveryRecomp.exe`

On first launch, the Asset Setup Wizard will ask for your game media and prepare the required files.

The Windows release is fully portable.

No installer is required.

---

## System requirements

- Windows 10 or Windows 11, 64-bit.
- A Direct3D 12 capable GPU.
- Microsoft Visual C++ Redistributable 2015-2022 (x64).

The runtime uses ReXGlue's `xenos` GPU plugin on top of Direct3D 12:

`rexgpu-xenos.dll`

The release depends on Microsoft runtime files including:

- `MSVCP140.dll`
- `MSVCP140_ATOMIC_WAIT.dll`
- `VCRUNTIME140.dll`
- `VCRUNTIME140_1.dll`

These Microsoft DLLs are **not** bundled with the release.

If Windows reports a missing runtime DLL, install the Microsoft Visual C++ Redistributable 2015-2022 (x64).

---

## Current status

The recomp can currently:

- launch Infinite Undiscovery as a native Windows x64 executable
- import supported USA, USA-UNDUB, EUROPE, JAPAN and ASIA game media
- detect Disc 1 and Disc 2 automatically
- read supported XDVDFS ISO images or extracted disc folders
- keep Disc 1 and Disc 2 assets separate
- validate known game revisions before installation
- perform the real in-game Disc 1 -> Disc 2 transition
- import the supported A Voucher and B Voucher DLC packages
- install DLC through the ReXGlue content system
- keep saves, shaders, cache, logs and configuration local to the portable folder
- reuse an existing validated setup on later launches
- switch between multiple isolated content profiles
- use English or Spanish in the setup UI
- use English or Spanish in the in-game utility UI
- provide an in-game Game Menu and recovery/debug tools
- provide Session / System diagnostic information
- use IU Save Bridge as the official portable save-editor companion

The current public release remains a **Release Candidate** because full-game completion and later-game validation are still ongoing.

---

## Game files

You must provide your own legally obtained copy of Infinite Undiscovery.

For each disc, the setup system expects the required game data:

    default.xex
    ud1.bin
    ud2.bin

The wizard identifies the game from the media itself.

Folder names are not used to determine edition or disc number.

Supported content profiles:

- USA
- USA-UNDUB (Japanese voices, USA text)
- EUROPE
- JAPAN
- ASIA

Legacy portable folder names are migrated automatically on first launch:

    NTSC-U -> USA
    PAL    -> EUROPE

All supported profiles may coexist inside the same portable installation without sharing assets, saves or runtime state.

---

## Portable layout

Runtime data is stored relative to the directory containing the EXE.

Example:

    InfiniteUndiscoveryRecomp\
      InfiniteUndiscoveryRecomp.exe
      rexruntime.dll
      rexgpu-xenos.dll
      README.md
      LICENSE
      THIRD_PARTY_NOTICES.txt
      LICENSES\
      setup.json

      USA\
        assets\disc1\
        assets\disc2\
        assets\dlc\
        saves\
        shaders\
        cache\
        logs\
        config.json

      USA-UNDUB\
        ...

      EUROPE\
        ...

      JAPAN\
        ...

      ASIA\
        ...

Only folders for profiles that are actually configured are created.

Each profile keeps its own:

- assets
- saves
- shaders
- cache
- logs
- configuration

Profiles do not share runtime state.

The recomp does not use Documents, AppData, Saved Games or previous development directories for normal runtime state.

Moving the complete portable folder moves the installation with it.

---

## Asset Setup Wizard

The Asset Setup Wizard is integrated directly into:

`InfiniteUndiscoveryRecomp.exe`

Normal users do not need to run:

- Python extraction scripts
- BAT files
- PowerShell setup scripts
- `xdvdfs.py`
- `xex.py`
- external extraction utilities

The wizard handles:

1. Disc 1 selection and validation
2. optional Disc 2 selection
3. optional DLC selection
4. media verification
5. native asset installation
6. portable configuration
7. profile setup

Disc 1 is required.

Disc 2 is optional during initial setup, but installing it is recommended for a complete playthrough.

English is the default setup language.

Spanish can be selected directly from the wizard.

---

## Content Profile Manager

The project includes a native Content Profile Manager.

Supported profiles:

- USA
- USA-UNDUB
- EUROPE
- JAPAN
- ASIA

The Profile Manager can:

- show installed profiles
- select the active profile
- install additional profiles
- auto-detect supported media
- keep multiple editions isolated inside one portable installation
- return from gameplay to profile selection through the in-game Game Menu

Legacy folders are migrated automatically:

    NTSC-U -> USA
    PAL    -> EUROPE

---

## Multi-disc status

Infinite Undiscovery is a two-disc game.

The setup system can:

- detect Disc 1
- detect Disc 2
- validate both discs
- import both discs
- keep their assets separated
- mount the required content during gameplay

The **real in-game Disc 1 -> Disc 2 transition has been successfully tested multiple times during development**.

This is no longer considered an untested feature.

The game has repeatedly reached the original disc-change point and transitioned into Disc 2 successfully using the recomp runtime.

Current multi-disc validation therefore consists of two separate states:

### Validated

- Disc 1 media detection
- Disc 2 media detection
- Disc 1 import
- Disc 2 import
- separate Disc 1 / Disc 2 asset storage
- runtime Disc 2 mounting
- real in-game Disc 1 -> Disc 2 transition

### Still under validation

- complete Disc 2 progression
- later-game runtime behavior
- complete playthrough to the ending

A complete playthrough has not yet been finished.

---

## In-game Game Menu

Press:

`F5`

to open the Infinite Undiscovery Game Menu.

The menu provides access to runtime information and recovery/debug tools.

Current controls include:

- `F5` — Game Menu
- `F6` — Save Anywhere
- `F8` — Safe Step Forward
- `F9` — Undo Debug Move
- `F10` — Return to Profile Manager
- `F12` — Quit Game

These tools are intended primarily for testing, recovery and diagnostics.

They are not meant to silently alter normal gameplay behavior.

---

## Session / System diagnostics

The in-game Game Menu includes a Session / System information panel.

It can display information such as:

- active profile
- profile display name
- current disc
- project version
- Title ID
- region / edition
- UI language
- DLC package count
- ReXGlue SDK version
- active disc mount
- Disc 1 / Disc 2 presence
- D3D12 backend
- GPU adapter
- window resolution
- Windows version
- logical CPU count
- system RAM
- portable paths
- recovery tool state
- diagnostic trace state
- PSO prewarm / telemetry state

Diagnostic reports can be copied to the clipboard or saved into the active profile logs folder.

User names inside Windows paths are sanitized from exported reports.

---

## DLC

Two Infinite Undiscovery Marketplace Content packages are currently supported:

- A Voucher
- B Voucher

Title ID:

    535107DB

The packages are validated before installation and remain separate.

DLC installation uses the ReXGlue content system rather than a custom content layout.

Already installed valid DLC is reused on later launches.

Different, damaged, incomplete or conflicting DLC is rejected instead of being silently overwritten.

The project does not distribute DLC packages.

---

## Saves

Save data is local to the portable installation.

Examples:

    USA\saves\
    USA-UNDUB\saves\
    EUROPE\saves\
    JAPAN\saves\
    ASIA\saves\

Each content profile keeps its own save data.

Profiles do not share saves automatically.

The recomp does not automatically import saves from:

- older development builds
- emulator directories
- Documents
- AppData
- previous runtime locations

This isolation is intentional.

---

## Official Save Companion

**IU Save Bridge v2.3.0** is the official portable save-editor companion for Infinite Undiscovery Recomp.

Repository:

https://github.com/doc-haz/iu-save-bridge

Latest release:

https://github.com/doc-haz/iu-save-bridge/releases/latest

Current profile model:

- USA
- USA-UNDUB
- EUROPE
- JAPAN
- ASIA

Legacy compatibility:

    NTSC-U -> USA
    PAL    -> EUROPE

IU Save Bridge v2.3.0 supports:

- direct save editing
- Fol editing
- character stats editing
- inventory editing for 1,023 items
- automatic save discovery
- Xbox 360 / STFS save import
- profile-aware save discovery
- automatic safety backups before writing
- manual backup support
- safe backup restore
- dual Tri-Ace CRC32 recalculation
- SHA-256 verification
- English / Spanish interface
- fully portable operation
- no installer
- no AppData, Documents or Registry dependency
- exclusion of non-playable `saves\achievements\` data from save-slot discovery

Backups are stored by profile:

    backups\USA\
    backups\USA-UNDUB\
    backups\EUROPE\
    backups\JAPAN\
    backups\ASIA\

Legacy backups remain visible without being renamed or moved.

Real retail save validation has currently been performed with:

- USA
- legacy NTSC-U mapped to USA

USA-UNDUB, EUROPE, JAPAN and ASIA save handling is currently validated through the shared format and synthetic fixtures.

The editor does not assume that saves from different editions are interchangeable merely because they share the same general format.

---

## Validation

The current setup/profile implementation has been tested with:

- USA Disc 1
- USA Disc 2
- EUROPE (PAL) Disc 1
- EUROPE (PAL) Disc 2
- USA-UNDUB media
- JAPAN media
- ASIA media
- automatic profile detection
- legacy NTSC-U -> USA migration
- legacy PAL -> EUROPE migration
- Disc 1 / Disc 2 detection
- real in-game Disc 1 -> Disc 2 transition
- mixed-region rejection
- swapped-disc rejection
- invalid and truncated media rejection
- modified XEX rejection
- foreign Title ID rejection
- A Voucher validation
- B Voucher validation
- duplicate DLC rejection
- corrupt DLC rejection
- foreign DLC rejection
- DLC installation through ReXGlue
- DLC reuse on subsequent launches
- conflicting installed DLC preservation
- setup cancellation and staging cleanup
- portable execution outside the development directory
- startup with a working directory different from the EXE directory
- save loading
- normal gameplay
- profile switching
- clean shutdown

Automated tests are developer validation tools and do not replace complete gameplay testing.

---

## Vesplume Tower / Orb of Patience

One important active gameplay investigation is the known **Vesplume Tower / Orb of Patience softlock**.

A community save reproducing the problem has been provided.

The issue is currently being used as a regression and diagnostic case for:

- progression state
- story flags
- runtime behavior
- scheduler behavior
- recovery tools
- later Disc 1 progression

The project does **not** currently claim that this issue is fixed.

Community reports, alternative reproduction cases and diagnostic information are welcome.

---

## Known limitations

The following remain under active validation:

- complete full-game playthrough
- complete Disc 2 progression
- late-game runtime issues
- Vesplume Tower / Orb of Patience softlock
- complete in-game verification of the A Voucher and B Voucher effects
- hardware / driver-specific graphical issues
- unusual aspect ratios and ultrawide behavior

The Disc 1 -> Disc 2 transition itself has been successfully tested multiple times and is **not** currently considered an unresolved limitation.

Asset preparation and successful disc switching still do not imply that every later section of the game has already been validated.

---

## Runtime and release engineering

The public RC uses:

**ReXGlue v0.10.0**

Source revision:

    f5337cdc947ff6d4c4196737e2c807a48f2a1fc2

The public runtime DLLs were rebuilt from the audited ReXGlue v0.10.0 source rather than using the older precompiled SDK binaries.

Runtime compatibility was checked against the symbols consumed by the recomp and Xenos GPU plugin.

The release pipeline includes a hash guard that rejects the previously used runtime DLLs.

Final public artifacts were scanned with Microsoft Defender before release.

---

## Third-party licensing and compliance

The project includes third-party license information under:

    LICENSES\
    THIRD_PARTY_NOTICES.txt

The ReXGlue runtime statically incorporates LGPL components including FFmpeg and libmspack.

A separate LGPL compliance package is published with the RC release.

It contains corresponding source and relink material required for those statically linked LGPL components.

Current compliance asset:

`InfiniteUndiscoveryRecomp-v1.0.0-rc1-LGPL-Compliance.zip`

The main portable game ZIP remains separate from the compliance archive.

---

## Building from source

This repository does not include proprietary game assets or generated game code.

A developer build requires:

- a legally obtained supported Infinite Undiscovery executable
- ReXGlue SDK / source tree
- CMake
- Ninja
- Clang
- MSVC / Windows SDK environment

General build flow:

1. Clone the repository.
2. Provide the required ReXGlue toolchain and SDK/source tree.
3. Generate the game code locally from your own supported executable.
4. Configure the project with CMake.
5. Build the Windows x64 target.

The generated game code is intentionally excluded from the public repository.

More details about setup are available in:

`ASSET_SETUP.md`

Development history is documented in:

`PROJECT_HISTORY.md`

---

## Development history

The project began as a small experiment:

> Can Infinite Undiscovery run as a native Windows recompilation?

It gradually evolved into a portable native Windows application with:

- multi-edition content profiles
- integrated asset setup
- profile management
- native DLC handling
- portable runtime state
- multi-disc support
- tested Disc 1 -> Disc 2 transition
- recovery tools
- diagnostics
- bilingual UI
- a dedicated save-editor companion
- audited runtime dependencies
- reproducible release/compliance work

The detailed timeline is preserved in:

`PROJECT_HISTORY.md`

Historical sections intentionally retain old designs and old validation states when they accurately describe that stage of development.

---

## Credits

This project builds on work from the Xbox 360 recompilation community.

Special thanks to:

- **Magna** — for major help with development, testing, portability, the Asset Setup Wizard, localization and preparing the portable build.
- **Premium** — for development help and support throughout the project.
- **Diesel** — for extensive help with source auditing, ReXGlue rebuild verification, ABI compatibility checks, release engineering, compliance validation and final RC1 preparation.
- **[vs-sr-dev / pc-infiniteundiscovery](https://github.com/vs-sr-dev/pc-infiniteundiscovery)** — for extensive reverse-engineering research, documentation and analysis tools for Infinite Undiscovery and the ASKA engine.
- **[dotslash (freefrank) / LostOdysseyRecomp](https://github.com/freefrank/LostOdysseyRecomp)** — for the Lost Odyssey native PC recompilation project and its importer and portable workflow, which were useful references during development.
- **ReXGlue / XenonRecomp contributors** — for the tooling and groundwork that make projects like this possible.
- Everyone testing the recomp, reporting bugs, providing saves and documenting reproduction cases.

---

## Legal

Infinite Undiscovery is property of its respective copyright holders.

This project is unofficial and is not affiliated with or endorsed by Square Enix, tri-Ace or Microsoft.

No copyrighted game files, DLC, ISOs or saves are included in this repository or release package.

You must provide your own legally obtained copy of the game and any optional DLC.

---

## License

The project code is licensed under the BSD-3-Clause license.

See:

`LICENSE`

Game files and external dependencies retain their respective rights and licenses.

The project license does not grant any rights to those materials.

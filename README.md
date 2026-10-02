# Infinite Undiscovery Recomp

Native PC recompilation of **Infinite Undiscovery** for Windows x64.

The current build uses ReXGlue / XenonRecomp and includes a native setup wizard for importing game data from a legally obtained copy of the game.

No game files, DLC, ISOs or other copyrighted assets are distributed with this project.

## Download

The latest portable build is available from:

https://github.com/doc-haz/infinite-undiscovery-recomp/releases/latest

Extract the ZIP to a writable folder and run:

`InfiniteUndiscoveryRecomp.exe`

On first launch, the Asset Setup Wizard will ask for your game media and prepare the required files.

The Windows release is portable. No installer is required.

## Current status

The recomp can currently:

- launch Infinite Undiscovery as a native Windows executable
- import supported PAL and NTSC-U game media
- detect Disc 1 and Disc 2 automatically
- read supported XDVDFS ISO images or extracted disc folders
- keep Disc 1 and Disc 2 assets separate
- validate known game revisions before installation
- import the supported A Voucher and B Voucher DLC packages
- install DLC through the ReXGlue content system
- keep saves, shaders, cache, logs and configuration local to the portable folder
- reuse an existing validated setup on later launches
- use English or Spanish in the setup UI

The current public release is still under active gameplay validation.

## Game files

You must provide your own legally obtained copy of Infinite Undiscovery.

For each disc, the setup system expects the required game data:

    default.xex
    ud1.bin
    ud2.bin

The wizard identifies the game from the media itself. Folder names are not used to determine region or disc number.

Supported regions:

- NTSC-U
- PAL

Both regions may exist in the same portable directory without sharing assets or saves.

## Portable layout

Runtime data is stored relative to the directory containing the EXE.

Example:

    InfiniteUndiscoveryRecomp\
    ├─ InfiniteUndiscoveryRecomp.exe
    ├─ rexruntime.dll
    ├─ rexgpu-xenos.dll
    ├─ setup.json
    │
    ├─ NTSC-U\
    │   ├─ assets\
    │   │   ├─ disc1\
    │   │   ├─ disc2\
    │   │   └─ dlc\
    │   ├─ saves\
    │   ├─ shaders\
    │   ├─ cache\
    │   ├─ logs\
    │   └─ config.json
    │
    └─ PAL\
        ├─ assets\
        │   ├─ disc1\
        │   ├─ disc2\
        │   └─ dlc\
        ├─ saves\
        ├─ shaders\
        ├─ cache\
        ├─ logs\
        └─ config.json

The recomp does not intentionally use Documents, AppData, Saved Games or previous development directories for normal runtime state.

Moving the complete portable folder moves the installation with it.

## Asset Setup Wizard

The Asset Setup Wizard is integrated into `InfiniteUndiscoveryRecomp.exe`.

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
5. asset installation
6. portable configuration

Disc 1 is required.

Disc 2 is supported and recommended, but live Disc 1 → Disc 2 switching during an actual playthrough still requires full gameplay validation.

English is the default setup language. Spanish can be selected from the wizard.

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

## Saves

Save data is local to the portable installation.

Examples:

    NTSC-U\saves\
    PAL\saves\

PAL and NTSC-U saves remain separate.

The project does not automatically import saves from older development builds, emulator directories or previous runtime locations.

This is intentional.

## Validation

The current Asset Setup implementation has been tested with:

- PAL Disc 1
- PAL Disc 2
- NTSC-U Disc 1
- NTSC-U Disc 2
- PAL / NTSC-U detection
- Disc 1 / Disc 2 detection
- mixed-region rejection
- swapped-disc rejection
- invalid and truncated media
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

The automated setup tests are developer validation tools and do not replace a complete gameplay test.

## Known limitations

The following are still being validated:

- full-game completion
- live Disc 1 → Disc 2 switching during an actual playthrough
- complete Disc 2 gameplay
- complete in-game verification of the A Voucher and B Voucher effects
- runtime issues that may only appear later in the game

Please do not assume a feature is fully validated simply because the setup system can prepare the required files.

## Building from source

This repository does not include proprietary game assets or generated game code.

A developer build requires:

- a legally obtained supported Infinite Undiscovery executable
- ReXGlue SDK
- CMake
- Ninja
- Clang
- MSVC / Windows SDK environment

General build flow:

1. Clone the repository.
2. Provide the required ReXGlue toolchain and SDK.
3. Generate the game code locally from your own supported executable.
4. Configure the project with CMake.
5. Build the Windows x64 target.

The generated game code is intentionally excluded from the public repository.

More details about the setup implementation are available in:

`ASSET_SETUP.md`

Development history is documented in:

`PROJECT_HISTORY.md`

## Credits

This project builds on work from the Xbox 360 recompilation community.

Special thanks to:

- **Magna** — for major help with development, testing, portability, the Asset Setup Wizard, localization and preparing the portable build.
- **Premium** — for development help and support throughout the project.
- **[vs-sr-dev / pc-infiniteundiscovery](https://github.com/vs-sr-dev/pc-infiniteundiscovery)** — for extensive reverse-engineering research, documentation and analysis tools for Infinite Undiscovery and the ASKA engine.
- **[freefrank / LostOdysseyRecomp](https://github.com/freefrank/LostOdysseyRecomp)** — for the Lost Odyssey native PC recompilation project, whose importer, portable workflow and user-facing structure served as useful references during development.
- **ReXGlue / XenonRecomp contributors** — for the tooling and groundwork that make projects like this possible.

## Legal

Infinite Undiscovery is property of its respective copyright holders.

This project is not affiliated with or endorsed by Square Enix, tri-Ace or Microsoft.

No copyrighted game files are included in this repository or in the release package.

You must provide your own legally obtained copy of the game and any optional DLC.

## License

The project code is licensed under the BSD-3-Clause license. See `LICENSE` for details.

Game files and external dependencies retain their respective rights and licenses. The project license does not grant any rights to those materials.

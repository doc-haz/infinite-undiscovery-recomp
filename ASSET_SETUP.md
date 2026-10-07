# Native Asset Setup

Infinite Undiscovery Recomp includes a native Asset Setup Wizard directly inside `InfiniteUndiscoveryRecomp.exe`.

The normal user does not need Python, BAT files, PowerShell setup scripts, `xdvdfs.py`, `xex.py`, or external extraction utilities.

The application is designed as a portable Windows x64 release.

The EXE, runtime DLLs, game assets, saves, shaders, cache, logs and configuration remain inside the portable application directory.

---

## Normal user flow

1. Extract the portable release ZIP anywhere.
2. Run `InfiniteUndiscoveryRecomp.exe`.
3. If no valid local game installation exists, the Asset Setup Wizard opens automatically.
4. Select Infinite Undiscovery Disc 1.
5. Select Disc 2 when requested.
6. Optionally select the supported DLC packages.
7. Confirm setup.
8. The wizard validates and extracts the required files.
9. The game starts.
10. Future launches reuse the validated local installation and skip the wizard unless maintenance/setup is requested.

The user does not manually select the region or content profile.

The application determines the supported game region from the selected media.

---

## Portable layout

All runtime state is stored relative to the directory containing the EXE.

Each supported content profile uses an independent root:

    USA\
    USA-UNDUB\
    EUROPE\
    JAPAN\
    ASIA\

Legacy folder names are migrated automatically on first launch (`NTSC-U` becomes
`USA`; `PAL` becomes `EUROPE`).

Example:

    InfiniteUndiscoveryRecomp\
      InfiniteUndiscoveryRecomp.exe
      rexruntime.dll
      rexgpu-xenos.dll
      setup.json
      USA\                    (default profile; legacy NTSC-U migrates here)
        assets\disc1\
        assets\disc2\
        assets\dlc\
        saves\
        shaders\
        cache\
        logs\
        config.json
      USA-UNDUB\              (same layout as USA)
      EUROPE\                 (legacy PAL migrates here; same layout)
      JAPAN\                  (same layout)
      ASIA\                   (same layout)

The application does not intentionally use Documents, AppData, LocalAppData, Saved Games, old development directories or emulator folders for its normal runtime state.

Moving the complete portable directory moves the installation with it.

Content profiles remain isolated from each other; assets and saves are never shared.

---

## Save isolation

Each supported content profile has its own save directory.

Examples:

    USA\saves\
    USA-UNDUB\saves\
    EUROPE\saves\
    JAPAN\saves\
    ASIA\saves\

The region `saves` folder is configured as ReXGlue's `user_data_root`. As a result, in addition to game save data, it also hosts runtime-managed user content such as installed DLC packages (`0000000000000000/535107DB/...`).

Old saves from previous development builds are not automatically imported or discovered.

There is currently no automatic save migration system.

This behavior is intentional.

---

## Supported media

The native media reader supports the tested USA, USA-UNDUB, EUROPE, JAPAN and ASIA Infinite Undiscovery Xbox 360 releases.

The required root files are:

    default.xex
    ud1.bin
    ud2.bin

Infinite Undiscovery is a two-disc game.

Disc 1 and Disc 2 are validated separately and their assets remain separate.

They must never overwrite each other.

---

## Game identity

The expected Infinite Undiscovery Title ID is:

    535107DB

The native reader inspects XEX2 execution metadata and Xbox 360 media information rather than trusting folder names or release labels.

Supported region masks include:

    PAL:     00FF0000
    NTSC-U:  000000FF
    NTSC-J:  0000FD00  (JAPAN / ASIA disambiguated by multidisc IDs)

Disc identity and multidisc metadata are also checked.

Mixed-region disc pairs, incorrect disc slots and unsupported media are rejected.

---

## Supported Disc 1 XEX profiles

The currently validated Disc 1 XEX SHA-256 profiles are:

EUROPE (PAL):

    22893bb8d96a1440ecbdbcae543baeaf89d26588c89c99a2c96fecf611475325

USA:

    9523b45e6a724d4988ce9cf70d55b672e461b89303b02dc2f0f9c84329c25555

Additional validated Disc 1 profiles (USA-UNDUB, JAPAN, ASIA):

    42dfaa90814f5b78592bb637ac371df20c30acfa897f922fff96d3251553473b
    cbb789e5e8842398253839b2e320874b3851bd10aaa2392aadf5cdeed438cc91
    fd063de17d98ab1201795efab4ffd06d76e85eeb4da0ce751926792deeeccd33

The decrypted PE payloads of these tested builds were compared during development and produced the same SHA-256:

    6b040cd127c89d64041d64e8ea814b388f1aba036e672d05c9d51472c0a308ea

They also matched in the executable sections and addresses relevant to the recompilation work.

This is why the existing generated function map can support both tested regions without maintaining a separate recompilation map for each one.

This does not imply compatibility with arbitrary or unknown XEX revisions.

Unsupported XEX builds are rejected.

---

## BIN validation

The large game BIN files are copied separately for each disc.

Their expected sizes and SHA-256 hashes are checked during installation.

The validated disc sets for the supported profiles passed these checks during development.

The setup system records installation metadata so later launches do not need to hash the complete multi-gigabyte game data every time.

Runtime fingerprints are used to detect unexpected changes to an existing validated installation.

These fingerprints are intended as change detection, not as a cryptographic trust or DRM system.

---

## Disc 1 and Disc 2

The setup wizard preserves both discs independently.

Conceptually:

    <REGION>\assets\disc1\
        default.xex
        ud1.bin
        ud2.bin

    <REGION>\assets\disc2\
        default.xex
        ud1.bin
        ud2.bin

Disc 2 files are never merged into Disc 1.

Preparing both discs successfully does not by itself prove that every live Disc 1 to Disc 2 transition works during a complete playthrough.

Full in-game disc switching remains a gameplay validation item.

---

## DLC

Two supported Infinite Undiscovery Marketplace Content packages were identified during development:

    A Voucher
    License mask: 0x1

    B Voucher
    License mask: 0x10

Both use:

    Title ID: 535107DB
    Content type: 00000002

A Voucher and B Voucher are independent packages.

They are not treated as interchangeable content.

The project does not distribute these packages.

Users must provide their own legally obtained copies.

---

## DLC validation

The native STFS inspection path validates package identity and structure before installation.

Validation includes relevant container metadata, Title ID, content type, package identity and content integrity.

The currently supported path was developed around the tested Infinite Undiscovery A Voucher and B Voucher packages.

Unsupported or malformed STFS layouts are rejected instead of being guessed or silently accepted.

The validation system does not establish ownership or acquisition rights for DLC.

---

## DLC installation

DLC is installed through the ReXGlue content system rather than by inventing a custom fake Xbox 360 content layout.

The runtime uses:

    ContentManager::InstallContent

The two packages retain their separate identities and license masks.

Installation is designed to be idempotent.

Expected behavior:

    First launch:
    validate → install → launch

    Later launch:
    detect same valid installed content → reuse → launch

An already-installed package that matches the validated source should not be reinstalled or reported as a conflict on every launch.

A genuinely different, incomplete, altered or corrupt package should still cause a conflict or validation failure.

The goal is to reuse identical valid content without weakening validation.

---

## DLC region behavior

No PAL/NTSC-U-specific region metadata was identified in the tested DLC packages.

During development, package loading and SDK enumeration were successfully exercised with both supported Disc 1 XEX variants.

Complete in-game verification of the actual voucher effects remains a separate gameplay validation item.

---

## Cancellation and staging

Asset installation uses staging so incomplete setup operations are not intentionally published as valid installations.

If the user cancels a handled setup operation, staging data owned by that operation is cleaned up.

Source game media is treated as read-only.

The setup process should never modify the user's original disc image, extracted disc folder or DLC source package.

As with any file operation, an unexpected process termination or power loss can leave partial temporary data.

The next run should validate the installation rather than silently trusting incomplete output.

---

## Setup maintenance

Normal first-time setup requires only launching the EXE.

The application can also expose the Asset Setup Wizard again for maintenance through the existing asset-setup runtime option.

The application can expose the Asset Setup Wizard again for maintenance through the existing asset-setup runtime option.

This option is not required for normal portable use.

---

## Language

The Asset Setup Wizard supports:

    English
    Español

English is the default language.

The language selection is stored as part of the portable configuration.

The user can change language directly from the wizard.

---

## User interface

The setup UI is implemented natively for Windows.

The current design uses a dark fantasy/JRPG-inspired presentation with:

- Disc 1 / Disc 2 / DLC progress
- validation status
- region/media feedback
- English / Español selection
- native file selection
- setup navigation controls

The UI does not depend on bundled official Infinite Undiscovery artwork.

Decorative elements are generated or implemented as part of the application UI.

---

## Build requirements

The project uses the existing ReXGlue / XenonRecomp-based Windows build environment.

The current Windows build requires the appropriate developer toolchain, including the ReXGlue SDK, CMake, Ninja/Clang and the Windows/MSVC SDK environment used by the project.

Generated game code is not stored in the public repository.

A developer must provide their own legally obtained supported game executable and run the project's code-generation step before configuring a clean build.

The existing build flow uses the repository CMake presets.

The Windows output executable is:

    InfiniteUndiscoveryRecomp.exe

The required ReXGlue runtime DLLs remain separate application dependencies.

---

## Code generation

The public repository does not contain generated Infinite Undiscovery game code.

Code generation must be performed locally from a supported legally obtained game executable.

The tested Disc 1 executable payloads across the supported profiles are equivalent for the function map used by the project.

The current project configuration therefore does not maintain independent generated function maps per profile.

Unknown executable revisions should not be assumed compatible.

---

## Setup tests

Developer setup tests can be enabled with:

    -DIU_BUILD_SETUP_TESTS=ON

The setup tests use the same implementation sources as the application.

Legal local media fixtures are supplied by the developer and are not committed to the repository.

Validation work has covered cases including:

- wizard cancellation
- public EXE cancellation
- profile detection (USA, USA-UNDUB, EUROPE, JAPAN, ASIA)
- legacy NTSC-U / PAL migration
- Disc 1 / Disc 2 detection
- swapped disc rejection
- mixed-region rejection
- invalid XEX rejection
- truncated XEX rejection
- foreign Title ID rejection
- complete extraction per supported profile
- BIN hash verification
- separate Disc 1 / Disc 2 destinations
- A Voucher validation
- B Voucher validation
- corrupt DLC rejection
- foreign DLC rejection
- duplicate/conflicting DLC behavior
- DLC installation
- DLC idempotence on later launches
- cancellation cleanup
- portable settings round-trip
- direct startup with already-configured assets

These tests are developer validation tools.

They are not prerequisites for normal users.

---

## What is not claimed yet

The current asset/setup implementation should not be interpreted as proof that the entire game has been completed from start to finish under the recomp.

The following remain explicit gameplay validation items:

- full-game playthrough validation
- live Disc 1 to Disc 2 switching during actual gameplay
- complete Disc 2 gameplay validation
- complete in-game verification of A Voucher and B Voucher effects
- runtime issues that may only appear much later in the game

The setup system and automated tests do not replace those gameplay tests.

---

## Repository policy

The public repository does not contain:

- Infinite Undiscovery ISOs
- `default.xex`
- `ud1.bin`
- `ud2.bin`
- DLC packages
- saves
- generated proprietary game code
- extracted copyrighted game assets
- local build outputs
- local diagnostic captures

The repository contains only the project code and documentation needed to build and develop the recompilation.

No commit or release should add proprietary game content.

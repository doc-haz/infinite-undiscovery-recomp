# Infinite Undiscovery Recomp — Project History

This document records the development history of **Infinite Undiscovery Recomp**,
from the first question of whether the Xbox 360 game could run as a native Windows
recompilation to the first public Release Candidate.

It is not intended to be a commit-by-commit changelog.

Instead, it documents the major milestones, experiments, wrong turns, technical
problems, design decisions, validation work, tooling changes and breakthroughs that
shaped the project.

Where an old design was later replaced, it is kept here as part of the historical
record rather than rewritten as though the final architecture had existed from the
beginning.

---

# September 2026 — The project begins

Infinite Undiscovery Recomp began with a simple question:

> Can Infinite Undiscovery run as a native Windows executable?

The project was built around the **ReXGlue / XenonRecomp** ecosystem.

The initial goals were deliberately modest:

1. Understand the Xbox 360 executable.
2. Generate native code.
3. Make the generated project compile.
4. Get the game to start.
5. Investigate whatever failed next.

From the beginning, one rule was fixed:

> **The repository would not contain proprietary Infinite Undiscovery game assets.**

Users would always need to provide their own legally obtained copy of the game.

The first repository commit was:

    d017e263
    Initial commit

At this point the repository was primarily a development environment containing
the recompilation project structure, build configuration, manifest and early
runtime support.

It was not yet something a normal player could download and use.

---

# Early media investigation

Development initially focused on the PAL release.

One reason was the expectation that the PAL version might provide the desired
language support.

Inspection of the Xbox 360 media identified the important game files:

    default.xex
    ud1.bin
    ud2.bin

Infinite Undiscovery is a two-disc game, and each disc contains its own copies of
these files.

That detail became important later because Disc 1 and Disc 2 assets cannot simply
overwrite each other.

Both sets must remain independently available.

---

# First native builds

The project eventually reached the point where the generated C++ could be compiled
into a native Windows executable.

The first builds were highly developer-oriented.

Game assets had to be prepared manually and paths were supplied directly to the
runtime.

There was:

- no setup wizard
- no profile manager
- no portable installation layout
- no normal end-user setup flow

Even so, this was the first major milestone:

> **Infinite Undiscovery was no longer only being analyzed. It was executing as a
> native PC recompilation.**

---

# Runtime bring-up

Getting the project to compile was only the beginning.

The next phase became a long runtime bring-up effort involving areas such as:

- guest memory initialization
- Xbox 360 runtime behavior
- Xenos GPU command processing
- synchronization
- frame progression
- fibers
- diagnostics
- shader handling
- game-specific hooks
- runtime tracing
- frame pacing

The game gradually progressed further instead of failing immediately.

At one stage, execution repeatedly reached roughly frame 309 before encountering
an unregistered guest function near:

    0x821F2360

Addresses such as this became individual investigation targets.

The fundamental development question had changed.

It was no longer:

> Can the executable run?

It had become:

> Which remaining runtime or game system is stopping it now?

---

# Graad Prison

One of the first major gameplay validations was reaching **Graad Prison** in the
native recompilation.

The historical build was launched manually and verified running actual game
content.

This demonstrated that the project had progressed far beyond a boot-screen or
menu-only proof of concept.

---

# PoC 0.1

The first historical proof-of-concept milestone was preserved as:

    86eaeca
    Add Infinite Undiscovery Recomp Preliminary PoC 0.1

PoC 0.1 contained working pieces including:

- native recompilation
- startup
- frame pacing
- diagnostics
- game hooks
- fiber support
- game asset path configuration

PoC 0.1 became a permanent historical baseline.

A project rule was adopted:

> **PoC 0.1 should remain historical and must not be rewritten retroactively.**

Later versions would move forward rather than pretending the early build had
features that did not exist yet.

---

# PAL vs NTSC-U investigation

The PAL version was originally expected to provide the desired Spanish-language
experience.

Testing did not produce the expected result, so attention shifted toward the North
American NTSC-U release.

That comparison led to an important discovery:

## PAL and NTSC-U use the same relevant executable payload

After decrypting and comparing the executable payloads, the relevant PE content
was found to be identical.

The versions shared:

- the same executable code
- the same `.text`
- the same `.rdata`
- the same `.data`
- the same relevant function addresses
- the same hook locations

Differences were primarily in Xbox 360 media/header information such as:

- region
- certification
- disc metadata

This meant the native recompilation work did **not** need to be duplicated for PAL
and NTSC-U.

One recompilation could support both media variants.

That discovery later made automatic edition detection possible.

---

# NTSC-U media validation

The North American discs were inspected independently.

Title ID:

    535107DB

Disc 1 and Disc 2 both contain:

    default.xex
    ud1.bin
    ud2.bin

The large BIN files were hashed and verified independently.

An important rule came out of this work:

> **Never merge or overwrite Disc 1 and Disc 2 BIN files.**

Both discs must remain independently available to the runtime.

---

# Development extraction tools

During early development, Xbox 360 media was prepared with utilities including:

    xdvdfs.py
    xex.py

These tools were extremely useful for research.

They could inspect Xbox 360 disc images, extract files and decrypt or inspect the
XEX.

However, requiring a normal user to run development scripts manually was never
considered an acceptable final experience.

This eventually became one of the strongest motivations for the integrated setup
system.

---

# Multi-disc investigation

Infinite Undiscovery uses two game discs.

The project became capable of preparing and preserving both Disc 1 and Disc 2
asset sets.

However, an important distinction was made very early:

> Having Disc 2 installed is not the same thing as proving live disc switching
> during an actual playthrough.

For that reason, runtime disc switching remained a separate validation target.

The project would not claim successful Disc 1 → Disc 2 transition behavior until
it had been demonstrated in real gameplay.

---

# DLC investigation

The game's downloadable content was investigated separately from the disc assets.

Two Marketplace Content packages were identified:

    A Voucher
    License mask: 0x1

and:

    B Voucher
    License mask: 0x10

Both belong to:

    Title ID: 535107DB
    Content type: 00000002

The packages had to remain distinct.

They could not safely be treated as interchangeable simply because some internal
resources appeared similar.

---

# ReXGlue STFS support

An important discovery was that ReXGlue already contained proper Xbox 360 STFS
content support.

The recomp could therefore use:

    ContentManager::InstallContent

instead of inventing a custom DLC layout.

This became the basis of the final DLC installation architecture.

---

# October 1, 2026 — The project changes direction

By October 1, the game itself was running well enough that another weakness had
become obvious:

> **The setup experience was terrible for a normal user.**

Development still required knowledge of internal Xbox 360 files, extraction
utilities and manual paths.

The project therefore adopted a new usability goal, influenced in part by the
portable workflow used by projects such as **Lost Odyssey Recomp**:

    Download
    → Run EXE
    → Select your own game media
    → Let the application prepare everything
    → Play

The intended end-user workflow should require:

- no Python
- no BAT setup
- no PowerShell setup
- no manual `xdvdfs.py`
- no manual `xex.py`
- no knowledge of XEX internals
- no knowledge of XDVDFS
- no knowledge of Title IDs
- no manual handling of `ud1.bin`
- no manual handling of `ud2.bin`
- no knowledge of Xbox 360 STFS package structure

This became the **Asset Setup Wizard** project.

---

# Asset Setup Wizard

The setup system was integrated directly into the recomp executable.

The first complete implementation could:

- detect Infinite Undiscovery media
- identify PAL / NTSC-U
- identify Disc 1 / Disc 2
- extract game assets natively
- keep both discs separated
- validate optional DLC
- install DLC through ReXGlue
- persist configuration
- reuse an existing installation on later runs
- cancel cleanly without launching the game

Both PAL and NTSC-U extraction paths were validated.

This was the point where Infinite Undiscovery Recomp began to feel less like a
development experiment and more like a standalone PC application.

---

# First integrated preview

The first wizard-enabled preview compiled successfully and could launch the game
after preparing the user's own media.

If valid assets were already installed, the executable correctly skipped setup and
continued into normal startup.

The build contained only application/runtime files.

It did **not** contain:

- game assets
- ISOs
- copyrighted DLC
- saves from development

The historical PoC work, hooks, diagnostics and fiber changes remained intact.

---

# The portability problem

Testing the integrated preview exposed a new problem.

The initial setup created generated installation directories similar to:

    InfiniteUndiscovery-USA-127528054305200-0

Technically this worked, but it was not suitable as a clean user-facing structure.

A more serious issue appeared immediately afterward.

The recomp discovered several old save files created by the previous development
runtime.

Those saves lived outside the new installation.

That behavior was considered unacceptable.

A new portable build should not silently discover state from an old development
environment.

---

# Portable-only becomes a permanent project rule

A permanent design decision was made:

> **Infinite Undiscovery Recomp will be fully portable.**

The directory containing the executable became the installation root.

Normal runtime state should not depend on:

- Documents
- My Documents
- AppData
- LocalAppData
- Saved Games
- emulator save directories
- old development paths
- previous recomp installations

The portable folder itself is the installation.

No traditional installer is required.

No registry-based installation is required.

The entire application can be moved or backed up simply by moving or copying its
folder.

---

# Historical two-region portable layout

The first portable design used two region folders:

    NTSC-U\
    PAL\

Each region contained independent runtime state:

    assets\
    saves\
    shaders\
    cache\
    logs\
    config.json

This was an important intermediate architecture because it established the principle
that different game variants must not share runtime state.

Later in development this two-region model was replaced by the final multi-profile
architecture.

The historical `NTSC-U` and `PAL` names are preserved in this document because
they accurately describe that stage of development.

---

# Save isolation

The portable redesign also changed save behavior.

New saves belonged only to the portable installation.

Older saves from Documents, emulator directories or previous development runtimes
were deliberately ignored.

There was no fallback discovery.

This prevented a fresh portable build from unexpectedly loading data created by an
older experimental runtime.

The same isolation principle was later extended to every supported content profile.

---

# DLC second-launch bug

The first DLC-enabled portable preview exposed another issue.

On the first launch, DLC installed correctly.

On the second launch, the runtime attempted to process the already-installed
content again and reported a conflict involving existing DLC metadata.

The packages were not actually different.

The problem was eventually traced to comparison behavior involving
padding/non-deterministic data in the installed content representation.

DLC handling was corrected to become idempotent.

The intended behavior became:

    First launch:
    validate → install → launch

    Later launches:
    detect same valid DLC → reuse → launch

Different, damaged or conflicting packages should still be rejected.

The fix did not weaken validation.

It simply stopped valid already-installed content from being incorrectly treated as
a conflict.

---

# English and Spanish support

The original integrated wizard was written primarily in Spanish.

Rather than replacing Spanish with English, the project adopted bilingual support:

    English | Español

English became the default.

The selected language was persisted.

User-facing strings were moved toward centralized localization instead of remaining
scattered as hard-coded dialog text.

Later this bilingual system was expanded to runtime menus and diagnostics.

---

# Wizard visual redesign

The first setup wizard was functional but visually minimal.

It looked like a standard white native Windows utility.

That was acceptable during early testing but did not match the identity of the
project.

A visual redesign introduced:

- dark midnight-blue presentation
- fantasy/JRPG atmosphere
- Infinite Undiscovery Recomp branding
- Disc 1 → Disc 2 → DLC progression
- validation status
- English / Español selection
- dark content panels
- decorative fantasy elements
- cleaner spacing and hierarchy

The implementation remained lightweight and native.

No copyrighted official artwork was embedded into the release.

The interface used original/procedural Win32/GDI presentation inspired by the
visual concept.

---

# Automated setup validation

The Asset Setup test suite expanded alongside the wizard.

Testing covered cases including:

- PAL detection
- NTSC-U detection
- Disc 1 / Disc 2 identification
- swapped disc order
- mixed-region media
- missing assets
- invalid paths
- truncated ISOs
- modified XEX builds
- foreign Title IDs
- valid DLC
- duplicate DLC
- corrupted DLC
- foreign DLC
- truncated DLC
- cancellation cleanup
- settings persistence
- explicit path priority
- DLC idempotency
- subsequent-launch behavior

Automated tests were never intended to replace real gameplay testing.

Their purpose was to protect the setup/runtime integration from regressions while
development continued.

---

# October 1, 2026 — Portable preview milestone

A new portable build was created after the portability, DLC, localization and UI
work.

The executable for that historical preview had SHA-256:

    BC1CDDA5F93727DA7AC8F4F7E7DA3A2F7829E55B947BE9677F4F113E57664E20

The preview archive was:

    InfiniteUndiscoveryRecomp-Portable-Test.zip

The archive intentionally contained no pre-created game data.

It excluded:

- NTSC-U assets
- PAL assets
- game files
- saves
- shaders
- cache
- DLC
- ISOs

The goal was to reproduce the experience of a completely new user.

---

# First real portable test

The portable ZIP was extracted outside the development tree.

It was launched from a completely unrelated directory.

The build worked.

This demonstrated that the recomp no longer depended on the original development
environment.

The target workflow had become real:

    Download ZIP
    → Extract anywhere
    → Run
    → Select your own media
    → Play

This validated the portable-only distribution model.

---

# No installer

The release philosophy was made explicit:

> **There will be no traditional installer.**

No MSI.

No Program Files installation.

No registry dependency.

No intentional spreading of files around the user's profile.

The official distribution format would be a portable ZIP.

---

# Expanding beyond PAL / NTSC-U

As media validation expanded, the original two-region model was no longer enough.

The project moved toward explicit content profiles.

The final RC1 profile model became:

    USA
    USA-UNDUB
    EUROPE
    JAPAN
    ASIA

Each profile has independent:

- assets
- saves
- shaders
- cache
- logs
- configuration

Profiles may coexist in the same portable installation without sharing runtime
state.

For compatibility with older portable builds:

    NTSC-U → USA
    PAL    → EUROPE

Legacy folders are migrated automatically.

The old two-folder PAL / NTSC-U design therefore remains historically important,
but it is no longer the current runtime layout.

---

# USA-UNDUB support

Community interest and internal testing led to explicit support for an Undub
configuration using:

- USA text/data
- Japanese voices

A dedicated profile was created:

    USA-UNDUB

This allowed Undub media to remain isolated rather than weakening validation for
every game variant.

The setup system gained edition-aware scanning and profile-specific detection.

USA-UNDUB also received a first-launch subtitle recommendation explaining where to
enable voice subtitles in the game options.

The warning can be dismissed permanently.

---

# Japan and Asia profiles

Media research continued beyond the original PAL and NTSC-U releases.

Explicit profiles were added for:

    JAPAN
    ASIA

Testing confirmed the Asia release containers matched the expected English content
layout.

The user-facing profile name was standardized as:

    Asia (English)

By RC1, all five profiles had been exercised through the setup/profile system.

---

# Content Profile Manager

The original single-flow setup wizard evolved into a multi-profile management
system.

The Content Profile Manager allows users to:

- view supported profiles
- install a profile
- auto-detect compatible media
- select an active profile
- maintain several profiles in one portable installation

Media scanning became edition-aware.

When auto-detection is launched from a specific profile row, the scan filters for
that edition.

When several compatible editions are found, the user can choose which one to
configure.

The wizard also began displaying persistent profile context such as:

    Setting up: <Profile>

This made it clear which edition was currently being installed.

---

# Profile Manager lifecycle work

Switching back to the Profile Manager exposed a lifecycle problem.

Earlier implementations could create overlapping processes or leave a guest
instance frozen while another process was launched.

The F10 flow was redesigned.

F10 now asks:

    Return to Profile Manager?
    Current game session will be closed.

If confirmed, the game performs a clean shutdown first:

- window close request
- title termination
- log flushing

Only during normal process exit is the Profile Manager launched.

This eliminated overlapping guest/runtime instances.

---

# Game Menu and recovery tools

The original development-oriented "Community Debug" overlay evolved into:

    Infinite Undiscovery — Game Menu

The menu was reorganized into clearer sections.

## Game

- Return to Profile Manager — F10
- Quit Game — F12

## Recovery Tools

- Save Anywhere — F6
- Safe Step Forward — F8
- Undo Debug Move — F9

## Developer / Debug

- Diagnostic Trace Recorder

## Menu

- Close — F5

F12 received its own confirmation dialog to ensure the title shuts down cleanly.

The recovery tools remained deliberate user-triggered actions rather than automatic
runtime modifications.

---

# Save Anywhere

Save Anywhere was introduced as a recovery/debug action.

Its purpose was not to redesign the game's original save system.

Instead, it provided a controlled mechanism for preserving progress while testing
problem areas and long gameplay sequences.

Development included separate investigation of:

- target acquisition
- runtime entry points
- safety constraints

The feature remained clearly identified as a recovery/debug tool.

---

# Safe Step Forward and Undo Debug Move

Additional recovery tools were introduced for difficult runtime situations.

Safe Step Forward allows controlled positional recovery.

Undo Debug Move allows reverting the most recent supported recovery movement.

These tools were intended to help investigate or escape runtime problems without
pretending that the underlying bug had been fixed.

---

# PSO diagnostics and prewarm work

Shader and pipeline behavior remained an important part of runtime stability.

The project added PSO-related diagnostics and prewarm/telemetry support.

This work helped characterize pipeline creation and graphics behavior while keeping
the normal gameplay path usable.

PSO state later became visible in the Session / System diagnostic interface.

---

# Disc 2 runtime work

Preparing Disc 2 assets had been solved much earlier.

The next problem was determining what actually happens when the running game reaches
the original disc transition.

Explicit Disc 2 runtime work introduced:

- Disc 2 IO tracing
- package tracing
- disc-swap state tracing
- current-disc tracking
- diagnostics around mounted devices
- runtime preparation for a future validated transition

The project deliberately continued to distinguish:

> Disc 2 is installed

from:

> The real Disc 1 → Disc 2 transition has been proven during gameplay

That distinction remains important in RC1.

---

# Vesplume Tower / Orb of Patience

A historical Infinite Undiscovery progression problem became one of the project's
most important real-game validation targets:

**Vesplume Tower / Orb of Patience softlock.**

The same or similar behavior had also been observed under emulation.

A community save was later provided specifically reproducing the problem.

The issue became useful not only as a bug report but also as a diagnostic checkpoint
for:

- game state
- story flags
- Disc 1 progression
- transition behavior
- runtime scheduling
- recovery tooling

The project does not claim that this bug is fixed in RC1.

Instead, it remains an active regression and investigation case.

---

# Diagnostic Game Menu

Before RC1, the Game Menu received a larger diagnostics pass.

A centralized version source was introduced:

    src/version.h

with:

    kProjectVersion = "v1.0.0-rc1"

The Game Menu header gained live information including:

- active profile
- current disc
- project version
- loaded DLC count

A dedicated:

    Session / System Info

window was added.

---

# Session / System Info

The diagnostics interface reports structured information in several categories.

## Game

- active profile
- profile display name
- current disc
- Title ID
- region / edition code
- UI language
- DLC count

## Build

- project version
- build type
- build timestamp
- compiler identity

## Runtime

- ReXGlue SDK version
- active disc mount
- Disc 1 availability
- Disc 2 availability

## Graphics

- D3D12 backend
- GPU adapter
- window resolution

## System

- Windows version
- logical CPU count
- total RAM
- available RAM

## Paths

- executable root
- profile root
- assets
- disc1
- disc2
- DLC
- saves
- shaders
- cache
- logs

## Diagnostics

- trace recorder status
- Safe Step status
- Undo availability
- Save Anywhere status
- PSO prewarm status
- PSO telemetry status

The paths shown in diagnostic reports are sanitized to avoid exposing Windows user
names unnecessarily.

---

# Diagnostic report export

The diagnostics window gained two support-oriented features:

    Copy All to Clipboard
    Save Diagnostic Report

Clipboard output produces a Markdown-friendly issue report.

Saved diagnostics are written into the active profile logs directory with a
timestamped name similar to:

    IU_Diagnostic_YYYY-MM-DD_HH-MM-SS.txt

This was designed to make future bug reports easier to reproduce and analyze.

---

# Runtime bilingual interface

The English / Spanish localization system was expanded beyond the setup wizard.

The Game Menu, status indicators, buttons and tooltips became bilingual.

An inline:

    English | Español

switch can change the runtime UI immediately without restarting the application.

The setting is persisted into the portable configuration.

Several localization consistency problems were also corrected, including the
English/Spanish ACTIVE / ACTIVO status badge.

---

# IU Save Bridge

As portable saves became an important part of the project, a companion utility was
developed:

    IU Save Bridge

Repository:

    https://github.com/doc-haz/iu-save-bridge

IU Save Bridge became the official portable save companion for Infinite Undiscovery
Recomp.

Its role is separate from the recomp itself.

The recomp runs the game.

Save Bridge provides controlled save inspection/editing.

Features developed for the companion included:

- portable save discovery
- Fol editing
- character stat editing
- inventory editing
- support for 1,023 inventory entries
- automatic backups
- manual backups
- safe restore
- dual CRC32 recalculation
- English / Spanish interface
- portable operation

The companion was later linked from the main recomp README and RC1 release
documentation.

As the recomp moved from historical NTSC-U / PAL directories to the final content
profiles, Save Bridge also became a follow-up target for profile-layout updates.

---

# Repository growth and cleanup

By the time the recomp was working reliably, the development directory had grown
very large.

It contained:

- generated code
- build trees
- extracted game media
- repeated Disc 1 / Disc 2 assets
- DLC test material
- diagnostics
- local tools
- portable previews
- temporary experiments
- evidence directories
- multiple runtime copies

A full inventory showed that the working root had grown to roughly 97 GB, mostly
because large game data had been duplicated across several experimental trees.

Eight region/build trees alone accounted for most of that usage.

A clean repository strategy was adopted.

Before reorganizing anything, a full backup was created:

    repo-clean-backup-2026-10-06

The backup preserved the original working tree.

The active `repo-clean` tree was then reduced to a lightweight source-oriented
working repository containing only the files needed for development and release.

The cleaned repository was approximately 189 MB rather than tens of gigabytes.

No proprietary game content was added to Git.

The large backup remained intact until after release validation.

---

# ReXGlue runtime investigation

Release preparation exposed an unexpected problem.

The project originally used precompiled ReXGlue v0.10.0 runtime DLLs.

VirusTotal results for the original precompiled runtime, particularly
`rexruntime.dll`, showed a high number of antivirus detections.

A detection count alone does not prove that a binary is malicious, but distributing
a public recomp with heavily flagged runtime DLLs was considered unacceptable.

The public release was therefore paused.

The solution was not to whitelist the files.

The solution was to rebuild ReXGlue from auditable source.

---

# Auditable ReXGlue v0.10.0 source

The official ReXGlue repository was checked out at:

    tag:    v0.10.0
    commit: f5337cdc947ff6d4c4196737e2c807a48f2a1fc2

The original source snapshot was compared against the Git tag.

Versioned files matched, but the snapshot did not contain all submodule state needed
for a fully auditable rebuild.

A proper Git checkout with pinned submodules was therefore prepared.

Submodule retrieval initially encountered HTTP/2 problems.

Using HTTP/1.1 and serial submodule initialization resolved the fetch issues.

---

# Windows symlink problem

Another source-build problem appeared in `libmspack`.

Several files expected as Unix-style symlinks had been checked out on Windows as
plain text files containing target paths because symlink support was unavailable.

Fifteen affected files were replaced locally with byte-identical copies of their
intended targets.

This allowed the official source tree to compile without changing the effective
source content.

---

# Rebuilding ReXGlue

The auditable ReXGlue build used:

- Clang / LLVM 20.1.8
- CMake 3.31.8
- Ninja 1.12.1
- MSVC STL / toolset 14.44.35207
- Windows SDK 10.0.26100.0

The resulting clean runtime DLLs were:

## rexruntime.dll

    SHA-256
    25C0F2D1DBB7FE3147FC59E22C9A3E4C764DC2E0B0C6877FC86F0DA499E0F67F

## rexgpu-xenos.dll

    SHA-256
    98CEF22E2AC1667F3A42910DD7474C0409BBEF7DB7599B3A48DF1EDBB0739A2D

Both newly built DLLs subsequently produced clean VirusTotal results and clean
Microsoft Defender scans.

---

# ABI compatibility audit

Replacing runtime DLLs immediately before a public release required more than simply
confirming that the filenames matched.

A detailed ABI audit was performed.

The Infinite Undiscovery executable imported:

    304 symbols

from `rexruntime.dll`.

The new runtime provided:

    304 / 304

required symbols.

The Xenos GPU plugin required:

    103 symbols

from the runtime.

The new runtime provided:

    103 / 103

required symbols.

Additional project tools and tests were also checked.

The original and rebuilt runtimes had some differences in their total exported
symbol sets, largely caused by STL/template instantiations exported through the
runtime's broad Windows export configuration.

None of the symbols actually consumed by the known project binaries were missing.

The Xenos GPU plugin ABI was also examined directly.

The plugin exported:

    AmdPowerXpressRequestHighPerformance
    NvOptimusEnablement
    rex_gpu_abi_version
    rex_gpu_create

`rex_gpu_abi_version()` returned ABI version 1.

PE characteristics, import sets and runtime linkage model were also compared.

The rebuilt DLLs were judged drop-in compatible for the recomp.

---

# Controlled runtime swap test

A controlled copy of a known-working Infinite Undiscovery build was created.

Only:

    rexruntime.dll
    rexgpu-xenos.dll

were replaced with the locally rebuilt auditable versions.

The game successfully:

- started
- loaded the Xenos GPU plugin
- created a D3D12 device
- translated more than one thousand shaders
- created graphics pipelines
- loaded an existing save
- accepted movement and camera input
- rendered gameplay
- shut down cleanly

No observable ABI, plugin or import regression was found.

---

# Release pipeline hardening

Once the rebuilt ReXGlue runtime was validated, the release pipeline itself was
hardened so that the old precompiled DLLs could not accidentally return.

The project was changed to consume an auditable installed ReXGlue v0.10.0 tree.

A post-build verification step was introduced:

    cmake/verify_rexglue_dlls.cmake

The build verifies that the runtime DLL hashes match the known clean builds.

If the old precompiled runtime is encountered, the build aborts.

The release pipeline also gained:

    release/make-release.ps1

The script stages only approved release files and performs antivirus checks before a
publicable archive is produced.

---

# Defender release gate

Microsoft Defender became a formal release gate.

The release workflow scans:

- InfiniteUndiscoveryRecomp.exe
- rexruntime.dll
- rexgpu-xenos.dll
- release staging directory
- final application ZIP
- compliance ZIP

A non-zero Defender result aborts the public-release path.

During RC preparation, the freshly created ZIP occasionally produced a transient:

    0x80508023

engine error.

The gate correctly treated this as a failure and stopped.

Repeated scans subsequently completed successfully with no threats detected.

Because the same transient behavior appeared more than once immediately after ZIP
creation, retry handling was identified as a possible future pipeline improvement.

---

# Third-party license audit

The deeper release audit also reviewed third-party licensing.

The runtime was found to statically incorporate LGPL components including:

- FFmpeg `libavcodec`
- FFmpeg `libavutil`
- libmspack

The relevant FFmpeg fork was identified at commit:

    0604b464c7cb4ebc94940cf1f324a3b26b87717c

with LGPL configuration and GPL/nonfree components disabled.

The libmspack source used by the runtime was identified at commit:

    305907723a4e7ab2018e58040059ffb5e77db837

Rather than ignoring the issue because similar recomp projects commonly ship
runtime binaries, the project chose to prepare explicit compliance material.

---

# LGPL relink compliance package

A separate compliance package was constructed containing:

- corresponding FFmpeg source
- corresponding libmspack source
- license texts
- non-LGPL object files needed for relinking
- non-LGPL static libraries needed for relinking
- `exports.def`
- exact object ordering
- exact library ordering
- relink scripts
- version/toolchain documentation

The package was tested in practice.

Three relink cases were validated:

1. baseline relink
2. modified/rebuilt libmspack
3. modified/rebuilt libavutil

The resulting `rexruntime.dll` files preserved the required export interface.

A baseline relinked runtime was also placed beside the real recomp executable and
successfully ran gameplay.

The purpose was not to reproduce an identical DLL byte-for-byte.

The purpose was to demonstrate that users have the material required to modify the
LGPL library and relink a functional runtime.

---

# disruptorplus license resolution

One remaining third-party dependency required investigation:

    disruptorplus

The vendored ReXGlue copy did not include its own LICENSE file.

Source comparison showed that it corresponded to the Xenia fork of
`disruptorplus`, derived from the Lewis Baker upstream project.

The vendored headers were almost entirely byte-identical to the Xenia fork, with a
single local code difference identified in one header.

The upstream and Xenia fork both use the MIT License:

    Copyright (c) 2013 Lewis Baker

The exact license text was added to:

    LICENSES/disruptorplus-LICENSE.txt

and the third-party notices were updated.

This resolved the final known third-party licensing blocker for RC1.

---

# Third-party notices

The public release was prepared with:

    LICENSES/
    THIRD_PARTY_NOTICES.txt

The notices document relevant third-party components and their licenses.

The main application ZIP contains the normal third-party notices.

The larger LGPL corresponding-source/relink material is distributed as a separate
release asset so that the normal game ZIP remains compact.

---

# Release Candidate consolidation

Before sealing RC1, a consolidated QA/fix pass addressed several lifecycle and UI
issues.

## F10 profile-manager lifecycle

Process overlap and frozen guest instances were eliminated.

F10 now performs clean shutdown before returning to the Profile Manager.

## F12 Quit Game

A dedicated quit command with confirmation was added.

## USA-UNDUB STFS handling

STFS metadata handling was aligned with ReXGlue behavior when the English Lang ID 0
string slot is empty.

## Setup UX

The wizard gained:

- profile-aware auto-detection
- multi-edition selection
- persistent setup-profile context
- corrected Back-button behavior
- wider profile cards
- post-install confirmation
- complete English / Spanish UI review

---

# RC1 final polish

The final polish pass included:

- centralized version reporting
- current-disc reporting
- DLC count
- Session / System Info
- sanitized diagnostic report export
- runtime language switching
- USA-UNDUB subtitle recommendation
- Asia profile naming cleanup
- PSO status
- recovery tool status
- profile information
- system information
- graphics information

By this point the five-profile model had been validated at the setup/runtime level:

    USA
    USA-UNDUB
    EUROPE
    JAPAN
    ASIA

---

# Final repository reconciliation

While RC1 was being prepared locally, two commits had appeared on the remote
`main` branch:

    Create .recomp.json
    Update README.md

The local RC1 commit and remote `main` therefore diverged.

Because the RC1 commit had not yet been published, the clean solution was to rebase
it onto the updated remote branch.

A local safety branch was created first.

The rebase completed successfully without conflicts.

A final documentation pass then corrected two legacy Save Bridge paths from:

    NTSC-U\saves\
    PAL\saves\

to:

    USA\saves\
    EUROPE\saves\

The RC1 commit was amended rather than creating a meaningless extra documentation
commit.

---

# Final RC1 commit

The final Release Candidate commit became:

    ce6586c0a237541caa438122ecdd99c62982b288

Commit title:

    feat: v1.0.0-rc1 — multi-region profiles, in-game menu & recovery tools, hardened ReXGlue release

The working tree was clean before publication.

---

# Final RC1 validation

The final build used the audited ReXGlue v0.10.0 runtime.

The application was tested from the exact staged release binaries.

Validation included:

- application startup
- ReXGlue v0.10.0 runtime
- correct clean runtime DLLs loaded
- D3D12 device creation
- shader translation
- USA profile
- DLC validation
- save loading
- real gameplay rendering
- clean shutdown

Microsoft Defender reported no threats in the final public artifacts.

---

# Final RC1 release hashes

The public application archive:

    InfiniteUndiscoveryRecomp-v1.0.0-rc1.zip

SHA-256:

    311A243E67FD992597A0AC72E5632DEF4FC9311F538B9F8C974AE07B315945B2

The separate LGPL compliance archive:

    InfiniteUndiscoveryRecomp-v1.0.0-rc1-LGPL-Compliance.zip

SHA-256:

    082BE0B4D0F938494A1C5852057EDA5DF905A8237C3DD451D266D0C4436B56B9

Runtime DLL hashes:

    rexruntime.dll
    25C0F2D1DBB7FE3147FC59E22C9A3E4C764DC2E0B0C6877FC86F0DA499E0F67F

    rexgpu-xenos.dll
    98CEF22E2AC1667F3A42910DD7474C0409BBEF7DB7599B3A48DF1EDBB0739A2D

---

# October 6, 2026 — v1.0.0-rc1 public release

The final RC1 commit was pushed to `main`.

An annotated tag was created:

    v1.0.0-rc1

The tag was pushed to GitHub.

The first public Release Candidate was then published as a GitHub **Pre-release**:

    Infinite Undiscovery Recomp v1.0.0-rc1

The release contains:

1. the portable Windows build
2. the separate LGPL compliance package

No copyrighted Infinite Undiscovery game files, DLC, ISOs or saves are included.

Users must provide their own legally obtained game media.

This was the first point at which the project moved from internal development and
preview builds to a publicly distributed native Windows Release Candidate.

---

# Public issue tracking begins

With the RC publicly available, community testing became the next source of
validation.

The first public issues included:

## Vesplume Tower softlock

A save was provided reproducing the Orb of Patience/Vesplume Tower problem.

The issue remains an active regression/investigation case.

## USA-UNDUB request

A user requested support for an Undub version using Japanese voices with English
text.

By the time RC1 was published, a dedicated `USA-UNDUB` profile already existed.

Community testing is still useful because independently produced Undub variants may
not all have identical file layouts or hashes.

## Ultrawide support

Users requested resolutions including:

    3440x1440
    5120x2160

Ultrawide support is considered a future feature rather than an RC1 stability
requirement.

Proper implementation should evaluate:

- aspect ratio
- camera/FOV behavior
- HUD placement
- 2D elements

rather than simply forcing a larger resolution.

---

# Current design principles

Several principles now define the project.

## Portable first

The portable folder is the installation.

No traditional installer is required.

---

## Bring your own game

The project does not distribute proprietary Infinite Undiscovery content.

Users provide their own legally obtained media.

---

## Native setup

Normal users should not need development extraction tools.

The executable handles the normal media import flow.

---

## Profile isolation

Different editions remain independent.

The final profile model is:

    USA
    USA-UNDUB
    EUROPE
    JAPAN
    ASIA

Each profile has its own:

- assets
- saves
- shaders
- cache
- logs
- configuration

---

## Historical compatibility

Older portable folders are migrated:

    NTSC-U → USA
    PAL    → EUROPE

Historical documentation retains the old names when describing the stage of
development in which they were actually used.

---

## Local saves

The recomp uses its own portable saves.

It does not silently adopt unrelated emulator or historical development saves.

---

## Recovery tools are recovery tools

Save Anywhere, Safe Step Forward and Undo Debug Move exist to support testing and
recovery.

Their existence should not be used to claim that an underlying game/runtime bug is
fixed.

---

## Do not claim what has not been demonstrated

The project deliberately separates:

    implemented

from:

    validated in real gameplay

The clearest example remains Disc 1 → Disc 2.

Disc 2 preparation and runtime support have progressed substantially.

That does not mean a complete real-game disc transition has already been proven.

---

## Auditable release dependencies

Public runtime DLLs should come from an identifiable source revision.

RC1 uses the audited ReXGlue v0.10.0 source revision rather than the previously used
precompiled SDK binaries.

---

## Clean public artifacts

A release should contain only what the user needs.

Development data, proprietary game assets, local saves, generated game files and
temporary diagnostics do not belong in the public archive.

---

# What v1.0.0-rc1 represents

By the first public Release Candidate, Infinite Undiscovery Recomp had evolved from
a low-level recompilation experiment into a portable Windows application with:

- native recompiled execution
- integrated media setup
- native Xbox 360 media inspection
- five content profiles
- automatic media identification
- Disc 1 / Disc 2 separation
- multi-disc runtime groundwork
- DLC validation
- STFS DLC installation
- idempotent DLC reuse
- fully portable runtime state
- isolated saves
- isolated shaders/cache/logs/configuration
- English / Spanish setup
- English / Spanish runtime UI
- Content Profile Manager
- Game Menu
- Save Anywhere
- Safe Step Forward
- Undo Debug Move
- F10 Profile Manager return
- F12 clean quit
- Session / System diagnostics
- sanitized issue-report generation
- PSO diagnostics
- Disc 2 tracing
- USA-UNDUB support
- Japan support
- Asia support
- official IU Save Bridge companion
- auditable ReXGlue runtime builds
- ABI-validated replacement runtime DLLs
- antivirus-gated release pipeline
- third-party license notices
- LGPL corresponding-source/relink package
- reproducible runtime relink validation
- a clean public Git repository
- a public GitHub Release Candidate

---

# Known RC1 validation items

The following remain active validation areas:

- full-game completion
- Vesplume Tower / Orb of Patience softlock
- real Disc 1 → Disc 2 transition
- complete Disc 2 gameplay
- complete validation of DLC effects during gameplay
- runtime problems that may only appear much later in the game
- compatibility with independently produced USA-UNDUB variants
- ultrawide support
- further performance and shader/pipeline tuning

These items are not hidden behind a "complete" label.

RC1 is intentionally a Release Candidate.

Its purpose is to turn internal testing into broader real-world validation.

---

# From experiment to public Release Candidate

The project began with a much smaller question:

> Can Infinite Undiscovery run as a native recompilation on PC?

The answer became yes.

The next question was:

> Can it be packaged so another person can use it without rebuilding the entire
> development environment?

That answer also became yes.

Then came a third question:

> Can it be released in a way that is portable, auditable, legally documented,
> antivirus-clean and honest about what has and has not been tested?

The work leading to `v1.0.0-rc1` was the answer to that question.

The intended user experience remains deliberately simple:

    Download
    Extract
    Run
    Select your own game media
    Play

Everything behind that workflow — recompilation, media detection, extraction,
profiles, DLC, runtime DLLs, diagnostics, compliance, release tooling and validation
— exists so that the user does not have to recreate the development environment.

`v1.0.0-rc1` is not the end of the project.

It is the point where Infinite Undiscovery Recomp moved from an internal experiment
to a public native-PC project ready for broader testing.

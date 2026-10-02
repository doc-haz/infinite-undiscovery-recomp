# Infinite Undiscovery Recomp — Project History

This file is a development timeline for **Infinite Undiscovery Recomp**, from the first experiments to the portable V1.0 executable.

It is not meant to be a technical changelog for every commit. It is the story of the important milestones, problems, wrong turns and breakthroughs that got the project here.

---

## September 2026 — The project begins

Infinite Undiscovery Recomp started as an experiment:

> Can Infinite Undiscovery be recompiled into a native Windows executable?

The project was built around the **ReXGlue / XenonRecomp** ecosystem.

The first goal was intentionally small:

1. Understand the Xbox 360 executable.
2. Generate native code.
3. Get the project to compile.
4. Get the game to start.
5. Deal with whatever broke next.

From the beginning, one rule was fixed:

**The repository would not contain proprietary Infinite Undiscovery game assets.**

Users would always need to provide their own legally obtained copy of the game.

---

## Initial repository

The first repository commit was:

    d017e263
    Initial commit

At this point the repository contained the basic recompilation project structure, build configuration, manifest and early runtime support.

The project was still very much a development environment rather than something a normal user could download and play.

---

## PAL investigation

Development initially focused on the PAL release.

One reason was the expectation that the PAL version might provide the desired language support.

The Xbox 360 media was inspected and the important game files were identified:

    default.xex
    ud1.bin
    ud2.bin

The game is a two-disc title, and each disc contains its own copies of:

    default.xex
    ud1.bin
    ud2.bin

This later became important because Disc 1 and Disc 2 assets cannot simply overwrite each other.

---

## First native builds

The project eventually reached the point where the generated C++ could be compiled into a native Windows executable.

The early builds were still developer-oriented.

Game assets had to be prepared manually and paths supplied directly to the runtime.

There was no setup wizard and no end-user installation flow.

Still, this was an important milestone:

**Infinite Undiscovery was no longer just being analyzed. It was executing as a native PC recompilation.**

---

## Runtime bring-up

Getting the code to compile was only the beginning.

The next stage was largely runtime debugging.

Work during this phase involved areas such as:

- guest memory initialization
- Xbox 360 runtime behavior
- Xenos GPU command processing
- synchronization
- frame progression
- fibers
- diagnostics
- shader handling
- game-specific hooks

The game gradually progressed further and further instead of failing immediately.

This phase produced many temporary diagnostics, traces and experiments that were useful during development but were never intended to become part of the public repository.

---

## GPU and execution progress

One of the most difficult parts of the early work involved the Xenos graphics/runtime path.

The game eventually reached hundreds of frames of execution reliably.

At one stage, execution repeatedly reached roughly frame 309 before encountering an unregistered guest function around:

    0x821F2360

That address became one of several low-level runtime problems investigated during bring-up.

Problems like this marked a change in the project.

The question was no longer:

> Can the executable run?

It was becoming:

> Which specific remaining game/runtime system is stopping it now?

---

## Graad Prison

One of the first major practical validations was reaching **Graad Prison** in the native recompilation.

The historical early build was manually launched by double-clicking the EXE and verified running actual game content.

This was important because it showed that the project had progressed well beyond a simple boot or menu proof of concept.

---

# PoC 0.1

The first historical proof-of-concept release was preserved as:

    86eaeca
    Add Infinite Undiscovery Recomp Preliminary PoC 0.1

PoC 0.1 included working pieces such as:

- recompilation
- startup
- frame pacing
- diagnostics
- existing hooks
- fibers
- game asset path configuration

The PoC became a historical baseline.

A deliberate project rule was adopted:

> **PoC 0.1 should remain historical and must not be rewritten retroactively.**

Later development would build forward from it rather than pretending the early version had capabilities that were added much later.

---

## PAL vs NTSC-U

The PAL version was originally expected to provide the desired Spanish language support.

The tested PAL release did not provide the expected result, so attention shifted toward the North American **NTSC-U** release.

That investigation produced one of the more useful discoveries of the project:

### PAL and NTSC-U use the same executable payload

After decrypting and comparing the executable payloads, the relevant PE content was found to be identical.

The versions shared:

- the same executable code
- the same `.text`
- the same `.rdata`
- the same `.data`
- the same relevant function addresses
- the same hook locations

The differences were primarily in Xbox 360 media/header information such as region, certification and disc metadata.

This meant the recompilation work did **not** need to be duplicated for PAL and NTSC-U.

One native recompilation could support both variants.

That discovery later made automatic region detection possible.

---

## NTSC-U media validation

The North American discs were inspected independently.

Title ID:

    535107DB

The game was confirmed as a two-disc NTSC-U release.

Disc 1 and Disc 2 both contain:

    default.xex
    ud1.bin
    ud2.bin

The large BIN files were hashed and verified independently.

One important rule came out of this work:

> **Never merge or overwrite Disc 1 and Disc 2 BIN files.**

Both discs need to remain independently available to the runtime.

---

## Development extraction tools

During early development, Xbox 360 media was prepared with development utilities including:

    xdvdfs.py
    xex.py

These tools were extremely useful for research and development.

They could inspect Xbox 360 disc images, extract files and decrypt/inspect the XEX.

However, requiring a normal user to run Python scripts manually was never considered an acceptable final experience.

This eventually became one of the motivations for the integrated setup system.

---

## Multi-disc work

Infinite Undiscovery uses two game discs.

The project eventually became capable of preparing and preserving both Disc 1 and Disc 2 asset sets.

However, another important distinction was made:

> Having Disc 2 installed is not the same thing as proving live disc switching during an actual playthrough.

For that reason, runtime disc swapping was deliberately kept as a separate validation item.

The project would not claim that disc switching worked until it had actually been tested during gameplay.

---

# DLC investigation

The game's downloadable content was investigated separately from the disc assets.

Two Marketplace Content packages were identified:

    A Voucher
    License mask: 0x1

and:

    B Voucher
    License mask: 0x10

Both belong to Infinite Undiscovery:

    Title ID: 535107DB
    Content type: 00000002

The packages had to remain distinct.

They could not be treated as interchangeable simply because some internal resources appeared similar.

---

## ReXGlue STFS support

An important discovery was that the ReXGlue runtime already had proper Xbox 360 STFS content support.

The project could therefore use:

    ContentManager::InstallContent

rather than inventing its own fake DLC layout.

This allowed the recomp to validate and install DLC through the runtime's content system.

That became the basis of the final DLC setup implementation.

---

# October 1, 2026 — The project changes direction

By October 1, the game itself was running well enough that the next major weakness had become obvious:

**The setup experience was terrible for a normal user.**

Development still involved manual paths, extraction utilities and knowledge of internal Xbox 360 files.

The project therefore adopted a new goal inspired in large part by the experience of projects such as **Lost Odyssey Recomp**:

    Download
    → Run EXE
    → Select your own game media
    → Let the EXE prepare everything
    → Play

No manual Python.

No BAT setup.

No PowerShell setup.

No manually running `xdvdfs.py`.

No manually running `xex.py`.

No requirement for the end user to understand:

- XEX files
- XDVDFS
- Title IDs
- `ud1.bin`
- `ud2.bin`
- Xbox 360 DLC container structure

This became the **Asset Setup Wizard** project.

---

# Asset Setup Wizard

The wizard was integrated directly into the recomp executable.

The first complete implementation could:

- detect Infinite Undiscovery media
- identify PAL / NTSC-U
- identify Disc 1 / Disc 2
- extract game assets natively
- keep both game discs separated
- validate optional DLC
- install DLC through the SDK
- persist configuration
- skip setup on future runs
- cancel cleanly without starting the game

Both PAL and NTSC-U extraction paths were tested.

This was the point where Infinite Undiscovery Recomp began to feel less like a developer experiment and more like an actual standalone PC application.

---

## First integrated preview

The first wizard preview compiled successfully and could launch the game after preparing the assets.

If valid assets were already installed, the EXE correctly skipped the wizard and went directly into game startup.

The build contained only:

- the EXE
- required runtime DLLs

It did **not** contain:

- game assets
- ISOs
- proprietary DLC

The historical PoC 0.1, diagnostics, hooks and fiber work were kept intact.

---

# The portability problem

Testing the first integrated preview exposed a problem.

The setup system created installation directories with generated names similar to:

    InfiniteUndiscovery-USA-127528054305200-0

Technically it worked, but it was not a clean user-facing layout.

A more serious problem was discovered immediately afterward.

The recomp found **three old save files** that had been created previously by the original development runtime.

Those saves lived outside the new portable setup.

That behavior was considered unacceptable.

The new application was supposed to be self-contained.

It should not silently discover saves or other runtime state from an older project installation.

---

# Portable-only becomes a project rule

A permanent design decision was made:

> **Infinite Undiscovery Recomp will be fully portable.**

The directory containing the EXE became the root of the installation.

The program should not depend on:

- Documents
- My Documents
- AppData
- LocalAppData
- Saved Games
- old development paths
- emulator save directories
- previous recomp directories

The application should be movable simply by copying its folder.

No installer is required.

No installer is planned.

The portable folder **is the installation**.

---

## Region-separated portable data

Because both PAL and NTSC-U are supported, the portable structure was designed to allow both to coexist.

The wizard automatically detects the region from the user's media.

The user does not manually select PAL or NTSC-U.

The resulting structure is based around stable region folders:

    NTSC-U\
    PAL\

Each region has its own runtime state.

Example:

    InfiniteUndiscoveryRecomp\
    ├─ EXE
    ├─ runtime DLLs
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

This design allows both supported regions to exist independently in the same portable installation.

---

## Save isolation

The portable redesign also changed save behavior.

New saves belong only to the portable region folder.

For example:

    NTSC-U\saves\

or:

    PAL\saves\

Old saves from Documents or previous development runtime locations are deliberately ignored.

There is no automatic migration.

There is no fallback discovery.

This prevents a fresh portable build from unexpectedly loading data created by an older experimental runtime.

---

# DLC second-launch bug

The first DLC-enabled portable preview revealed another issue.

On the first launch, the DLC installed correctly.

On the second launch, the runtime attempted to process the already-installed DLC again and reported a conflict involving the existing DLC header/license.

The packages were not actually different.

The problem was eventually traced to comparison behavior involving padding/non-deterministic data in the installed content representation.

The DLC handling was corrected to become idempotent.

The intended behavior became:

    First launch:
    validate → install → launch

    Later launch:
    detect same valid DLC → reuse → launch

A genuinely different, corrupted or conflicting DLC package should still be rejected.

The goal was not to weaken validation.

It was to stop reporting a conflict when the already-installed DLC was actually the same package.

---

# English and Spanish

The original integrated wizard was written primarily in Spanish.

Rather than simply replacing it with English, the project adopted bilingual support.

The wizard now supports:

    English | Español

English is the default.

The language can be changed from the wizard and the preference is persisted.

User-facing strings were moved into a centralized localization system rather than leaving scattered hard-coded dialog text.

---

# Wizard visual redesign

The first wizard was functional but visually very simple.

It looked like a plain white native Windows utility.

That was useful during development, but it did not match the identity of the project.

A visual concept was created for a more polished setup experience.

The design direction included:

- dark midnight-blue presentation
- fantasy/JRPG atmosphere
- Infinite Undiscovery Recomp branding
- Disc 1 → Disc 2 → DLC progress display
- clear validation status
- English / Español selector
- dark content panels
- decorative fantasy elements
- cleaner spacing and hierarchy

The final implementation stayed lightweight and native.

Rather than shipping copyrighted official game art, the UI was implemented with procedural/original Win32/GDI elements inspired by the visual concept.

The redesigned wizard included elements such as:

- celestial/fantasy decorative motifs
- dark midnight styling
- starfield-like decoration
- architectural/fantasy cards
- disc indicators
- validation badges
- styled navigation controls

The mockup served as a direction, not as a giant image pasted behind the controls.

---

# Automated setup validation

The Asset Setup test suite grew alongside the wizard.

Tests covered cases including:

- PAL media detection
- NTSC-U media detection
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
- subsequent launch behavior

The purpose of the tests was not to replace manual gameplay testing.

They were intended to protect the setup/runtime integration from regressions while the project moved toward a public release.

---

# October 1, 2026 — Portable preview milestone

A new portable build was compiled after the portability, DLC, localization and UI work.

The executable produced for that revision had SHA-256:

    BC1CDDA5F93727DA7AC8F4F7E7DA3A2F7829E55B947BE9677F4F113E57664E20

The associated runtime libraries remained separate from the game assets.

The project still distributed no proprietary game content.

The build was packaged into a clean test archive:

    InfiniteUndiscoveryRecomp-Portable-Test.zip

The archive contained only the application/runtime files necessary to start the recomp.

It deliberately did not contain pre-created:

- NTSC-U data
- PAL data
- game assets
- saves
- shaders
- cache
- DLC
- ISOs

The point was to test the same experience a new user would have after downloading a release.

---

# The first real portable test

The portable ZIP was extracted outside the development tree.

It was launched from a completely different directory, like a normal release.

The build worked.

This was a major milestone.

It demonstrated that the recomp no longer depended on the original development directory to function.

The project had reached the intended user flow:

    Download ZIP
    → Extract anywhere
    → Run
    → Select your own media
    → Play

That success made the portable-only model the final distribution design.

---

# No installer

During release preparation, another design decision was made explicit:

**There will be no traditional installer.**

No MSI.

No setup package writing into Program Files.

No registry-based installation.

No files intentionally scattered around the Windows user profile.

The official release format is a portable ZIP.

The user extracts it wherever they want and runs the application from there.

If they want to move it, they move the folder.

If they want to back it up, they back up the folder.

That simplicity is intentional.

---

# Cleaning the repository

By the time the portable build worked, the main development directory had accumulated a large amount of local material:

- generated code
- build outputs
- temporary tests
- diagnostic traces
- old experiments
- extracted game media
- DLC test data
- portable preview packages
- evidence folders
- local tools

Most of that was useful during development.

Almost none of it belonged in the public repository.

A new clean working copy was therefore created from the Git repository baseline.

The clean tree preserved the legitimate current project source while excluding development clutter and proprietary material.

The clean repository included the current source for:

- Asset Setup Wizard
- native media inspection
- DLC handling
- portable path setup
- save isolation
- localization
- UI
- tests
- required build integration

It deliberately excluded:

- ISOs
- `default.xex`
- `ud1.bin`
- `ud2.bin`
- extracted game assets
- DLC packages
- saves
- shaders/cache
- logs
- compiled outputs
- portable ZIPs
- temporary diagnostics
- local-only developer files

The historical Git repository and PoC 0.1 remained intact.

---

# V1.0 release philosophy

By the time the V1.0 executable was ready, several principles had become part of the project itself.

## Portable first

The application should be usable from the directory where it was extracted.

No traditional installation is required.

---

## Bring your own game

The project does not distribute Infinite Undiscovery game assets.

Users provide their own legally obtained media.

---

## EXE-first setup

Normal users should not need to know how Xbox 360 media is structured.

They should not need to manually run development scripts.

The EXE handles the normal asset setup flow.

---

## Automatic media detection

The application identifies supported media automatically.

The user should not need to know whether their copy is PAL or NTSC-U before setup.

---

## Region isolation

PAL and NTSC-U can coexist without sharing assets, saves or runtime state.

---

## Local saves

The recomp uses its own local save data.

It does not silently adopt saves created by older development environments.

---

## Historical versions stay historical

PoC 0.1 remains a record of what the project actually looked like at that point in development.

It is not rewritten to match later capabilities.

---

## Do not claim what has not been tested

If something has not been demonstrated in real gameplay, the project should say so.

The clearest example is live Disc 1 → Disc 2 switching.

Having both discs installed is supported.

That does not automatically prove every in-game disc transition until it has actually been played and validated.

---

# V1.0 status

At the V1.0 executable milestone, Infinite Undiscovery Recomp had evolved from a low-level recompilation experiment into a portable PC application with:

- native recompiled execution
- integrated setup wizard
- native media inspection
- automatic PAL / NTSC-U detection
- Disc 1 / Disc 2 detection
- native asset extraction
- separate multi-disc asset storage
- DLC A / B validation
- STFS DLC installation through the runtime
- idempotent DLC behavior on later launches
- portable region-separated runtime data
- isolated local saves
- local shaders/cache/configuration
- English and Spanish setup UI
- English as the default language
- redesigned setup presentation
- subsequent-launch setup skipping
- no required external extraction scripts for normal users
- no installer
- no bundled copyrighted game assets

The remaining work around V1.0 is primarily release work and continued real-game validation rather than rebuilding the setup architecture from scratch.

---

# Known V1.0 validation items

Some areas remain intentionally documented as ongoing validation items.

These include:

- full-game testing
- live Disc 1 → Disc 2 switching during actual gameplay
- complete in-game verification of the DLC voucher effects
- any runtime issue that only appears much later in the game

These are not hidden behind a "fully complete" label.

They are simply the next things to validate as the project is played further.

---

# From experiment to portable release

The project began with a much smaller question:

> Can Infinite Undiscovery run as a native recompilation?

By V1.0, the question had changed.

The game could run.

The next challenge had become making the experience practical enough that someone else could use it without recreating the development environment.

That is what the integrated wizard and portable release were built to solve.

The final intended experience is deliberately simple:

    Download
    Extract
    Run
    Select your own game media
    Play

That is the direction Infinite Undiscovery Recomp will continue to follow.
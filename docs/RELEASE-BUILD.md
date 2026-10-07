# Release build & ReXGlue dependency

This document records the pinned dependency and the hardened release flow used
to produce a public Windows x64 build of Infinite Undiscovery Recomp.

## Required ReXGlue

The only supported runtime for a release is the auditable source checkout, not
the old precompiled SDK package.

| Item | Value |
|---|---|
| Repository | `rexglue-sdk-v0.10.0-git` (kept as an external sibling of this repo) |
| Tag | `v0.10.0` |
| Commit | `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` |
| Config | Windows AMD64 Release |

The ReXGlue checkout is **not** vendored into this repository. Keep it next to
the project and build an install tree from it:

```powershell
cmake --install <rexglue-sdk-v0.10.0-git>/out/build/win-amd64 `
      --config Release `
      --prefix  <sibling>/rexglue-0.10.0-install
```

Point the project build at that install tree with `CMAKE_PREFIX_PATH` /
`rexglue_DIR`. `REXSDK_DIR` (the `add_subdirectory` path) is not usable here:
the SDK's own version resolution fails when it is added as a subdirectory
(`rex_compute_version: floor version (0.10) is behind tag version (1.0)`), and a
fresh from-source build would not reproduce the exact release DLL bytes anyway.

## Pinned ReXGlue DLL hashes

Both DLLs must match these SHA-256 values in the final package:

```
25C0F2D1DBB7FE3147FC59E22C9A3E4C764DC2E0B0C6877FC86F0DA499E0F67F  rexruntime.dll
98CEF22E2AC1667F3A42910DD7474C0409BBEF7DB7599B3A48DF1EDBB0739A2D  rexgpu-xenos.dll
```

These are the old precompiled-SDK copies that must never ship again:

```
E359209FB2B0570E693C966D4C1D99A82465D36EF70D033833FAE56ADB2F1B7A  rexruntime.dll
0C23CFA23FC4FA5638DC8A3DC0DE87B1041D97705D4677774083F1F350CD8D89  rexgpu-xenos.dll
```

`cmake/verify_rexglue_dlls.cmake` encodes these values. It runs as a POST_BUILD
step of `InfiniteUndiscoveryRecomp.exe`, so a build that would stage the old
DLLs **fails immediately** with a clear message. It is also invoked by the
release script against both the build output and the staging folder.

The build requests the GPU plugin explicitly (`rexglue_setup_target(... GPU_PLUGINS xenos)`),
so the build output already contains both `rexruntime.dll` and
`rexgpu-xenos.dll`. No manual DLL replacement is needed.

## Reproduced toolchain

| Tool | Version |
|---|---|
| Clang / clang++ | 20.1.8 |
| MSVC | 14.44.35207 |
| Windows SDK | 10.0.26100.0 |
| CMake | 3.31.8 |
| Ninja | 1.12.1 |

## Building a release

Source a shell that has the toolchain on `PATH` (clang, cmake, ninja), then:

```powershell
powershell -ExecutionPolicy Bypass -File release/make-release.ps1 `
    -RexGlueInstall <sibling>/rexglue-0.10.0-install `
    -StagingDir    <sibling>/release-staging/v1.0.0-rc1-hardened `
    -ZipPath       <sibling>/release-staging/InfiniteUndiscoveryRecomp-v1.0.0-rc1-hardened.zip
```

The script:

1. validates the auditable ReXGlue install (aborts if it holds an old DLL);
2. configures a brand-new Release build directory;
3. builds `InfiniteUndiscoveryRecomp.exe`;
4. validates the ReXGlue DLLs staged next to the EXE;
5. builds a fresh staging folder (refuses to reuse a non-empty one);
6. writes `RELEASE_HASHES_SHA256.txt`;
7. creates the release ZIP;
8. runs a Microsoft Defender scan over the EXE, both DLLs, the staging folder
   and the ZIP.

The script never deletes anything; it requires new or empty destinations.

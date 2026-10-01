# Infinite Undiscovery Recomp

Preliminary PoC 0.1 for recompiling Infinite Undiscovery with ReXGlue 0.10.0. The reference configuration is Windows x64 Release / Direct3D 12.

The user manually verified launching by double-clicking, the embedded icon, game asset discovery, and Graad Prison running with experimental frame pacing. This validation does not cover the full game, other computers, or other platforms. The reference executable hash is recorded in `BUILD_INFO_POC_0.1.txt`.

## Game assets

Game assets are not included. Each user must supply the files from their own legally obtained copy of the game. Game-generated code, the SDK, local tools, and binaries are also excluded.

The current code generation configuration targets the PAL `default.xex` identified by its SHA-256 hash in `functions.toml`. Compatibility with other versions is not claimed.

To run the project, place the compiled EXE, `rexruntime.dll`, and `rexgpu-xenos.dll` alongside an `assets/` folder containing `default.xex`, `ud1.bin`, and `ud2.bin`. When no explicit path is provided, the code looks for `assets/` next to the EXE, then in the project root when running from `out/build/win-amd64-release/`. An explicitly supplied path takes priority:

```powershell
.\infinite_undiscovery.exe --game_data_root "<path to assets>"
```

An Asset Setup Wizard is not yet included.

## Code generation and build

ReXGlue SDK and CLI 0.10.0, CMake 3.25 or later, Ninja, Clang, and the MSVC/Windows SDK environment are required. Presets for other platforms are present, but this PoC is described as validated only on Windows x64.

1. Place your own game files in `assets/` and the CLI at `tools/rexglue/rexglue.exe`.
2. Install the ReXGlue SDK and prepare an environment with Clang, Ninja, and MSVC/Windows SDK available.
3. From the project root, replace the SDK path placeholder and run:

```powershell
.\run-codegen.ps1
cmake --preset win-amd64-release -DCMAKE_PREFIX_PATH="<path to installed SDK>"
cmake --build --preset win-amd64-release
```

The script generates `generated/` from the manifest and `functions.toml`; check its exit code before proceeding. `-Diagnostic` forces analysis and is reserved for diagnostics. CMake requires the generated `generated/rexglue.cmake` file and the SDK package configuration. Build output is located in `out/build/win-amd64-release/`; supply the corresponding runtime DLLs if they are not already there.

These instructions reflect the inspected local configuration. A build from a clean clone was not performed while preparing this documentation.

## Experimental frame pacing

The code enables the DXGI Frame Latency Waitable Object by default. Set `IU_EXPERIMENT_WAITABLE=0` to disable it for comparison. Optional traces use `IU_PERF_TRACE_PATH` and `IU_DIAG_TRACE_PATH`.

## License

The project code is licensed under BSD-3-Clause; see `LICENSE`. Game files and external dependencies retain their respective rights and licenses. The project license grants no rights to those materials.

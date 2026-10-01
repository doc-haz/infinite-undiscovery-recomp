# Infinite Undiscovery Recomp

Preliminary PoC 0.1 de recompilación de Infinite Undiscovery con ReXGlue 0.10.0. La configuración de referencia es Windows x64 Release / Direct3D 12.

El usuario confirmó manualmente el arranque por doble clic, el icono embebido, la resolución de assets y el funcionamiento de Graad Prison con el frame pacing experimental. Esto no cubre el juego completo, otros equipos ni otras plataformas. El hash del ejecutable de referencia figura en `BUILD_INFO_POC_0.1.txt`.

## Datos del juego

Los game assets no están incluidos. Cada usuario debe aportar los archivos de su propia copia obtenida legalmente. Tampoco se incluyen código generado a partir del juego, SDK, herramientas locales ni binarios.

La configuración actual de codegen utiliza el `default.xex` PAL identificado por SHA-256 en `functions.toml`. No se afirma compatibilidad con otras versiones.

Para ejecutar, reunir el EXE compilado, `rexruntime.dll`, `rexgpu-xenos.dll` y una carpeta `assets/` con `default.xex`, `ud1.bin` y `ud2.bin`. Sin ruta explícita, las fuentes buscan `assets/` junto al EXE y después en la raíz del proyecto cuando se ejecuta desde `out/build/win-amd64-release/`. La selección explícita conserva prioridad:

```powershell
.\infinite_undiscovery.exe --game_data_root "<ruta a assets>"
```

No se incluye todavía un Asset Setup Wizard.

## Generar y compilar

Se requiere ReXGlue SDK y CLI 0.10.0, CMake 3.25 o posterior, Ninja, Clang y el entorno de MSVC/Windows SDK. Los presets de otras plataformas están presentes, pero esta PoC solo se describe como validada en Windows x64.

1. Colocar los archivos propios del juego en `assets/` y el CLI en `tools/rexglue/rexglue.exe`.
2. Instalar el SDK de ReXGlue y preparar un entorno con Clang, Ninja y MSVC/Windows SDK disponibles.
3. Desde la raíz del proyecto, sustituir el marcador del SDK y ejecutar:

```powershell
.\run-codegen.ps1
cmake --preset win-amd64-release -DCMAKE_PREFIX_PATH="<ruta al SDK instalado>"
cmake --build --preset win-amd64-release
```

El script genera `generated/` a partir del manifest y `functions.toml`; revisar su código de salida antes de continuar. `-Diagnostic` fuerza el análisis y se reserva para diagnóstico. CMake necesita el archivo generado `generated/rexglue.cmake` y la configuración de paquete del SDK. La salida de compilación se encuentra en `out/build/win-amd64-release/`; aportar las DLL de runtime correspondientes si no se encuentran allí.

Estas instrucciones reflejan la configuración local inspeccionada; no se realizó una compilación desde un clon limpio durante esta preparación.

## Frame pacing experimental

Las fuentes habilitan por defecto DXGI Frame Latency Waitable Object. `IU_EXPERIMENT_WAITABLE=0` permite desactivarlo para comparar. Las trazas opcionales usan `IU_PERF_TRACE_PATH` e `IU_DIAG_TRACE_PATH`.

## Licencia

BSD-3-Clause para el código del proyecto; véase `LICENSE`. Los archivos del juego y las dependencias externas conservan sus respectivos derechos y licencias. La licencia del proyecto no concede derechos sobre esos materiales.

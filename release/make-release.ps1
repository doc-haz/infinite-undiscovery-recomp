# Infinite Undiscovery Recomp - hardened release build.
#
# Produces a release candidate from a brand-new Release build that is configured
# against the auditable ReXGlue v0.10.0 install tree (NOT the old precompiled
# SDK), and refuses to continue if any shipped ReXGlue DLL is an old copy.
#
# The script never deletes anything: it requires new/empty destinations.
#
# Microsoft Defender scans the staged files BEFORE the ZIP is produced and scans
# the ZIP afterwards; any non-zero scan result aborts immediately. -SkipDefender
# is available for offline/pre-scan situations but marks the AV gate as NOT
# satisfied (the build is then not publishable until scanned elsewhere).
#
# Example (source the toolchain environment first):
#   . C:/path/to/build-environment.ps1
#   powershell -ExecutionPolicy Bypass -File release/make-release.ps1

param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$RexGlueInstall = '',
    [string]$StagingDir = '',
    [string]$ZipPath = '',
    [string]$BuildDirName = '',
    [switch]$SkipDefender
)

$ErrorActionPreference = 'Stop'

function Fail([string]$Message) {
    Write-Host "ABORT: $Message" -ForegroundColor Red
    exit 1
}
function Info([string]$Message) { Write-Host "[release] $Message" }

if (-not $RexGlueInstall) {
    $RexGlueInstall = Join-Path (Split-Path -Parent $RepoRoot) 'rexglue-0.10.0-install'
}
$releaseRoot = Join-Path (Split-Path -Parent $RepoRoot) 'release-staging'
if (-not $StagingDir) { $StagingDir = Join-Path $releaseRoot 'v1.0.0-rc1' }
if (-not $ZipPath)    { $ZipPath    = Join-Path $releaseRoot 'InfiniteUndiscoveryRecomp-v1.0.0-rc1.zip' }

$guard = Join-Path $RepoRoot 'cmake\verify_rexglue_dlls.cmake'
if (-not (Test-Path $guard)) { Fail "missing hash guard: $guard" }

# --- Toolchain -------------------------------------------------------------
foreach ($tool in 'clang', 'clang++', 'cmake', 'ninja') {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Fail "toolchain '$tool' not on PATH. Source the build environment first."
    }
}

function Assert-RexGlueDlls([string]$Directory, [bool]$RequireAll) {
    $cmakeArgs = @("-DVERIFY_DIR=$Directory")
    if ($RequireAll) { $cmakeArgs += '-DREQUIRE_ALL=ON' }
    $cmakeArgs += @('-P', $guard)
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { Fail "ReXGlue DLL guard rejected '$Directory' (old or unexpected DLL)." }
}

# --- 1. Auditable ReXGlue SDK ---------------------------------------------
$sdkConfig = Join-Path $RexGlueInstall 'lib\cmake\rexglue\rexglueConfig.cmake'
if (-not (Test-Path $sdkConfig)) {
    Fail "auditable ReXGlue install not found at '$RexGlueInstall' (missing lib/cmake/rexglue/rexglueConfig.cmake)."
}
Info "ReXGlue install: $RexGlueInstall"
Assert-RexGlueDlls (Join-Path $RexGlueInstall 'bin') $true

# --- 2. Fresh build directory ---------------------------------------------
if (-not $BuildDirName) { $BuildDirName = 'release-rc-' + (Get-Date -Format 'yyyyMMdd-HHmmss') }
$buildDir = Join-Path $RepoRoot ('out\build\' + $BuildDirName)
if (Test-Path $buildDir) { Fail "build directory already exists: $buildDir (a release build must be new)." }

Info "Configuring fresh build: $buildDir"
& cmake -S $RepoRoot -B $buildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER=clang `
    -DCMAKE_CXX_COMPILER=clang++ `
    -DCMAKE_PREFIX_PATH="$RexGlueInstall" `
    "-Drexglue_DIR=$(Join-Path $RexGlueInstall 'lib\cmake\rexglue')"
if ($LASTEXITCODE -ne 0) { Fail 'CMake configure failed.' }

Info 'Building InfiniteUndiscoveryRecomp.exe'
& cmake --build $buildDir --target infinite_undiscovery
if ($LASTEXITCODE -ne 0) { Fail 'Build failed.' }

$exe = Join-Path $buildDir 'InfiniteUndiscoveryRecomp.exe'
if (-not (Test-Path $exe)) { Fail "EXE not produced: $exe" }

# The build stages both ReXGlue DLLs next to the EXE; the guard fails the build
# if they are the old precompiled copies.
Info 'Validating build-output ReXGlue DLLs'
Assert-RexGlueDlls $buildDir $true

# --- 3. Fresh staging ------------------------------------------------------
if (Test-Path $StagingDir) {
    if (@(Get-ChildItem $StagingDir -Force).Count -gt 0) {
        Fail "staging already exists and is not empty: $StagingDir (do not reuse an old RC)."
    }
} else {
    New-Item -ItemType Directory -Force -Path $StagingDir | Out-Null
}

Copy-Item -LiteralPath $exe -Destination $StagingDir
Copy-Item -LiteralPath (Join-Path $buildDir 'rexruntime.dll') -Destination $StagingDir
Copy-Item -LiteralPath (Join-Path $buildDir 'rexgpu-xenos.dll') -Destination $StagingDir
Copy-Item -LiteralPath (Join-Path $RepoRoot 'README.md') -Destination $StagingDir
Copy-Item -LiteralPath (Join-Path $RepoRoot 'LICENSE') -Destination $StagingDir
$licensesDir = Join-Path $RepoRoot 'LICENSES'
$noticesFile = Join-Path $RepoRoot 'THIRD_PARTY_NOTICES.txt'
if (-not (Test-Path $licensesDir))   { Fail "missing third-party license bundle: $licensesDir" }
if (-not (Test-Path $noticesFile))   { Fail "missing third-party notices: $noticesFile" }
Copy-Item -LiteralPath $noticesFile -Destination $StagingDir
Copy-Item -LiteralPath $licensesDir -Destination $StagingDir -Recurse

Info 'Validating staging ReXGlue DLLs'
Assert-RexGlueDlls $StagingDir $true

# --- 4. Manifest -----------------------------------------------------------
$manifestPath = Join-Path $StagingDir 'RELEASE_HASHES_SHA256.txt'
$manifestLines = @('# Infinite Undiscovery Recomp v1.0.0-rc1',
                   '# SHA-256 manifest (the files that form the release)',
                   "# Generated $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')",
                   '')
$releaseFiles = Get-ChildItem $StagingDir -Recurse -File |
    Where-Object { $_.Name -ne 'RELEASE_HASHES_SHA256.txt' } |
    Sort-Object FullName
foreach ($file in $releaseFiles) {
    $rel = $file.FullName.Substring($StagingDir.Length).TrimStart('\')
    $hash = (Get-FileHash $file.FullName -Algorithm SHA256).Hash
    $manifestLines += ('{0}  {1}' -f $hash, $rel)
}
$manifestLines | Set-Content -Path $manifestPath -Encoding ASCII

# --- 5. Defender scan (staged files, before the public ZIP) ----------------
function Invoke-DefenderScan([string]$Target) {
    Info "Defender scan: $Target"
    & $mp -Scan -ScanType 3 -File "$Target"
    $rc = $LASTEXITCODE
    if ($rc -ne 0) {
        Fail "Microsoft Defender reported a detection or failed for '$Target' (exit code $rc). Aborting; do NOT publish this build."
    }
    Info '  -> no threats (exit code 0)'
}
if ($SkipDefender) {
    Write-Warning 'Defender scan skipped (-SkipDefender). The AV gate is NOT satisfied; do not publish until scanned.'
} else {
    $mp = Join-Path $env:ProgramFiles 'Windows Defender\MpCmdRun.exe'
    if (-not (Test-Path $mp)) {
        Fail "Microsoft Defender CLI not found: $mp (pass -SkipDefender only if you accept an unscanned AV gate)."
    }
    Invoke-DefenderScan (Join-Path $StagingDir 'InfiniteUndiscoveryRecomp.exe')
    Invoke-DefenderScan (Join-Path $StagingDir 'rexruntime.dll')
    Invoke-DefenderScan (Join-Path $StagingDir 'rexgpu-xenos.dll')
    Invoke-DefenderScan $StagingDir
}

# --- 6. ZIP ----------------------------------------------------------------
if (Test-Path $ZipPath) { Fail "zip already exists: $ZipPath (refusing to overwrite)." }
Compress-Archive -Path (Join-Path $StagingDir '*') -DestinationPath $ZipPath -CompressionLevel Optimal

if (-not $SkipDefender) { Invoke-DefenderScan $ZipPath }

# --- 7. Summary ------------------------------------------------------------
Info 'Release artifacts:'
Get-ChildItem $StagingDir -Recurse -File | Sort-Object FullName | ForEach-Object {
    '{0,12}  {1}  {2}' -f $_.Length, (Get-FileHash $_.FullName -Algorithm SHA256).Hash, $_.FullName.Substring($StagingDir.Length).TrimStart('\')
}
$zipItem = Get-Item $ZipPath
Info ("ZIP: {0}  ({1} bytes)  {2}" -f $zipItem.FullName, $zipItem.Length, (Get-FileHash $ZipPath -Algorithm SHA256).Hash)

exit 0

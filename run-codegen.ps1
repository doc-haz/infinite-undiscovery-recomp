param([switch]$Diagnostic)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $cli = Join-Path $PSScriptRoot 'tools/rexglue/rexglue.exe'
    if ($Diagnostic) {
        & $cli --force --log-level debug codegen --ignore-stamp *> codegen-diagnostic-rerun.log
    } else {
        & $cli codegen --ignore-stamp *> codegen-validation-rerun.log
    }
    $result = $LASTEXITCODE
} finally { Pop-Location }
exit $result

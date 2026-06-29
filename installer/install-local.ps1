# VOSK — quick local install (no Inno Setup needed).
# Copies the built VST3 into the system VST3 folder. Run from an elevated
# PowerShell (Run as administrator):  powershell -ExecutionPolicy Bypass -File installer\install-local.ps1
#
# Factory presets are baked into the plugin; user presets live in
# Documents\MantisVex\VOSK\Presets and are not touched.

$ErrorActionPreference = "Stop"
$root   = Split-Path -Parent $PSScriptRoot
$src    = Join-Path $root "build\VOSK_artefacts\Release\VST3\VOSK.vst3"
$destDir = Join-Path $env:CommonProgramFiles "VST3"
$dest    = Join-Path $destDir "VOSK.vst3"

if (-not (Test-Path $src)) {
    Write-Error "Build the plugin first (Release). Not found: $src"
}

New-Item -ItemType Directory -Force -Path $destDir | Out-Null
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
Copy-Item -Recurse -Force $src $dest

Write-Host "Installed VOSK.vst3 -> $dest" -ForegroundColor Green
Write-Host "Rescan plugins in your DAW to pick it up."

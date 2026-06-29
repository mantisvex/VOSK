# VOSK — installation

Factory presets are compiled into the plugin, so they ship automatically. User
presets are stored in `Documents\MantisVex\VOSK\Presets\*.voskpreset`.

## Option A — distributable installer (recommended for release)

1. Build the plugin in Release: `cmake --build build --config Release`
2. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php) (free).
3. Compile `installer\VOSK.iss` (open it and press F9, or run `ISCC.exe installer\VOSK.iss`).
4. The setup `.exe` lands in `installer\Output\`. It installs the VST3 to the system
   VST3 folder and (optionally) the standalone app.

## Option B — quick local install (no extra tools)

From an **elevated** PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File installer\install-local.ps1
```

This copies the built `VOSK.vst3` into `C:\Program Files\Common Files\VST3\`.
Rescan plugins in your DAW afterwards.

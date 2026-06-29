; VOSK installer — Inno Setup script (https://jrsoftware.org/isinfo.php, free).
; Build the plugin first (Release), then compile this with Inno Setup 6:
;   ISCC.exe installer\VOSK.iss   (or open it in the Inno Setup IDE and press F9)
; Factory presets are baked into the binary, so nothing extra needs shipping.
; User presets live in Documents\MantisVex\VOSK\Presets and are left untouched.

#define MyAppName    "VOSK"
#define MyAppVersion "0.1.0"
#define MyPublisher  "MantisVex"
#define ArtefactDir  "..\build\VOSK_artefacts\Release"

[Setup]
AppId={{4F3A1C2E-9B77-4D55-9E21-VOSKMANTISVEX}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyPublisher}
DefaultDirName={autopf}\{#MyPublisher}\{#MyAppName}
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=VOSK-{#MyAppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName} {#MyAppVersion}

[Components]
Name: "vst3";       Description: "VST3 plugin";   Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone app"; Types: full

[Files]
; VST3 bundle -> the standard system VST3 folder.
Source: "{#ArtefactDir}\VST3\VOSK.vst3\*"; DestDir: "{commoncf64}\VST3\VOSK.vst3"; \
    Flags: recursesubdirs createallsubdirs ignoreversion; Components: vst3
; Standalone application (optional).
Source: "{#ArtefactDir}\Standalone\VOSK.exe"; DestDir: "{app}"; \
    Flags: ignoreversion; Components: standalone

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\VOSK.exe"; Components: standalone

[Run]
Filename: "{app}\VOSK.exe"; Description: "Launch VOSK"; \
    Flags: nowait postinstall skipifsilent; Components: standalone

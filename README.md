# HandBrake Preset Creator

A Windows desktop editor for HandBrake preset JSON files, built with **C++20 and Qt 6 Widgets**. Create presets from existing templates, edit a whole library, and save changes without losing unexposed parameters or folder metadata.

This application edits preset files. It does not scan media or encode video, and is not affiliated with the HandBrake project.

## Features

- **Easy view:** familiar Summary, Dimensions, Filters, Video, Audio, Subtitles, and Chapters tabs.
- **Advanced view:** searchable, typed editing of every parameter present in a preset, including nested audio templates and language lists.
- Add a preset to the current JSON library, or create a separate preset file from a selected template.
- Delete individual presets with confirmation; other presets and folders stay intact.
- Named resolution limits, crop/anamorphic/padding dependencies, custom RGB fill colors, and expanding custom filter fields.
- Encoder-specific preset, tune, profile, level, quality, and multi-pass controls for common encoder families.
- Light and dark themes, with a remembered theme preference.
- On-demand JSON preview, atomic saves, and unsaved-change prompts.

## Requirements

| Component | Requirement |
| --- | --- |
| Operating system | Windows x64 |
| IDE/compiler | Visual Studio 2026, **Desktop development with C++**, MSVC v145 and a Windows SDK |
| GUI toolkit | Qt 6.12 MSVC x64 kit with Qt Core, GUI, and Widgets (tested configuration) |
| HandBrake | Needed to export templates and use the finished presets; not required to build or open the editor |

Install Qt using the [Qt installer](https://doc.qt.io/qt-6/get-and-install-qt.html). Choose an **MSVC x64** kit; a MinGW kit cannot be linked into this Visual Studio project. CMake and the Qt VS Tools extension are not required. Other toolchain/Qt combinations are not currently verified.

## Build and run

### Visual Studio

1. Clone this repository and open `HandBrakePresetCreator.slnx`.
2. Copy `Qt.local.props.example` to **`Qt.local.props`**.
3. Edit its `QtRoot` value to your installed kit directory, for example `C:\Qt\6.12.0\msvc2022_64`.
4. Select **Debug | x64** or **Release | x64**.
5. Set **HandBrakePresetCreator** as the startup project and press **F5**.

`Qt.local.props` is ignored by Git. Keep machine-specific paths there; do not edit the shared project file for your installation. The debugger uses this path to locate Qt DLLs and plugins.

### Command line

From **Developer PowerShell for Visual Studio**, in the repository root:

```powershell
$env:QT_ROOT = 'C:\Qt\6.12.0\msvc2022_64' # Replace with your kit
msbuild .\HandBrakePresetCreator.vcxproj /p:Configuration=Release /p:Platform=x64

$env:PATH = "$env:QT_ROOT\bin;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "$env:QT_ROOT\plugins\platforms"
& .\build\Release\HandBrakePresetCreator.exe
```

An explicit `/p:QtRoot=...` MSBuild argument can also select the kit. If a local props file is present, it takes priority over `QT_ROOT`; an explicit MSBuild argument takes priority over both. Build outputs and intermediates go to `build/` and are ignored by Git.

To open a file at startup:

```powershell
& .\build\Release\HandBrakePresetCreator.exe '.\presets.json'
```

## Quick usage guide

### Create a preset

1. Click **Open presets…** and choose the included `presets.json`, or a preset export from your HandBrake installation.
2. Select the preset you want to use as a starting point.
3. Open **New preset** and choose:
   - **Add to current JSON file** to keep the new preset alongside the existing library.
   - **Create in a new JSON file…** to create a separate file containing only the selected copy, with its version metadata.
4. Enter a name and adjust settings in **Easy view**. Use **Advanced view** for less common parameters.
5. Click **Save**. New files prompt for a location. **Save as…** writes a separate copy of the whole current library.
6. Import the saved JSON through HandBrake's preset import action, then check the resulting settings before encoding.

### Edit or delete a preset

- Open a JSON library and select a preset in the left-hand list. **Save** updates that file; **Save as…** keeps the original file and writes a copy.
- In Advanced view, double-click a value to edit it. Expand lists to access entries; right-click a list or entry to add, duplicate, or remove items.
- **Add setting…** copies a missing parameter and its initial value from another preset in the loaded library.
- Click **Delete preset…**, or press **Delete** while the library list has focus. Confirm the name. The deletion reaches the JSON file only when you save.
- The final preset can be deleted too: the resulting empty library can be saved and reopened. Adding to an empty library lets you choose a template from another file.

### Easy-view tips

| Tab | Useful controls |
| --- | --- |
| Summary | Container, MP4-specific options, metadata, preset description |
| Dimensions | 480p–8K limits, Unlimited/Custom, cropping, anamorphic, Letterbox/Pillarbox/Fill/Custom padding |
| Filters | Algorithm-dependent presets and tunes; select **Custom** to reveal the expression field; hover for explanations |
| Video | Encoder, Same as source/FPS, CFR/VFR, whole-number quality, average bitrate, and encoder options |
| Audio | Language codes, track selection, passthrough/fallback, and add/duplicate/remove encoder templates |
| Subtitles | Language codes, foreign-audio search, selection, and burn-in behavior |
| Chapters | Include chapter markers; source-specific names and timestamps are handled in HandBrake |

Disabled fields are inactive for the current mode: for example, pixel aspect requires Custom anamorphic, border sizes require Custom padding, and hardware encoders have different multi-pass/tune support. Both views share the same in-memory preset. Switching views does not replace unknown fields or insert defaults into untouched settings.

See the [filter reference](docs/filter-reference.md) for preset families, tunes, and explanations.

## Included template

`presets.json` is a shareable starting library based on HandBrake presets. It retains the supplied folders, encoder settings, and preset-version metadata. Personal custom presets are not included; default-selection and expanded-folder flags are reset. It contains hardware and platform-specific examples, so not every preset is usable on every machine.

For compatibility with your HandBrake version, exporting a template from that installation is the most reliable starting point. Use **Save as…** or create a new file when experimenting with the included template. Keep personal libraries outside the repository or under the ignored `local-presets/` directory.

## Verification

Tests are compiled into a separate console executable; they are not included in the application. Build the Tests project in the solution, then run the six regression checks from PowerShell:

```powershell
.\scripts\Test.ps1 -Configuration Debug
# Or build the Tests project and run it from Developer PowerShell:
.\scripts\Test.ps1 -Configuration Release -Build
```

The script uses `Qt.local.props`, `QT_ROOT`, or its `-QtRoot` argument. `-PresetFile` can select another library; the complete UI checks expect the supplied template or a fixture containing x264, NVENC, and audio-template parameters. The tests exercise JSON preservation, typed edits, view synchronization, dependencies, RGB validation, template creation, deletion confirmation, and saving/reopening an empty library. Test outputs use temporary files; the supplied library is not saved over. Some checks briefly display dialogs that the test handles automatically.

The separate build/<configuration>/tests/HandBrakePresetCreator.Tests.exe accepts --self-test, --delete-test, --editor-test, --easy-test, --ui-rules-test, and --delete-ui-test. The last four require a library filename before the switch. Exit code 0 means success. The production application accepts an optional preset-file path and --help; it contains no test switches, test fixtures, or screenshot helpers.

## Troubleshooting

- **Qt Widgets not found:** point `QtRoot` at the kit folder containing `include/QtWidgets`, `lib/`, `bin/`, and `plugins/`, rather than the top-level Qt installer directory.
- **Missing Qt DLL or platform plugin:** launch using F5, configure the shell environment as above, or deploy the application with Qt's deployment tool.
- **Cannot overwrite the executable (LNK1168):** stop the running application or debugger and rebuild.
- **Setting is missing in Easy view:** inspect it in Advanced view, or add it from another template in the library.
- **HandBrake rejects a preset:** check its version and encoder/container/filter support against the target installation. The editor does not perform a HandBrake import or encoding validation.

## Creating a binary distribution

A build executable alone is not a portable package. Qt DLLs and platform plugins must accompany it. For a Release build, a starting deployment sequence is:

```powershell
New-Item -ItemType Directory -Force .\dist | Out-Null
Copy-Item .\build\Release\HandBrakePresetCreator.exe .\dist\
& "$env:QT_ROOT\bin\windeployqt.exe" --release --no-compiler-runtime .\dist\HandBrakePresetCreator.exe
Copy-Item .\presets.json, .\LICENSE.txt, .\THIRD_PARTY_NOTICES.md .\dist\
Copy-Item .\docs\licenses .\dist\licenses -Recurse
```

Provide the official Visual C++ Redistributable required by your toolchain and the relevant Qt notices/license materials. See [Qt's Windows deployment documentation](https://doc.qt.io/qt-6/windows-deployment.html). Validate the package on a machine without the development kit before distributing it. No installer or signed binary is included in this repository.

## Project layout

| File | Purpose |
| --- | --- |
| `main.cpp` | Production application startup and command-line handling |
| `editor.h` | Library management, Advanced view, and file operations |
| `easyview.h` | Easy-view forms and dependencies |
| `presetdocument.h` | JSON traversal, copying, adding, and deletion |
| `handbrakeoptions.h` | Researched filter and encoder option data |
| `theme.h` | Light/dark palettes, styles, and generated control icons |
| `presets.json` | Shared template library |
| `scripts/Test.ps1` | Builds/runs the separate regression-test executable |

The tests/ directory contains the separate Visual Studio test project, test fixtures, and integration checks.

## Scope and limitations

- Windows x64 is the supported build target. The editor does not probe GPU availability or call HandBrake/libhb.
- Common encoder families have explicit capability data; other encoders use options scoped to that encoder in the loaded library. Full backend compatibility and custom filter-expression validation remain with HandBrake.
- Choosing a different encoder replaces incompatible dependent options and clears old extra arguments. Opening a file or switching views does not do this.
- Easy view displays whole-number video quality; untouched fractional imports remain intact and are editable in Advanced view.
- Theme preference is stored locally with Qt settings. The application contains no application-level network requests or telemetry; preset data is read/written as local files.

## License and acknowledgments

The independently implemented application source is licensed under the [MIT License](LICENSE.txt). The included HandBrake-derived preset data has separate upstream terms, documented in [third-party notices](THIRD_PARTY_NOTICES.md). Qt is a separate dependency with its own license terms.

The GUI organization and parameter mappings were researched from the [HandBrake source repository](https://github.com/HandBrake/HandBrake), particularly its Windows settings views, preset mapping, and native filter/encoder tables. No HandBrake binaries or implementation files are bundled with the application source.

## Contributing and publishing

Keep local Qt paths in the ignored `Qt.local.props`, personal presets in `local-presets/`, and generated files under `build/` or `dist/`. Run the test script before submitting changes. Include a description of the behavior changed and the checks run.

Before committing, review `git diff --cached` and the author/committer identity. Git commits contain identity metadata independently of the source files; `.gitignore` cannot remove metadata already present in history. Configure your preferred public or GitHub-provided noreply identity before creating public commits. Rewriting previously pushed history is a separate operation requiring coordination with anyone using that history.

Use Git's tracked files for a source release rather than zipping the entire working directory, which can include ignored IDE state and personal settings.

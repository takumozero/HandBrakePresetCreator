# HandBrake Preset Creator

C++20 and Qt 6 Widgets application using a native Visual Studio solution.

## Build and debug

1. Open HandBrakePresetCreator.slnx in Visual Studio 2026 with Desktop development with C++ installed.
2. Select Debug and x64.
3. Set HandBrakePresetCreator as the startup project and press F5.

The project defaults to the Qt kit at C:\Qt\6.12.0\msvc2022_64.
For a different installation, override the QtRoot MSBuild property with your
MSVC x64 kit directory. Both Debug and Release link their matching Qt libraries.
The debugger environment includes Qt DLLs and the Windows platform plugin.

This first milestone displays a minimal window. Preset editing is not implemented yet.
No Qt VS Tools extension is required for this window. Custom Q_OBJECT classes and
Designer forms will need Qt build-tool integration when introduced.

The executable currently depends on the installed Qt kit; it is not a standalone
deployment package.

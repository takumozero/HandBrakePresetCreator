[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$QtRoot = $env:QT_ROOT,
    [string]$PresetFile,
    [switch]$Build
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $QtRoot) { $QtRoot = $env:QtRoot }
if (-not $QtRoot) {
    $localProps = Join-Path $projectRoot 'Qt.local.props'
    if (Test-Path -LiteralPath $localProps) {
        [xml]$settings = Get-Content -LiteralPath $localProps -Raw
        $kitNode = $settings.SelectSingleNode("//*[local-name()='QtRoot']")
        if ($kitNode) { $QtRoot = $kitNode.InnerText.Trim() }
    }
}
if (-not $QtRoot -or -not (Test-Path -LiteralPath (Join-Path $QtRoot 'bin\Qt6Core.dll'))) {
    throw 'Set QT_ROOT, pass -QtRoot, or configure Qt.local.props with an MSVC x64 Qt kit.'
}
if (-not $PresetFile) { $PresetFile = Join-Path $projectRoot 'presets.json' }
$PresetFile = (Resolve-Path -LiteralPath $PresetFile).Path
if ($Build) {
    & msbuild (Join-Path $projectRoot 'tests\HandBrakePresetCreator.Tests.vcxproj') /nologo /v:minimal "/p:Configuration=$Configuration" /p:Platform=x64 "/p:QtRoot=$QtRoot"
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE." }
}
$executable = Join-Path $projectRoot "build\$Configuration\tests\HandBrakePresetCreator.Tests.exe"
if (-not (Test-Path -LiteralPath $executable)) { throw 'Build the separate Tests project first, or run this script with -Build in Developer PowerShell.' }
$previousPath = $env:PATH
$previousPluginPath = $env:QT_QPA_PLATFORM_PLUGIN_PATH
$reportDirectory = Join-Path ([IO.Path]::GetTempPath()) ('HandBrakePresetCreator-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $reportDirectory | Out-Null
try {
    $env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = Join-Path $QtRoot 'plugins\platforms'
    foreach ($flag in @('--self-test', '--delete-test', '--editor-test', '--easy-test', '--ui-rules-test', '--delete-ui-test')) {
        $arguments = if ($flag -in @('--self-test', '--delete-test')) { "$flag --light" } else { '"' + $PresetFile + '" ' + $flag + ' --light' }
        $report = Join-Path $reportDirectory ($flag.TrimStart('-') + '.log')
        $process = Start-Process -FilePath $executable -ArgumentList $arguments -WorkingDirectory $projectRoot -WindowStyle Hidden -RedirectStandardError $report -Wait -PassThru
        if (Test-Path -LiteralPath $report) { Get-Content -LiteralPath $report | Write-Output }
        if ($process.ExitCode -ne 0) { throw "$flag failed with exit code $($process.ExitCode)." }
        Write-Output "PASS $flag"
    }
} finally {
    $env:PATH = $previousPath
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = $previousPluginPath
    # Remove only files in this run's dedicated directory; no recursive deletion.
    Get-ChildItem -LiteralPath $reportDirectory -File | Remove-Item -Force
    Remove-Item -LiteralPath $reportDirectory
}

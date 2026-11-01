param([string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) { $cmakePath = $cmake.Source }
else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio Build Tools with Desktop development with C++ and CMake.' }
    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $cmakePath = Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
}
if (!(Test-Path -LiteralPath $cmakePath)) { throw 'CMake not found. Install CMake 3.22 or later.' }
$ctestPath = Join-Path (Split-Path $cmakePath) 'ctest.exe'
& $cmakePath -S $projectRoot -B (Join-Path $projectRoot 'build') -A x64
if ($LASTEXITCODE) { throw 'Configuration failed.' }
& $cmakePath --build (Join-Path $projectRoot 'build') --config $Configuration --parallel 3 -- /nodeReuse:false
if ($LASTEXITCODE) { throw 'Build failed.' }
& $ctestPath --test-dir (Join-Path $projectRoot 'build') -C $Configuration --output-on-failure
if ($LASTEXITCODE) { throw 'Tests failed.' }
Write-Host "Built: $projectRoot\build\Undertone_artefacts\$Configuration\Undertone.exe"

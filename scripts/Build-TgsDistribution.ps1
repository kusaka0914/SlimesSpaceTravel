param(
    [string]$BuildDirectory = "build-tgs",
    [string]$Configuration = "Release",
    [string]$VcpkgRoot = $env:VCPKG_ROOT
)

$ErrorActionPreference = "Stop"

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
if ([string]::IsNullOrWhiteSpace($VcpkgRoot)) {
    $VcpkgRoot = "C:\dev\vcpkg"
}

$resolvedVcpkgRoot = [IO.Path]::GetFullPath($VcpkgRoot)
$toolchainFile = Join-Path $resolvedVcpkgRoot `
    "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path -LiteralPath $toolchainFile -PathType Leaf)) {
    throw "vcpkg toolchain was not found: $toolchainFile"
}

if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    $resolvedBuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
} else {
    $resolvedBuildDirectory = [IO.Path]::GetFullPath(
        (Join-Path $projectRoot $BuildDirectory))
}

& cmake `
    -S $projectRoot `
    -B $resolvedBuildDirectory `
    -DGAME_TGS_BUILD=ON `
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile"
if ($LASTEXITCODE -ne 0) {
    throw "TGS build configuration failed with exit code $LASTEXITCODE"
}

& cmake `
    --build $resolvedBuildDirectory `
    --config $Configuration `
    --target package_windows_tgs
if ($LASTEXITCODE -ne 0) {
    throw "TGS distribution build failed with exit code $LASTEXITCODE"
}

Write-Host "TGS distribution created: $(Join-Path $projectRoot 'dist-tgs')"

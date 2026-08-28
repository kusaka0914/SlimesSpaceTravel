param(
    [string]$BuildDirectory = "build",
    [string]$Configuration = "Release",
    [string]$OutputDirectory = "dist",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))

function Resolve-ProjectPath([string]$path) {
    if ([IO.Path]::IsPathRooted($path)) {
        return [IO.Path]::GetFullPath($path)
    }
    return [IO.Path]::GetFullPath((Join-Path $projectRoot $path))
}

function Get-RelativeChildPath(
    [string]$parentDirectory,
    [string]$childPath) {
    $normalizedParent = [IO.Path]::GetFullPath($parentDirectory).TrimEnd(
        [IO.Path]::DirectorySeparatorChar) +
        [IO.Path]::DirectorySeparatorChar
    $normalizedChild = [IO.Path]::GetFullPath($childPath)
    if (-not $normalizedChild.StartsWith(
            $normalizedParent,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside its expected parent: $normalizedChild"
    }
    return $normalizedChild.Substring($normalizedParent.Length)
}

$resolvedBuildDirectory = Resolve-ProjectPath $BuildDirectory
$resolvedOutputDirectory = Resolve-ProjectPath $OutputDirectory
$projectPrefix = $projectRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) +
    [IO.Path]::DirectorySeparatorChar

if (-not $resolvedOutputDirectory.StartsWith(
        $projectPrefix,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDirectory must be inside the project: $resolvedOutputDirectory"
}

if (-not $SkipBuild) {
    & cmake --build $resolvedBuildDirectory --config $Configuration --target game
    if ($LASTEXITCODE -ne 0) {
        throw "Release build failed with exit code $LASTEXITCODE"
    }
}

$configurationDirectory = Join-Path $resolvedBuildDirectory $Configuration
$gameExecutable = Join-Path $configurationDirectory "game.exe"
if (-not (Test-Path -LiteralPath $gameExecutable -PathType Leaf)) {
    throw "Game executable was not found: $gameExecutable"
}

if (Test-Path -LiteralPath $resolvedOutputDirectory) {
    Remove-Item -LiteralPath $resolvedOutputDirectory -Recurse -Force
}

$distributionBinDirectory = Join-Path $resolvedOutputDirectory "bin"
$distributionAssetsDirectory = Join-Path $resolvedOutputDirectory "assets"
$distributionModelsDirectory = Join-Path $distributionAssetsDirectory "models"
New-Item -ItemType Directory -Path $distributionBinDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $distributionModelsDirectory -Force | Out-Null

Copy-Item -LiteralPath $gameExecutable -Destination $distributionBinDirectory
Get-ChildItem -LiteralPath $configurationDirectory -Filter "*.dll" -File |
    Copy-Item -Destination $distributionBinDirectory

foreach ($assetDirectoryName in @("audio", "fonts", "textures", "videos")) {
    $sourceDirectory = Join-Path $projectRoot "assets\$assetDirectoryName"
    if (Test-Path -LiteralPath $sourceDirectory -PathType Container) {
        Copy-Item -LiteralPath $sourceDirectory -Destination $distributionAssetsDirectory -Recurse
    }
}

$sourceDataDirectory = Join-Path $projectRoot "assets\data"
$distributionDataDirectory = Join-Path $distributionAssetsDirectory "data"
$excludedDataPaths = @(
    [IO.Path]::GetFullPath((Join-Path $sourceDataDirectory "save")),
    [IO.Path]::GetFullPath((Join-Path $sourceDataDirectory "stage\ugc_saves"))
)
$excludedDataFiles = @(
    [IO.Path]::GetFullPath((Join-Path $sourceDataDirectory "stage\ugc_stage.yaml")),
    [IO.Path]::GetFullPath((Join-Path $sourceDataDirectory "stage\test.yaml"))
)

Get-ChildItem -LiteralPath $sourceDataDirectory -Recurse -File | ForEach-Object {
    $sourceFilePath = $_.FullName
    $isExcludedDirectory = $false
    foreach ($excludedDirectory in $excludedDataPaths) {
        $excludedPrefix = $excludedDirectory.TrimEnd(
            [IO.Path]::DirectorySeparatorChar) +
            [IO.Path]::DirectorySeparatorChar
        if ($sourceFilePath.StartsWith(
                $excludedPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            $isExcludedDirectory = $true
            break
        }
    }
    if ($isExcludedDirectory -or $excludedDataFiles -contains $sourceFilePath) {
        return
    }

    $relativePath = Get-RelativeChildPath $sourceDataDirectory $sourceFilePath
    $destinationFile = Join-Path $distributionDataDirectory $relativePath
    New-Item -ItemType Directory -Path (
        Split-Path $destinationFile -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $sourceFilePath -Destination $destinationFile
}

$modelReferenceFiles = @()
$modelReferenceFiles += Get-ChildItem (Join-Path $projectRoot "src") -Recurse -File |
    Where-Object { $_.Extension -in @(".cpp", ".h", ".hpp") }
$modelReferenceFiles += Get-ChildItem $sourceDataDirectory -Recurse -File |
    Where-Object {
        $_.Extension -eq ".yaml" -and
        -not $_.FullName.Contains("\ugc_saves\") -and
        $_.Name -notin @("ugc_stage.yaml", "test.yaml")
    }
$modelReferenceText = [Text.StringBuilder]::new()
foreach ($referenceFile in $modelReferenceFiles) {
    [void]$modelReferenceText.AppendLine(
        [IO.File]::ReadAllText($referenceFile.FullName))
}
$allReferenceText = $modelReferenceText.ToString()

$sourceModelsDirectory = Join-Path $projectRoot "assets\models"
$modelFileExtensions = @(
    ".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".ply", ".stl", ".assxml")
$selectedModelFiles = Get-ChildItem -LiteralPath $sourceModelsDirectory -Recurse -File |
    Where-Object {
        $_.Extension.ToLowerInvariant() -in $modelFileExtensions -and
        $allReferenceText.IndexOf(
            $_.Name,
            [StringComparison]::OrdinalIgnoreCase) -ge 0
    }

$copiedModelPaths = [Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase)
$copiedModelTextureDirectories = [Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase)

function Copy-ModelFile([IO.FileInfo]$sourceModelFile) {
    if (-not $copiedModelPaths.Add($sourceModelFile.FullName)) {
        return
    }

    $relativeModelPath = Get-RelativeChildPath `
        $sourceModelsDirectory `
        $sourceModelFile.FullName
    $destinationModelFile = Join-Path $distributionModelsDirectory $relativeModelPath
    New-Item -ItemType Directory -Path (
        Split-Path $destinationModelFile -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $sourceModelFile.FullName -Destination $destinationModelFile
}

function Copy-ModelTextureDirectory([IO.DirectoryInfo]$modelDirectory) {
    $textureDirectory = Get-ChildItem -LiteralPath $modelDirectory.FullName -Directory |
        Where-Object { $_.Name -ieq "Textures" } |
        Select-Object -First 1
    if (-not $textureDirectory -or
        -not $copiedModelTextureDirectories.Add($textureDirectory.FullName)) {
        return
    }

    $relativeTextureDirectory = Get-RelativeChildPath `
        $sourceModelsDirectory `
        $textureDirectory.FullName
    $destinationTextureDirectory = Join-Path `
        $distributionModelsDirectory `
        $relativeTextureDirectory
    New-Item -ItemType Directory -Path (
        Split-Path $destinationTextureDirectory -Parent) -Force | Out-Null
    Copy-Item `
        -LiteralPath $textureDirectory.FullName `
        -Destination $destinationTextureDirectory `
        -Recurse
}

foreach ($selectedModelFile in $selectedModelFiles) {
    Copy-ModelFile $selectedModelFile
    Copy-ModelTextureDirectory $selectedModelFile.Directory
    if ($selectedModelFile.Extension -ine ".obj") {
        continue
    }

    $objText = [IO.File]::ReadAllText($selectedModelFile.FullName)
    $materialMatches = [regex]::Matches(
        $objText,
        "(?im)^\s*mtllib\s+(?<file>.+?)\s*$")
    foreach ($materialMatch in $materialMatches) {
        $materialFileName = $materialMatch.Groups["file"].Value.Trim()
        $materialFilePath = Join-Path $selectedModelFile.DirectoryName $materialFileName
        if (-not (Test-Path -LiteralPath $materialFilePath -PathType Leaf)) {
            throw "Material referenced by model was not found: $materialFilePath"
        }
        Copy-ModelFile (Get-Item -LiteralPath $materialFilePath)
    }
}

$sourceShadersDirectory = Join-Path $projectRoot "shaders"
Copy-Item -LiteralPath $sourceShadersDirectory -Destination $resolvedOutputDirectory -Recurse

$itchManifest = @'
[[actions]]
name = "play"
path = "bin/game.exe"
platform = "windows"
'@
Set-Content -LiteralPath (
    Join-Path $resolvedOutputDirectory ".itch.toml") -Value $itchManifest -Encoding utf8

$allSourceModelFiles = Get-ChildItem -LiteralPath $sourceModelsDirectory -Recurse -File
$copiedModelFiles = Get-ChildItem -LiteralPath $distributionModelsDirectory -Recurse -File
$sourceModelBytes = ($allSourceModelFiles | Measure-Object Length -Sum).Sum
$copiedModelBytes = ($copiedModelFiles | Measure-Object Length -Sum).Sum
$excludedModelMiB = [math]::Round(
    ($sourceModelBytes - $copiedModelBytes) / 1MB,
    2)
$distributionBytes = (
    Get-ChildItem -LiteralPath $resolvedOutputDirectory -Recurse -File |
        Measure-Object Length -Sum).Sum

Write-Host "Distribution created: $resolvedOutputDirectory"
Write-Host "Copied model files: $($copiedModelFiles.Count)"
Write-Host "Excluded model data: $excludedModelMiB MiB"
Write-Host "Distribution size: $([math]::Round($distributionBytes / 1MB, 2)) MiB"

# package-zip.ps1
# Builds a self-contained Compendia zip archive for direct distribution.
#
# Usage (from repo root, after a Release build in Qt Creator):
#   .\package-zip.ps1 -BuildDir build
#
# Prerequisites:
#   - CMake Release build completed in Qt Creator (or via cmake --build)
#   - Model .dat files present in models/
#     (if missing: cmake -P cmake/DownloadModels.cmake)
#
# Output:
#   Compendia-<version>-x64.zip  in the repo root (or -OutputDir)

param(
    [string]$BuildDir  = "build",
    [string]$StageDir  = "zip_stage",
    [string]$OutputDir = "."
)

$ErrorActionPreference = "Stop"
$RepoRoot = $PSScriptRoot

# --- Read version from CMakeLists.txt ---

$cmake = Get-Content (Join-Path $RepoRoot "CMakeLists.txt") -Raw
if ($cmake -match 'project\s*\(\s*compendia\s+VERSION\s+(\d+\.\d+\.\d+)') {
    $version = $Matches[1]
} else {
    Write-Error "Could not parse version from CMakeLists.txt"
}
Write-Host "Version: $version"

# --- Validate inputs ---

$buildPath  = Join-Path $RepoRoot $BuildDir
$stagePath  = Join-Path $RepoRoot $StageDir
$outputPath = Join-Path $RepoRoot $OutputDir

if (-not (Test-Path $buildPath)) {
    Write-Error "Build directory not found: $buildPath`nRun a Release build in Qt Creator first."
}

# Warn if model files are missing (face recognition will not work)
$modelsOk = (Test-Path (Join-Path $RepoRoot "models\shape_predictor_5_face_landmarks.dat")) -and
            (Test-Path (Join-Path $RepoRoot "models\dlib_face_recognition_resnet_model_v1.dat"))
if (-not $modelsOk) {
    Write-Warning "Model .dat files not found in models/. Face recognition will not work in this package."
    Write-Warning "To download: cmake -P cmake/DownloadModels.cmake"
}

# --- Stage the app via cmake --install ---

Write-Host ""
Write-Host "Staging app files..."
if (Test-Path $stagePath) {
    Write-Host "Removing old staging directory..."
    Remove-Item $stagePath -Recurse -Force
}

Write-Host "Running cmake --install..."
& cmake --install $buildPath --config Release --prefix $stagePath
if ($LASTEXITCODE -ne 0) { Write-Error "cmake --install failed." }

# --- Create zip archive ---

Write-Host ""
Write-Host "Creating zip archive..."
$zipFile = Join-Path $outputPath "Compendia-$version-x64.zip"
if (Test-Path $zipFile) { Remove-Item $zipFile -Force }

Compress-Archive -Path (Join-Path $stagePath "*") -DestinationPath $zipFile
Write-Host "Output: $zipFile"

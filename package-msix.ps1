# package-msix.ps1
# Builds a self-contained Compendia MSIX package for Microsoft Store submission.
#
# Usage (from repo root, after a Release build in Qt Creator):
#   .\package-msix.ps1 -BuildDir build
#
# Prerequisites:
#   - CMake Release build completed in Qt Creator (or via cmake --build)
#   - Model .dat files present in models/
#     (if missing: cmake -P cmake/DownloadModels.cmake)
#   - AppxManifest.xml filled in: replace REPLACE_WITH_PACKAGE_IDENTITY_NAME
#     with the Package/Identity/Name from Partner Center → App identity
#   - Inkscape installed (auto-detected at default path or in PATH)
#   - ImageMagick installed, magick.exe in PATH
#   - Windows SDK installed (makeappx.exe auto-detected)
#
# Output:
#   Compendia-<version>-x64.msix  in the repo root (or -OutputDir)

param(
    [string]$BuildDir  = "build",
    [string]$StageDir  = "msix_stage",
    [string]$OutputDir = "."
)

$ErrorActionPreference = "Stop"
$RepoRoot = $PSScriptRoot

# ── Helpers ───────────────────────────────────────────────────────────────────

function Find-Inkscape {
    $candidates = @(
        "C:\Program Files\Inkscape\bin\inkscape.exe",
        "C:\Program Files (x86)\Inkscape\bin\inkscape.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    $found = Get-Command inkscape -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    return $null
}

function Find-Magick {
    $found = Get-Command magick -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    return $null
}

function Find-MakeAppx {
    $sdkBase = "C:\Program Files (x86)\Windows Kits\10\bin"
    if (Test-Path $sdkBase) {
        $versions = Get-ChildItem $sdkBase -Directory |
            Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
            Sort-Object Name -Descending
        foreach ($v in $versions) {
            $candidate = Join-Path $v.FullName "x64\makeappx.exe"
            if (Test-Path $candidate) { return $candidate }
        }
    }
    $found = Get-Command makeappx -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    return $null
}

function Export-SvgToPng {
    param([string]$Inkscape, [string]$Svg, [int]$Width, [int]$Height, [string]$Out)
    & $Inkscape -w $Width -h $Height -o $Out $Svg 2>$null
    if (-not (Test-Path $Out)) { throw "Inkscape failed to produce: $Out" }
}

# Export SVG into a PNG centered on a solid-color canvas of the given dimensions.
# Used for wide tile and splash screen where the SVG is roughly square.
function Export-SvgComposited {
    param(
        [string]$Magick,
        [string]$Svg,
        [int]$CanvasW, [int]$CanvasH,
        [int]$IconSize,
        [string]$BgColor,
        [string]$Out
    )
    $tmpIcon = [System.IO.Path]::GetTempFileName() -replace '\.tmp$', '.png'
    try {
        # Render the SVG at IconSize x IconSize via ImageMagick (which uses Inkscape/rsvg internally if available)
        & $Magick -background none -size "${IconSize}x${IconSize}" $Svg -resize "${IconSize}x${IconSize}" $tmpIcon
        if (-not (Test-Path $tmpIcon)) { throw "magick failed to rasterize SVG for composited image" }
        # Composite centered on solid background
        & $Magick -size "${CanvasW}x${CanvasH}" xc:"$BgColor" $tmpIcon `
            -gravity Center -composite $Out
        if (-not (Test-Path $Out)) { throw "magick failed to produce composited image: $Out" }
    } finally {
        if (Test-Path $tmpIcon) { Remove-Item $tmpIcon -ErrorAction SilentlyContinue }
    }
}

# ── Read version from CMakeLists.txt ─────────────────────────────────────────

$cmake = Get-Content (Join-Path $RepoRoot "CMakeLists.txt") -Raw
if ($cmake -match 'project\s*\(\s*compendia\s+VERSION\s+(\d+\.\d+\.\d+)') {
    $version3 = $Matches[1]
    $version4 = "$version3.0"
} else {
    Write-Error "Could not parse version from CMakeLists.txt"
}
Write-Host "Version: $version4"

# ── Validate inputs ───────────────────────────────────────────────────────────

$buildPath   = Join-Path $RepoRoot $BuildDir
$stagePath   = Join-Path $RepoRoot $StageDir
$outputPath  = Join-Path $RepoRoot $OutputDir
$manifestSrc = Join-Path $RepoRoot "msix\AppxManifest.xml"
$assetsSrc   = Join-Path $RepoRoot "msix\Assets"
$iconSvg     = Join-Path $RepoRoot "resources\compendia_icon.svg"

if (-not (Test-Path $buildPath)) {
    Write-Error "Build directory not found: $buildPath`nRun a Release build in Qt Creator first."
}

$manifestContent = Get-Content $manifestSrc -Raw
if ($manifestContent -match 'REPLACE_WITH_PACKAGE_IDENTITY_NAME') {
    Write-Error "AppxManifest.xml still contains REPLACE_WITH_PACKAGE_IDENTITY_NAME.`nReplace it with your Package/Identity/Name from Partner Center → App identity."
}

$inkscape = Find-Inkscape
if (-not $inkscape) { Write-Error "Inkscape not found. Install from https://inkscape.org or add to PATH." }
Write-Host "Inkscape: $inkscape"

$magick = Find-Magick
if (-not $magick) { Write-Error "ImageMagick (magick.exe) not found. Install from https://imagemagick.org or add to PATH." }
Write-Host "ImageMagick: $magick"

$makeappx = Find-MakeAppx
if (-not $makeappx) { Write-Error "makeappx.exe not found. Install the Windows SDK." }
Write-Host "makeappx: $makeappx"

# Warn if model files are missing (face recognition will not work)
$modelsOk = (Test-Path (Join-Path $RepoRoot "models\shape_predictor_5_face_landmarks.dat")) -and
            (Test-Path (Join-Path $RepoRoot "models\dlib_face_recognition_resnet_model_v1.dat"))
if (-not $modelsOk) {
    Write-Warning "Model .dat files not found in models/. Face recognition will not work in this package."
    Write-Warning "To download: cmake -P cmake/DownloadModels.cmake"
}

# ── Stage the app via cmake --install ─────────────────────────────────────────

Write-Host "`n── Staging app files ────────────────────────────────────────────"
if (Test-Path $stagePath) {
    Write-Host "Removing old staging directory..."
    Remove-Item $stagePath -Recurse -Force
}

Write-Host "Running cmake --install..."
& cmake --install $buildPath --config Release --prefix $stagePath
if ($LASTEXITCODE -ne 0) { Write-Error "cmake --install failed." }

# ── Generate MSIX visual assets ───────────────────────────────────────────────

Write-Host "`n── Generating visual assets ─────────────────────────────────────"
$assetsOut = Join-Path $stagePath "Assets"
New-Item -ItemType Directory -Force -Path $assetsOut | Out-Null

$bgColor = "#1a2035"   # dark navy: used for wide tile and splash background

# Store logo  (shown in search results and the Store page)
Export-SvgToPng $inkscape $iconSvg  50  50  (Join-Path $assetsOut "StoreLogo.png")
Export-SvgToPng $inkscape $iconSvg 100 100  (Join-Path $assetsOut "StoreLogo.scale-200.png")
Write-Host "  StoreLogo done"

# Square 44x44  (taskbar, title bar, app list)
Export-SvgToPng $inkscape $iconSvg  44  44  (Join-Path $assetsOut "Square44x44Logo.png")
Export-SvgToPng $inkscape $iconSvg  88  88  (Join-Path $assetsOut "Square44x44Logo.scale-200.png")
# Target-size variants for File Explorer / system contexts
foreach ($sz in @(16, 24, 32, 48, 256)) {
    Export-SvgToPng $inkscape $iconSvg $sz $sz `
        (Join-Path $assetsOut "Square44x44Logo.targetsize-$sz.png")
}
Write-Host "  Square44x44Logo done"

# Square 150x150  (medium Start tile)
Export-SvgToPng $inkscape $iconSvg 150 150  (Join-Path $assetsOut "Square150x150Logo.png")
Export-SvgToPng $inkscape $iconSvg 300 300  (Join-Path $assetsOut "Square150x150Logo.scale-200.png")
Write-Host "  Square150x150Logo done"

# Square 310x310  (large Start tile)
Export-SvgToPng $inkscape $iconSvg 310 310  (Join-Path $assetsOut "Square310x310Logo.png")
Export-SvgToPng $inkscape $iconSvg 620 620  (Join-Path $assetsOut "Square310x310Logo.scale-200.png")
Write-Host "  Square310x310Logo done"

# Wide 310x150  (wide Start tile — icon centered on colored background)
Export-SvgComposited $magick $iconSvg 310 150 120 $bgColor (Join-Path $assetsOut "Wide310x150Logo.png")
Export-SvgComposited $magick $iconSvg 620 300 240 $bgColor (Join-Path $assetsOut "Wide310x150Logo.scale-200.png")
Write-Host "  Wide310x150Logo done"

# Splash screen (shown while app is loading)
Export-SvgComposited $magick $iconSvg 620 300 200 $bgColor (Join-Path $assetsOut "SplashScreen.png")
Export-SvgComposited $magick $iconSvg 1240 600 400 $bgColor (Join-Path $assetsOut "SplashScreen.scale-200.png")
Write-Host "  SplashScreen done"

# ── Inject version into manifest and copy to staging dir ─────────────────────

Write-Host "`n── Preparing manifest ───────────────────────────────────────────"
$manifestOut = Join-Path $stagePath "AppxManifest.xml"

# Update the Version attribute to match the current build
$updatedManifest = $manifestContent -replace '(?<=Version=")[^"]+(?=")', $version4
$updatedManifest | Set-Content $manifestOut -Encoding UTF8
Write-Host "  Manifest written (version $version4)"

# ── Pack with makeappx ────────────────────────────────────────────────────────

Write-Host "`n── Packing MSIX ─────────────────────────────────────────────────"
$msixFile = Join-Path $outputPath "Compendia-$version3-x64.msix"
if (Test-Path $msixFile) { Remove-Item $msixFile -Force }

& $makeappx pack /d $stagePath /p $msixFile /nv
if ($LASTEXITCODE -ne 0) { Write-Error "makeappx pack failed." }

Write-Host "`n── Done ─────────────────────────────────────────────────────────"
Write-Host "Output: $msixFile"
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Test locally: Add-AppxPackage -Path '$msixFile'"
Write-Host "     (requires a self-signed cert — see Partner Center for test cert options,"
Write-Host "      or use MSIX Packaging Tool to sign with a trusted dev cert)"
Write-Host "  2. Submit to Partner Center: upload $msixFile as a new submission package."
Write-Host "     Microsoft signs the package during Store ingestion — no cert purchase needed."

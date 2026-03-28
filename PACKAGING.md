# Packaging

This document describes how to produce distribution packages of Compendia for each platform. See [VERSIONING.md](VERSIONING.md) for the overall release workflow (tagging, GitHub releases, version bumping).

---

## Windows — Self-Contained ZIP

The Windows package is a self-contained ZIP. Unzip and run `compendia.exe` with no additional installation.

### Prerequisites

- Qt 6 (MinGW 64-bit) installed via the Qt Online Installer
- CMake 3.16+
- vcpkg with `x64-windows` triplet, providing `libexif`, `libheif`, `libde265`, and `libx265`
- A Release build already compiled in Qt Creator

### Steps

#### 1. Configure with CMake (Release)

From the repository root:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
```

CMake auto-detects `windeployqt` from your Qt installation and the MinGW runtime DLLs. It also searches for the vcpkg install tree in these locations (in order):

1. `../vcpkg/installed/x64-windows` (sibling of the repo)
2. `%USERPROFILE%/vcpkg/installed/x64-windows`
3. `C:/vcpkg/installed/x64-windows`

If your vcpkg lives elsewhere, pass it explicitly:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DVCPKG_INSTALLED_DIR=C:/path/to/vcpkg/installed/x64-windows
```

#### 2. Build

Build in Qt Creator (Release configuration), or from the terminal:

```bash
cmake --build build --config Release
```

#### 3. Create the ZIP

```bash
cd build
cpack -G ZIP -C Release
```

This produces:

```
build/Compendia-<version>-Windows-x64.zip
```

### What the package contains

CPack runs `cmake --install` internally, which:

1. Copies `compendia.exe` to the archive root.
2. Runs `windeployqt --release --no-translations` to collect all required Qt DLLs and plugins.
3. Copies the three MinGW runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`).
4. Copies the vcpkg DLLs: `exif-12.dll`, `heif.dll`, `libde265.dll`, `libx265.dll`.
5. Bundles any dlib face-recognition model files from `models/` if they are present.

### Deployment

Distribute the ZIP as-is. The recipient unzips to any folder and runs `compendia.exe` directly — no installer or PATH changes needed.

### Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| CMake warning: `vcpkg installed dir not found` | vcpkg path not detected | Pass `-DVCPKG_INSTALLED_DIR=<path>` |
| CMake warning: `windeployqt not found` | Qt not in expected location | Pass `-DWINDEPLOYQT_EXECUTABLE=<path>` or add Qt `bin/` to `PATH` |
| Missing MinGW DLL warning | MinGW tools path changed | Set `-DMINGW_BIN_DIR=<path>` or verify Qt Tools directory |
| App crashes on launch | A DLL was not bundled | Run `compendia.exe` from a terminal to see the missing-DLL error message |

---

## Windows — MSIX (Microsoft Store)

The MSIX package is the format required for Microsoft Store distribution. It is self-contained — no dependencies, no separate model downloads.

### Prerequisites

- Everything from the ZIP prerequisites (Qt 6 MinGW, CMake, vcpkg)
- Model `.dat` files present in `models/` (see ZIP section — download with `cmake -P cmake/DownloadModels.cmake`)
- [Inkscape](https://inkscape.org) installed (default path or in PATH)
- [ImageMagick](https://imagemagick.org) installed (`magick.exe` in PATH)
- Windows SDK installed — provides `makeappx.exe` (auto-detected by the script)
- `msix/AppxManifest.xml`: the `REPLACE_WITH_PACKAGE_IDENTITY_NAME` placeholder replaced with the **Package/Identity/Name** from Partner Center → your app → App identity

### Steps

#### 1. Build in Qt Creator

Build with the **Release** configuration in Qt Creator.

#### 2. Run the packaging script

From the repository root (PowerShell):

```powershell
.\package-msix.ps1 -BuildDir build
```

This produces `Compendia-<version>-x64.msix` in the repo root. The script:

1. Runs `cmake --install` to stage `compendia.exe`, all Qt DLLs, MinGW runtimes, vcpkg DLLs, and model files.
2. Generates all required Store visual assets (tiles, logos, splash screen) from `resources/compendia_icon.svg`.
3. Injects the current version into `msix/AppxManifest.xml` and copies it into the staging directory.
4. Packs the staging directory into an MSIX using `makeappx.exe`.

#### 3. Submit to Partner Center

Upload `Compendia-<version>-x64.msix` as a new package in your Partner Center submission. Microsoft signs the package during Store ingestion — no code signing certificate purchase is required.

### Updating the version

The script reads the version automatically from `CMakeLists.txt`. The manifest `Version` field is always set to `MAJOR.MINOR.PATCH.0`. Just increment `project(compendia VERSION ...)` in `CMakeLists.txt` as normal.

### `broadFileSystemAccess` justification

The manifest declares `broadFileSystemAccess`. When submitting, Partner Center asks for a business justification. Use this text:

> Compendia is a file management application that reads and writes metadata alongside user-selected media files. It writes `.compendia_cache/` subdirectories and a tag library JSON file in the same folders as the user's photos and videos, which can be located anywhere on the user's drives. The app does not collect, transmit, or share any file data externally.

### Testing locally

To install and test the package on your own machine before submitting:

```powershell
# Create a self-signed test certificate (one-time setup)
$cert = New-SelfSignedCertificate -Subject "CN=1DFEABBD-CF47-4AC4-85C7-F2B014071012" `
    -CertStoreLocation Cert:\CurrentUser\My -Type CodeSigningCert
Export-PfxCertificate -Cert $cert -FilePath test-cert.pfx -Password (ConvertTo-SecureString "test" -AsPlainText -Force)

# Sign the MSIX
& "C:\Program Files (x86)\Windows Kits\10\bin\<SDK version>\x64\signtool.exe" sign `
    /fd SHA256 /p7ce DetachedSignedData /p test /f test-cert.pfx `
    Compendia-<version>-x64.msix

# Trust the test cert (run once as administrator)
Import-Certificate -FilePath test-cert.pfx -CertStoreLocation Cert:\LocalMachine\Root

# Install
Add-AppxPackage -Path Compendia-<version>-x64.msix
```

### Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `makeappx: publisher mismatch` | Identity.Publisher in manifest doesn't match cert CN | Ensure both use `CN=1DFEABBD-CF47-4AC4-85C7-F2B014071012` |
| Asset file missing error | Inkscape or ImageMagick failed silently | Run the asset commands manually; check Inkscape/magick are in PATH |
| Face recognition missing | Model files absent | `cmake -P cmake/DownloadModels.cmake` then re-run the script |
| Store certification fails on `broadFileSystemAccess` | Justification not provided | Add the justification text in the submission notes |

---

## Linux — AppImage (Self-Contained)

The preferred Linux distribution format is an AppImage — a single portable file that bundles Qt and all required libraries. It runs on any modern Linux distribution without installation or matching system Qt versions.

### Prerequisites

- Qt 6.10+ installed via the Qt Online Installer (used for compilation in Qt Creator)
- A Release build already compiled in Qt Creator
- `cmake` available in the terminal
- Internet access on first run (linuxdeploy tools are downloaded automatically)

### Steps

#### 1. Build in Qt Creator

Build the project with the **Release** configuration in Qt Creator.

#### 2. Build the AppImage

From the terminal, pointing at your Qt Creator Release build directory:

```bash
cmake --build /path/to/build --target appimage
```

On first run this downloads `linuxdeploy` and `linuxdeploy-plugin-qt` into `<build>/linuxdeploy/` (~30 MB combined), then builds the image. Subsequent runs skip the download.

This produces:

```
<build>/Compendia-<version>-x86_64.AppImage
```

e.g. `Compendia-0.1.1-x86_64.AppImage`

### What the package contains

linuxdeploy bundles:

- `compendia` binary
- All Qt 6 shared libraries and plugins required at runtime
- `libheif`, `libexif`, and their codec dependencies
- The `.desktop` file and SVG icon (for desktop integration when mounted)
- `LICENSE` and `licenses/dlib-LICENSE.txt`
- dlib face-recognition model files from `models/` if present at build time

### Deployment

Mark the AppImage executable and run it directly — no installation needed:

```bash
chmod +x Compendia-0.1.1-x86_64.AppImage
./Compendia-0.1.1-x86_64.AppImage
```

### Uploading to a GitHub Release

```bash
gh release upload v<version> Compendia-<version>-x86_64.AppImage
```

### Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Download fails during build | No internet / GitHub unreachable | Download tools manually into `<build>/linuxdeploy/` and make them executable |
| `FUSE` error on launch | FUSE not available (some containers/VMs) | Run with `--appimage-extract-and-run` flag, or extract with `--appimage-extract` |
| Wrong Qt bundled | Multiple Qt installs; wrong qmake found | Check CMake output for `qmake (for AppImage):` line; pass correct Qt to CMake |
| App crashes on launch | Missing Qt plugin | Run from terminal to see the error; re-check qmake path |

---

## Linux — TGZ Archive (requires system Qt)

The TGZ is a lighter-weight archive produced by CPack. It contains the binary, `.desktop` file, and icon but **does not bundle Qt or system libraries**. Use this only when the target system is known to have Qt 6.10+ installed (e.g. developer machines). For general distribution, use the AppImage above.

### Prerequisites

- Qt 6.10+ installed on the target system
- A Release build already compiled in Qt Creator
- `cmake` available in the terminal

### Prerequisites

- Qt 6 installed on the build machine (used for compilation in Qt Creator)
- A Release build already compiled in Qt Creator
- CMake 3.16+ available in the terminal

### Steps

#### 1. Build in Qt Creator

Open the project in Qt Creator and build with the **Release** configuration. The build directory is wherever Qt Creator places it (typically a `build-compendia-...-Release` folder alongside the repo, or inside it).

#### 2. Configure with CMake (if not already done)

If Qt Creator has already run CMake for the Release build, this step is done. If you need to run it manually from the terminal:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
```

#### 3. Create the TGZ

From inside the build directory:

```bash
cd /path/to/build
cpack -G TGZ
```

This produces:

```
Compendia-<version>-Linux-x86_64.tar.gz
```

e.g. `Compendia-0.1.1-Linux-x86_64.tar.gz`

### What the package contains

The archive has a standard FHS layout:

```
usr/bin/compendia
usr/share/applications/compendia.desktop
usr/share/icons/hicolor/scalable/apps/compendia.svg
models/   (dlib .dat files, if present at build time)
```

Qt and system libraries (libheif, libexif) are **not** included. The recipient's system must satisfy them.

### Deployment

The target system needs Qt 6.10 or later. The Qt packages shipped with most distros (e.g. Ubuntu 24.04 ships Qt 6.4) are too old — install Qt 6.10 via the Qt Online Installer.

Also required:
- `libheif` (`sudo apt install libheif1`)
- `libexif` (`sudo apt install libexif12`)

The user extracts the archive and runs `usr/bin/compendia` directly, or installs the contents under `/usr/` to integrate with the desktop environment (icon and `.desktop` file picked up automatically).

### Uploading to a GitHub Release

If you created the GitHub Release from Windows (e.g. with the Windows ZIP already attached), add the Linux archive to the same release:

```bash
gh release upload v<version> Compendia-<version>-Linux-x86_64.tar.gz
```

Run from the build directory, or provide the full path to the file.

### Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| App fails to start: Qt libraries not found | Qt 6 not installed on target | Install Qt 6 packages (see Deployment above) |
| HEIF images not loading | libheif not installed | `sudo apt install libheif1` |
| `cpack` not found | CMake not on PATH | Ensure CMake 3.16+ is installed and in PATH |

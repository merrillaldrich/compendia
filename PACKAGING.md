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

## Linux — TGZ Archive

The Linux package is a `.tar.gz` archive produced by CPack. It contains the binary, `.desktop` file, and icon — but **does not bundle Qt or system libraries**. The target system must have Qt 6 and `libheif` installed.

> **Note:** For a fully self-contained portable package (no Qt dependency on the target), an AppImage built with `linuxdeploy` would be the right tool. That is not yet wired into the CMake build; the TGZ approach below is the current workflow.

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

The target system needs:
- Qt 6 Widgets, Multimedia, Concurrent, Svg (`sudo apt install libqt6widgets6 libqt6multimediawidgets6 libqt6svg6` or equivalent)
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

# Packaging — Windows ZIP Bundle

This document describes how to produce the self-contained Windows ZIP distribution of Compendia. Unzipping the archive and running `compendia.exe` requires no additional installation.

## Prerequisites

- Qt 6 (MinGW 64-bit) installed via the Qt Online Installer
- CMake 3.16+
- vcpkg with `x64-windows` triplet, providing `libexif`, `libheif`, `libde265`, and `libx265`
- A Release build already compiled (see [README.md](README.md))

## Steps

### 1. Configure with CMake (Release)

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

### 2. Build

```bash
cmake --build build --config Release
```

### 3. Create the ZIP

```bash
cd build
cpack -G ZIP -C Release
```

This produces:

```
build/Compendia-<version>-Windows-x64.zip
```

e.g. `Compendia-0.1.0-Windows-x64.zip`

## What the package contains

CPack runs `cmake --install` internally, which:

1. Copies `compendia.exe` to the archive root.
2. Runs `windeployqt --release --no-translations` to collect all required Qt DLLs and plugins.
3. Copies the three MinGW runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`).
4. Copies the vcpkg DLLs: `exif-12.dll`, `heif.dll`, `libde265.dll`, `libx265.dll`.
5. Bundles any dlib face-recognition model files from `models/` if they are present.

## Deployment

Distribute the ZIP as-is. The recipient:

1. Unzips to any folder.
2. Runs `compendia.exe` directly — no installer or PATH changes needed.

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| CMake warning: `vcpkg installed dir not found` | vcpkg path not detected | Pass `-DVCPKG_INSTALLED_DIR=<path>` |
| CMake warning: `windeployqt not found` | Qt not in expected location | Pass `-DWINDEPLOYQT_EXECUTABLE=<path>` or add Qt `bin/` to `PATH` |
| Missing MinGW DLL warning | MinGW tools path changed | Set `-DMINGW_BIN_DIR=<path>` or verify Qt Tools directory |
| App crashes on launch | A DLL was not bundled | Run `compendia.exe` from a terminal to see the missing-DLL error message |

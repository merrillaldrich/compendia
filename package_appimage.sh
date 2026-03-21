#!/usr/bin/env bash
# package_appimage.sh — Build and package Compendia as a Linux AppImage.
#
# Usage:
#   ./package_appimage.sh
#
# Environment overrides:
#   QT_DIR        Path to Qt installation prefix  (default: /home/maldrich/Qt/6.10.1/gcc_64)
#   BUILD_TYPE    CMake build type                 (default: Release)
#
# Prerequisites (one-time apt installs):
#   sudo apt install librsvg2-bin
#
# The script downloads its own packaging tools (linuxdeploy, appimagetool) into
# packaging/tools/ the first time it runs — internet access is required.
# Output: Compendia-<version>-x86_64.AppImage in the repo root.

set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────

QT_DIR="${QT_DIR:-/home/maldrich/Qt/6.10.1/gcc_64}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-appimage"
APPDIR="$SCRIPT_DIR/AppDir"
TOOLS_DIR="$SCRIPT_DIR/packaging/tools"

# Tell AppImage tools to extract-and-run without FUSE (needed on Ubuntu 22.04+
# where libfuse2 may not be installed).
export APPIMAGE_EXTRACT_AND_RUN=1

# ── Tools ─────────────────────────────────────────────────────────────────────

LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="$TOOLS_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="$TOOLS_DIR/appimagetool-x86_64.AppImage"

download_if_missing() {
    local url="$1" dest="$2"
    if [[ ! -f "$dest" ]]; then
        echo "==> Downloading $(basename "$dest") ..."
        curl -fsSL --progress-bar -o "$dest" "$url"
        chmod +x "$dest"
    fi
}

mkdir -p "$TOOLS_DIR"
download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "$LINUXDEPLOY"
download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "$LINUXDEPLOY_QT"
download_if_missing \
    "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" \
    "$APPIMAGETOOL"

# linuxdeploy discovers its Qt plugin by scanning PATH for
# "linuxdeploy-plugin-qt-x86_64.AppImage".
export PATH="$TOOLS_DIR:$PATH"

# ── Build ─────────────────────────────────────────────────────────────────────

echo "==> Configuring ($BUILD_TYPE) ..."
cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "==> Building ..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

# ── Assemble AppDir ───────────────────────────────────────────────────────────

echo "==> Assembling AppDir ..."
rm -rf "$APPDIR"
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# ── Deploy Qt and shared libraries ───────────────────────────────────────────

echo "==> Deploying Qt libraries (linuxdeploy + Qt plugin) ..."
export QMAKE="$QT_DIR/bin/qmake"
# ldd (used by linuxdeploy) must be able to resolve Qt's shared libraries.
export LD_LIBRARY_PATH="$QT_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Pre-generate a 256x256 PNG icon.  The file must be named "compendia.png"
# because linuxdeploy uses the source filename as the destination filename.
# Use a temp directory outside AppDir so linuxdeploy performs a real copy.
ICON_TMPDIR="$(mktemp -d)"
ICON_PNG="$ICON_TMPDIR/compendia.png"
rsvg-convert -w 256 -h 256 -o "$ICON_PNG" "$SCRIPT_DIR/resources/compendia_icon.svg"
trap 'rm -rf "$ICON_TMPDIR"' EXIT

# Run linuxdeploy to:
#  - copy all shared-library dependencies of the executable
#  - run linuxdeploy-plugin-qt to copy Qt plugins and write qt.conf
#
# linuxdeploy has a bug where it fails to find the icon when deploying AppDir
# root files (even though it successfully copies the icon into the hicolor
# tree). We ignore the exit code and complete the AppDir root manually below.
set +e
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/compendia" \
    --desktop-file "$SCRIPT_DIR/compendia.desktop" \
    --icon-file "$ICON_PNG" \
    --plugin qt
LINUXDEPLOY_EXIT=$?
set -e

# linuxdeploy exits 1 only for the icon-in-AppDir-root step; everything else
# (lib deployment, Qt plugin) runs first and succeeds.  Verify the critical
# outputs are present before continuing.
if [[ ! -f "$APPDIR/usr/bin/compendia" ]]; then
    echo "ERROR: linuxdeploy failed to deploy the binary — aborting."
    exit 1
fi
if [[ ! -d "$APPDIR/usr/plugins/platforms" ]]; then
    echo "ERROR: linuxdeploy Qt plugin failed to deploy platform plugins — aborting."
    exit 1
fi
if [[ $LINUXDEPLOY_EXIT -ne 0 ]]; then
    echo "==> linuxdeploy exited $LINUXDEPLOY_EXIT (known icon-root bug) — completing AppDir root manually."
fi

# ── Complete AppDir root manually ────────────────────────────────────────────
#
# appimagetool requires three files at the AppDir root:
#   AppRun          — entry-point script
#   compendia.desktop — desktop file (symlink or copy)
#   compendia.png    — application icon (symlink or copy)
#
# linuxdeploy (Qt 6) skips AppRun creation and fails before placing the icon,
# so we create all three here.

APPRUN="$APPDIR/AppRun"

# Create AppRun if linuxdeploy didn't.
if [[ ! -f "$APPRUN" ]]; then
    echo "==> Creating AppRun ..."
    cat > "$APPRUN" << 'APPRUN_EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
export APPDIR="${HERE}"

# Prefer bundled shared libraries over system ones.
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"

# libheif decoder plugins (libde265, aom) — needed for HEIC support.
export LIBHEIF_PLUGINS_PATH="${APPDIR}/usr/lib/libheif/"

exec "${APPDIR}/usr/bin/compendia" "$@"
APPRUN_EOF
    chmod +x "$APPRUN"
fi

# Desktop file symlink at AppDir root.
if [[ ! -e "$APPDIR/compendia.desktop" ]]; then
    ln -sf "usr/share/applications/compendia.desktop" "$APPDIR/compendia.desktop"
fi

# Icon at AppDir root (PNG preferred; fall back to SVG).
if [[ ! -e "$APPDIR/compendia.png" ]]; then
    ICON_IN_APPDIR="$APPDIR/usr/share/icons/hicolor/256x256/apps/compendia.png"
    if [[ -f "$ICON_IN_APPDIR" ]]; then
        ln -sf "usr/share/icons/hicolor/256x256/apps/compendia.png" "$APPDIR/compendia.png"
    else
        ln -sf "usr/share/icons/hicolor/scalable/apps/compendia.svg" "$APPDIR/compendia.svg"
    fi
fi

# ── Patch AppRun for LIBHEIF_PLUGINS_PATH (if linuxdeploy created AppRun) ────
#
# If linuxdeploy did create an AppRun (future version fix), insert the
# LIBHEIF_PLUGINS_PATH export so HEIC decoding still works.
if grep -q 'LIBHEIF_PLUGINS_PATH' "$APPRUN" 2>/dev/null; then
    : # already set (our own AppRun or already patched)
elif grep -q '^exec ' "$APPRUN" 2>/dev/null; then
    echo "==> Patching linuxdeploy AppRun for LIBHEIF_PLUGINS_PATH ..."
    sed -i '/^exec /i export LIBHEIF_PLUGINS_PATH="${APPDIR}/usr/lib/libheif/"' "$APPRUN"
fi

# ── Bundle libheif decoder plugins ───────────────────────────────────────────
#
# libheif loads decoder plugins (libde265, aom) at runtime from a configurable
# path.  The system plugins live in /usr/lib/x86_64-linux-gnu/libheif/; we
# bundle them inside the AppDir so HEIC decoding works on other machines.

HEIF_SYS_PLUGINS="/usr/lib/x86_64-linux-gnu/libheif/plugins/"
HEIF_APP_PLUGINS="$APPDIR/usr/lib/libheif/"

if [[ -d "$HEIF_SYS_PLUGINS" ]] && compgen -G "$HEIF_SYS_PLUGINS*.so" > /dev/null 2>&1; then
    echo "==> Bundling libheif decoder plugins ..."
    mkdir -p "$HEIF_APP_PLUGINS"
    cp "$HEIF_SYS_PLUGINS"*.so "$HEIF_APP_PLUGINS"

    # Copy any shared-library dependencies that the decoder plugins themselves
    # need and that aren't already bundled.
    for plugin in "$HEIF_APP_PLUGINS"*.so; do
        [[ -f "$plugin" ]] || continue
        while IFS= read -r dep; do
            dep_name="$(basename "$dep")"
            if [[ -f "$dep" && ! -f "$APPDIR/usr/lib/$dep_name" ]]; then
                cp "$dep" "$APPDIR/usr/lib/"
            fi
        done < <(ldd "$plugin" 2>/dev/null | awk '/=> \// { print $3 }')
    done
else
    echo "Warning: libheif system plugins not found at $HEIF_SYS_PLUGINS"
    echo "         HEIC decoding will be unavailable in the AppImage."
fi

# ── Determine version ─────────────────────────────────────────────────────────

VERSION="$(cmake -L -N "$BUILD_DIR" 2>/dev/null \
    | grep '^CMAKE_PROJECT_VERSION:' \
    | cut -d= -f2 \
    || echo "0.1.0")"

OUTPUT="$SCRIPT_DIR/Compendia-${VERSION}-x86_64.AppImage"

# ── Package ───────────────────────────────────────────────────────────────────

echo "==> Creating AppImage ..."
export ARCH=x86_64
"$APPIMAGETOOL" "$APPDIR" "$OUTPUT"

echo ""
echo "==> Done: $(basename "$OUTPUT")"

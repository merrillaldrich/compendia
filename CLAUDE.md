# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About

Compendia is a Qt/C++17 desktop application for managing and tagging media files (images, HEIF/HEIC). Users select a root folder, browse files, create tag families and tags, and apply them to files. Tag data is persisted as JSON alongside media files.

## Build

The project uses CMake and is developed in Qt Creator. Third-party libraries (libexif, libheif, dlib) are expected pre-built in `3rdParty/lib/` and `3rdParty/src/`. The `3rdParty/` directory is excluded from git.

```bash
# Configure (from repo root)
cmake -B build -S .

# Build
cmake --build build

# Or open CMakeLists.txt in Qt Creator and build from there
```

**macOS:** Install dependencies via Homebrew before configuring: `brew install libexif libheif`. The CMake script auto-detects the Homebrew prefix for both Apple Silicon and Intel.

**Windows:** libexif and libheif are pre-built static libraries in `3rdParty/lib/`. Their DLLs come from vcpkg (`x64-windows` triplet). The CMake script auto-detects vcpkg; override with `-DVCPKG_INSTALLED_DIR=<path>` if needed.

**Face recognition models** (optional): Download `shape_predictor_5_face_landmarks.dat` and `dlib_face_recognition_resnet_model_v1.dat` from dlib.net and place them in `models/` next to the executable, or run `cmake -P cmake/DownloadModels.cmake`.

The project version is defined in `CMakeLists.txt` (`project(compendia VERSION …)`) and injected into `version.h` via `version.h.in` at configure time.

There is no test suite.

## Architecture

**`CompendiaCore`** is the central controller. It owns:
- `QStandardItemModel* tagged_files_` — the file list model; each item's `Qt::UserRole+1` data stores a `TaggedFile*` via `QVariant`
- `FilterProxyModel* tagged_files_proxy_` — wraps the model for filename/folder/tag/date/rating/isolation filtering
- `QSet<TagFamily*>* tag_families_` and `QSet<Tag*>* tags_` — the tag library

**`MainWindow`** holds a `CompendiaCore` instance and wires its own UI widgets to core operations. It does not own any data directly.

**`TaggedFile`** is the per-file data object (tags as `QSet<Tag*>`, EXIF as `QMap<QString, QString>`, perceptual hash, star rating, dirty flag). It is stored as the `Qt::UserRole+1` data on each `QStandardItem` in the model.

**Tag hierarchy:** `TagFamily` (has a color, owns conceptual grouping) → `Tag` (has a name and a `TagFamily*` pointer). Both live in heap-allocated `QSet`s managed by `CompendiaCore`.

**Scan + async thumbnail flow:** `FolderScanner` runs on a dedicated `QThread` and emits `onScanBatch` with `QList<ScanItem>` batches as it discovers files; `onScanFinished` starts `IconGenerator`. `IconGenerator` dispatches image files in parallel via a dedicated `QThreadPool` and video files via `FrameGrabber` (main-thread async state machine). Results arrive via `onIconReady`, are held in a mutex-protected queue, and flushed to the model by `uiFlushTimer_` on the GUI thread. Thumbnails are cached as PNGs in a `.compendia_cache` sub-folder next to each batch of media files.

**Viewport-aware icon loading:** After initial backfill, only icons for items visible in the list view's viewport are loaded from cache. `wantedIconPaths_` (guarded by `wantedMutex_`) tracks the desired paths; `scheduleIconLoad()` fires background reads; `dataChanged` is emitted when they complete so the delegate repaints.

**Metadata I/O:** `ExifParser` reads EXIF from standard formats via libexif and from HEIF/HEIC via libheif, returning a `QMap<QString, QString>`. GPS keys use camelCase names as produced by libexif's `exif_tag_get_name()` (e.g. "GPSLatitude", "GPSLatitudeRef"). Tag data is saved/loaded as JSON sidecar files (see `TaggedFile::TaggedFileJSON()` and `CompendiaCore::parseTagJson()`).

**Filtering:** `FilterProxyModel` subclasses `QSortFilterProxyModel`. It filters on filename text, folder path, active `Tag*` pointers (OR or AND mode), creation date, star rating, and an **isolation set** — a `QSet<TaggedFile*>` that, when non-empty, restricts visible files to only those in the set. The two isolation entry-points in `CompendiaCore` are `setIsolationSet()` (selection-based) and `setFolderFilter()` (folder-path substring). `passesNonIsolationFilters()` is exposed so callers can preview filter behaviour without changing the isolation set.

**Undo/redo:** `UndoManager` uses full model snapshots (`ModelSnapshot`). Call `core->checkpoint("description")` before any mutation; `undo()` and `redo()` call `CompendiaCore::restoreSnapshot()`. The undo stack is capped at 20 entries. EXIF, pHash, file paths, and filter state are excluded from snapshots.

**UI layout pattern:** Tags and tag families use `FlowLayout` (a custom `QLayout` that wraps widgets like inline text) inside scrollable containers (`TagAssignmentContainer`, `TagContainer`, `TagFamilyWidget`).

**Drag-drop tagging:** `TaggableListView` subclasses `QListView` and accepts tag drops to apply tags to the file(s) under the cursor.

**Geography display:** `Geo` (static utility class) parses GPS EXIF keys into decimal degrees, performs OSM tile math, and initiates async Nominatim reverse-geocode requests. `MapTileCache` (singleton) owns the `QNetworkAccessManager` and fetches/caches up to 200 OSM tile pixmaps. `MapWidget` is a custom `QWidget` that renders tile layers with a red location marker; in interactive mode it supports drag-to-pan and scroll-to-zoom. `MapDialog` wraps a large interactive `MapWidget`. A small non-interactive `MapWidget` (`mapOverlay_`) is parented directly to `PreviewContainer` and positioned absolutely in the lower-right corner — same pattern as the nav arrow buttons.

**Compile-time constants** live in `constants.h` under the `Compendia` namespace. Add new project-wide magic values there rather than as local literals.

## Documentation

The user documentation site is hosted at https://compendia.gitbook.io/compendia via GitBook. The source files for the documentation live in a separate branch of this repository. Screenshots for the documentation are created and stored in Figma.

## Key dependencies

| Library | Role | Linked via |
|---|---|---|
| Qt 6 (Widgets, Core, Concurrent, Multimedia, MultimediaWidgets, Svg, Network) | UI, async, HTTP | `find_package` |
| dlib | Face detection (`FaceRecognizer`) | `add_subdirectory(3rdParty/src/dlib)` |
| libexif | EXIF extraction for standard images | Static lib; Homebrew on macOS, `3rdParty/lib/` on Windows |
| libheif | HEIF/HEIC support | Static lib; Homebrew on macOS, `3rdParty/lib/` on Windows |

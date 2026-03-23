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

# Or open compendia.pro / CMakeLists.txt in Qt Creator and build from there
```

There is no test suite.

## Architecture

**`CompendiaCore`** is the central controller. It owns:
- `QStandardItemModel* tagged_files_` — the file list model; each item's data role stores a `TaggedFile*` via `QVariant`
- `FilterProxyModel* tagged_files_proxy_` — wraps the model for filename/folder/tag filtering
- `QSet<TagFamily*>* tag_families_` and `QSet<Tag*>* tags_` — the tag library

**`MainWindow`** holds a `CompendiaCore` instance and wires its own UI widgets to core operations. It does not own any data directly.

**`TaggedFile`** is the per-file data object (tags as `QSet<Tag*>`, EXIF as `QMap<QString, QString>`, dirty flag). It is stored as the `Qt::UserRole` data on each `QStandardItem` in the model.

**Tag hierarchy:** `TagFamily` (has a color, owns conceptual grouping) → `Tag` (has a name and a `TagFamily*` pointer). Both live in heap-allocated `QSet`s managed by `CompendiaCore`.

**Asynchronous work:** `IconGenerator` runs background thumbnail generation using `QtConcurrent`. Results land in a mutex-protected `QVector<std::tuple<...>> results_` buffer inside `CompendiaCore`, flushed to the model by a `QTimer` (`uiFlushTimer_`).

**Metadata I/O:** `ExifParser` reads EXIF from standard formats via libexif and from HEIF/HEIC via libheif, returning a `QMap<QString, QString>`. Tag data is saved/loaded as JSON (see `TaggedFile::TaggedFileJSON()` and `CompendiaCore::parseTagJson()`).

**Filtering:** `FilterProxyModel` subclasses `QSortFilterProxyModel` and filters on filename text, folder path, and a set of active `Tag*` pointers.

**UI layout pattern:** Tags and tag families use `FlowLayout` (a custom `QLayout` that wraps widgets like inline text) inside scrollable containers (`TagAssignmentContainer`, `TagContainer`, `TagFamilyWidget`).

**Drag-drop tagging:** `TaggableListView` subclasses `QListView` and accepts tag drops to apply tags to the file(s) under the cursor.

## Documentation

The user documentation site is hosted at https://compendia.gitbook.io/compendia via GitBook. The source files for the documentation live in a separate branch of this repository. Screenshots for the documentation are created and stored in Figma.

## Key dependencies

| Library | Role | Linked via |
|---|---|---|
| Qt 6 (Widgets, Core, Concurrent, Multimedia, MultimediaWidgets) | UI and async | `find_package` |
| dlib | Face detection (`FaceRecognizer`) | `add_subdirectory(3rdParty/src/dlib)` |
| libexif | EXIF extraction for standard images | Static `.lib` in `3rdParty/lib/` |
| libheif | HEIF/HEIC support | Static `.lib` in `3rdParty/lib/` |

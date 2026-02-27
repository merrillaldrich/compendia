# Luminism

A media asset manager for vacationers, photographers, hobbyists, archivists, 
luminism's mission is to make our vast collections of photos and videos fun 
again! Browse large libraries, organise your photos and videos with a fully 
customizable set of labels. Interactively search and filter your collection
by tags, folder, filename, or capture date — all without touching
the files themselves.

---

## Features

### Browsing & Preview
- Point Luminism at any folder; it recursively discovers all images and videos,
  presenting them with high quality previews.
- Handles a LOT of images. luminism has been tested with libraries up to 100,000 
  files, the reality of our current cellphone camera lives!
- Click any thumbnail to open a full-size preview pane with smooth pinch/scroll
  zoom.

### Tagging
- luminism's main organizing principle is easy and intuitive tagging.
- Apply tags to one or 100 or 1000+ selected files by dragging a tag onto the file, 
  selection of files, or use the tag assignment panel below the preview for mass
  assignment to large numbers of files.
- Tags are organised into **Tag Families**, each with a unique colour, for
  clear visual grouping.
- Tags and their bounding rectangles (e.g. for face regions) are persisted as
  **sidecar files** written alongside the originals — your directory
  structure and original files are never modified.

### Filtering

Search thousands of files with ease

- **Filename** — substring match, case-insensitive.
- **Folder path** — substring match with shell-style inline autocomplete; Tab
  or Enter accepts the suggestion, Backspace dismisses it cleanly.
- **Tags** — filter by one or more tags in **Any (OR)** or **All (AND)** mode.
- **Capture date** — type a date or use the calendar popup; uses EXIF capture
  date when available, falls back to filesystem creation date. Leaving the field
  blank shows all files.

### EXIF Metadata

Get all the details from your photos with automatic support for EXIF data

- EXIF is available from JPEG and from HEIF/HEIC (iPhone) files.
- Extracted data is cached as JSON next to each folder so subsequent loads are
  instant.
- Capture date, camera model, lens, exposure settings and all other standard
  EXIF fields are available for display and filtering.

### Face Detection
- The **Find Faces** action runs dlib's HOG-based frontal face detector over
  every selected image and stores each detected face as a tagged bounding
  rectangle in the sidecar file.
- Coming soon: fully offline/private face detection

---

## Architecture

Built with **Qt 6 / C++17**

| Component | Role |
|---|---|
| `LuminismCore` | Central controller; owns the file model, proxy model, tag library, and async workers |
| `MainWindow` | UI shell; wires widgets to core operations |
| `TaggedFile` | Per-file data object — path, timestamps, tag set, EXIF map, dirty flag |
| `FilterProxyModel` | `QSortFilterProxyModel` subclass; applies filename / folder / tag / date filters |
| `IconGenerator` | `QtConcurrent`-based background thumbnail generator |
| `ExifParser` | Static EXIF reader for JPEG (libexif) and HEIF/HEIC (libheif) with JSON cache |
| `FaceRecognizer` | dlib HOG face detector; returns normalised bounding rectangles |
| `TaggableListView` | `QListView` subclass that accepts tag drag-drops |
| `FlowLayout` | Custom `QLayout` that wraps tag chips like inline text |
| `DateFilterButton` | Custom widget — typed date entry with inline autocomplete + calendar popup |
| `FolderFilterLineEdit` | `QLineEdit` subclass with shell-style path autocomplete |
| `ZoomableGraphicsView` | Pinch/scroll-zoomable `QGraphicsView` for the full-size preview |
| `PreviewContainer` | Full-size image and video preview panel |

Tag hierarchy: **TagFamily** (colour, conceptual group) → **Tag** (name, pointer back to family).

---

## Building

### Prerequisites

- **Qt 6.x** — Widgets, Core, Concurrent, Multimedia, MultimediaWidgets
- **CMake 3.16+**
- **C++17** compiler (MSVC, GCC, or Clang)
- **dlib**, **libexif**, and **libheif** pre-built and placed under `3rdParty/`:

```
3rdParty/
  lib/
    exif.lib      # libexif static library
    heif.lib      # libheif static library
  src/
    libexif/      # libexif headers
    libheif/      # libheif headers
    dlib/         # dlib source tree
```

> The `3rdParty/` directory is excluded from the repository. Build or obtain
> these libraries separately and place them as shown above.

### Configure and build

```bash
# From the repository root
cmake -B build -S .
cmake --build build
```

Or open `CMakeLists.txt` directly in **Qt Creator** and build from there.

---

## Tag persistence format

Each folder that contains tagged files gets a sidecar JSON file alongside the
images. The JSON records tag assignments (including optional normalised face
bounding rectangles) and EXIF data so that tag information travels with the
files if you move the folder.

---

## License

GNU - See LICENSE file

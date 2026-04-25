# compendia

A media asset manager for vacationers, photographers, hobbyists, archivists, 
compendia's mission is to make our huge collections of photos and videos fun 
again! Browse large libraries, organise your photos and videos with a fully 
customizable set of labels. Interactively search and filter your collection
using tags and criteria you define and control.

---

## Features

### Cross-Platform
- compendia has been built and tested on Windows 11, macOS, and Linux (Ubuntu/Gnome).
  Files and metadata are 100% identical across platforms so you can seamlessly
  move between them if needed.

### Browsing & Preview
- Point compendia at any folder; it recursively discovers all images and videos,
  presenting them with high quality previews.
- Handles a LOT of images. compendia has been tested with libraries up to 60,000 
  files.
- Click any thumbnail to open a full-size preview pane with smooth pinch/scroll
  zoom.

### Tagging
- compendia's main organizing principle is easy and intuitive tagging.
- Apply tags to one or 100 or 1000+ selected files by dragging a tag onto the file, 
  selection of files, or use the tag assignment panel below the preview for mass
  assignment to large numbers of files.
- Tags are organised into groups called **Tag Families**, each with a unique colour, 
  for clear visual grouping.
- The tag library stays in a stable sorted order and can be re-sorted at any time
  via the **Edit → Sort Library** command.
- Tags can apply to a whole image or to a region you define, for example to identify
  people in images.
- Tags are persisted as **"sidecar files"** written alongside the original images.
  Your directory structure and original files are never modified.
- **Undo / Redo** — full snapshot-based undo (Ctrl+Z) and redo (Ctrl+Y) for all
  tag and tag-family changes.

### Filtering

Search thousands of files with ease

- **Tags** — groups of tags are the main organizing principle in compendia. Filter 
  even a huge library interactively. **Any (OR)** or **All (AND)** mode enables 
  more search options.
- **Filename** — substring match, case-insensitive.
- **Folder path** — substring match with shell-style inline autocomplete; Tab
  or Enter accepts the suggestion, Backspace dismisses it cleanly.
- **Capture date** — type a date or use the calendar popup; uses EXIF capture
  date when available, falls back to filesystem creation date. Leaving the field
  blank shows all files.
- **Star rating** — filter by rating using less-than, equal, or greater-than.
- **Untagged Images** — isolate files that have no tags applied, making it easy
  to work through an unorganized library.

### EXIF Metadata

Get all the details from your photos with automatic support for EXIF data

- EXIF is available from JPEG and from HEIF/HEIC (iPhone) files.
- Extracted data is cached as JSON next to each folder so subsequent loads are
  instant.
- Capture date, camera model, lens, exposure settings and all other standard
  EXIF fields are available for display and filtering.

### Star Ratings
- Assign a 1–5 star rating to any file from the preview panel, or apply a
  rating to all currently visible files at once from the file list panel.
- Ratings are persisted in the sidecar file alongside tag data.
- Filter the visible set by rating using less-than, equal, or greater-than
  comparison modes.

### Navigation & Isolation
- **Isolate Selection** — narrow the view to only the files you have selected,
  leaving all other filters active within that set.
- **Isolate Folder** — restrict the view to all files under the folder of the
  currently selected file.
- **Drill to Folder** — reload the file list with the selected file's folder as
  the new root, for a focused browse of a single album.
- **Drill Up** — step back up the folder tree. Like a "zoom out" for your 
  library

### Autos

Automated operations that run across the loaded library:

- **Grab Video Frames** — captures a representative still frame from every
  video file and uses it as the thumbnail. Frame metadata (duration, date) is
  extracted and displayed in the preview panel.
- **Find Similar Images** — groups near-duplicate images using a perceptual
  hash (pHash) comparison and isolates those groups so you can review and
  resolve duplicates.
- **Auto-Tag Year / Month** — reads each file's capture date and tags it with
  its year or month in a dedicated tag family, making date-based organisation
  one click away.

### Face Detection
- The **Find Faces** action runs dlib's HOG-based frontal face detector over
  every image and stores each detected face as a tagged bounding rectangle in
  the sidecar file.
- Detected face regions are displayed as labelled overlays on the preview and
  can be renamed, repositioned, or deleted interactively.
- **Remove Auto-Detected Faces** clears all auto-detected face tags from the
  library and every file in one step.

### Export
- **File → Export** copies every file currently visible in the file list to a
  folder of your choice — useful for collecting a filtered or tagged subset of
  your library.
- Warns before exporting into the open project tree (which would create
  duplicates inside the root folder).
- When files already exist at the destination, prompts to **Overwrite**,
  **Skip**, or **Cancel** before any copying begins.
- Only the original media files are copied; sidecar JSON and cache files are
  excluded.

---

## Architecture

Built with **Qt 6 / C++17**

| Component | Role |
|---|---|
| `CompendiaCore` | Central controller; owns the file model, proxy model, tag library, and async workers |
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
| `StarRatingWidget` | Interactive 1–5 star rating control used in the preview, file list, and filter bar |
| `PerceptualHasher` | Computes and compares pHash values for near-duplicate detection |
| `FrameGrabber` | Captures a representative still frame from video files using `QMediaPlayer` |

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

# compendia 0.1.2

## New Features

### macOS Support
compendia now runs on macOS! This is the first official macOS build. It uses native macOS look and feel so you'll have a familiar experience on your Mac.

### Undo
Undo! Snapshot-based undo history in the application means you can revert changes to any tags applied to images.

### Untagged Images Filter
A new **Untagged Images** button in the filter bar isolates files that have no tags applied. Useful for working through a larger library or a fresh batch of photos and making sure nothing gets missed.

### Tag Library Sorting
The tag library now maintains a stable sort during your session. Use **Edit → Sort Library** or the Sort button to explicitly alphabetize the tag library.

## Bug Fixes

- **Large image preview errors** - removed an internal Qt image allocation cap that caused preview failures on very large files.
- **Filter not updating after external file move** - the file list filter now refreshes correctly when files are moved outside the app.
- **Disabled menu commands** - menu items that require a loaded folder are now correctly disabled on startup.
- **Sort button blue tint on Windows** - fixed a rendering artefact caused by Windows QStyleSheetStyle alpha compositing. Struggling to do the same on macOS. Help.
- **macOS filesystem event handling** - fixed duplicate and stale model entries caused by macOS move events.

## Known Issues

- On macOS the required AI models were not included in the release binary

## Platform Notes

compendia is now available for Windows, macOS, and Linux. Packages:

- **Windows** - ZIP archive (includes MSVC runtime, no separate install needed)
- **Linux** - AppImage
- **macOS** - ZIP archive. A signed/notarized package is planned for a future release

In the plan is availability through both the Microsoft Store and Apple Store. Stay tuned!

## Building from Source

See [README](README.md) for prerequisites and build instructions.

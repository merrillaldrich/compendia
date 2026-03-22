---
description: How to load photos, create tags, and filter your media library
---

# compendia

<figure><img src=".gitbook/assets/Corner.png" alt="" width="188"><figcaption></figcaption></figure>

Welcome to compendia. Here are the basics to help you get started wrangling your photos and videos.

## Opening a Folder

Compendia works with a folder of images on your local machine. To get started:

1. Click **Open Folder** in the toolbar (or use **File → Open Folder**).
2. Navigate to a folder containing images or videos and click **Select Folder**.
3. Compendia scans the folder and displays thumbnails for all supported files in the main list.

Supported formats include JPEG, PNG, TIFF, HEIC/HEIF, and common video formats.

> **Tip:** Compendia saves tag data as small `.json` sidecar files alongside your media, so your tags travel with your files.

---

## The Interface

The window is divided into three main areas:

| Area | Purpose |
|---|---|
| **Tag Library** (top-left) | All tag families and tags you have created |
| **Tag Filters** (bottom-left) | Active filters that narrow the file list |
| **File List** (center) | Thumbnails of files matching the current filters |
| **Tag Assignment** (right) | Tags applied to the currently selected files |

---

## Creating Tags

Tags are organized into **Tag Families** — groups that share a theme or color. For example, you might have a family called *Subject* containing tags like *Portrait*, *Landscape*, and *Architecture*.

### Create a Tag Family

1. Click **Add Family** in the Tag Library area.
2. Enter a name and choose a color.
3. Click **OK**. The new family appears in the Tag Library.

### Create a Tag

1. Click the **+** button next to a tag family in the Tag Library.
2. Enter a tag name and click **OK**.
3. The tag appears inside its family in the library.

---

## Applying Tags to Files

### By dragging from the library

1. Select one or more files in the file list (click to select; **Ctrl+click** or **Shift+click** for multiple).
2. Drag a tag from the Tag Library and drop it onto the selected file(s) in the file list.

### By dragging into the Tag Assignment area

1. Select one or more files.
2. Drag a tag from the Tag Library and drop it into the **Tag Assignment** area on the right. The tag is applied to all currently visible (filtered) files.

### Removing a tag

Click the **×** button on any tag chip in the Tag Assignment area to remove that tag from the selected files.

---

## Filtering by Tag

To narrow the file list to only files that have a specific tag:

1. Drag a tag from the Tag Library and drop it into the **Tag Filters** area (bottom-left).
2. The file list updates immediately to show only files that have all the active filter tags.
3. To remove a filter, click the **×** on the tag chip in the Tag Filters area.

You can stack multiple filters — only files matching **all** active tags are shown.

---

## Searching by Filename

Type in the search box at the top of the file list to filter by filename. This works alongside tag filters — both are applied at the same time.

---

## Saving Your Work

Tag data is saved by clicking **Save** in the toolbar, or using **File → Save**. Each file's tags are stored in a `.json` sidecar file in the same folder as the image.

When you have unsaved changes, save before closing to avoid losing your work.

---

## Next Steps

Once you have tagged your library you can:

- Use multiple simultaneous tag filters to find exactly the files you need.
- Reopen the same folder later — Compendia reads the sidecar files and restores all your tags.
- Reorganise your tag families and rename tags at any time; existing assignments update automatically.

---
description: Getting Started
---

# compendia

<figure><img src=".gitbook/assets/Corner.png" alt="" width="188"><figcaption></figcaption></figure>

Welcome to compendia. Here are the basics to help you get started wrangling your photos and videos.

## Getting Started

### Opening a Folder

Compendia works with folders of images on your local machine. To get started:

1. Make sure you have your images locally. If you use a cloud service like Apple or Google, download your pictures.
2. Open the folder containing your pictures, either with the link in the center of the screen or the **Browse** button.
3. Navigate to a folder containing images or videos and click **Select Folder**.
4. Compendia will show a brief warning about how it creates files to tag your images.
5. It'll scan the folder and subfolders, and display thumbnails for all supported files.

Supported formats include JPEG, PNG, TIFF, HEIC/HEIF, and common video formats.

If you have a very large library of images, it may be best to start by exploring with a small set. Compendia can handle large sets of images, but it may be slower to try to work on your whole collection while you experiment and learn the application.

> **Tip:** Compendia saves tag data as small `.json` sidecar files alongside your media, so your tags travel with your files.

### The Interface

The application's main window is divided into these areas:

<table><thead><tr><th width="279.99993896484375">Area</th><th>Purpose</th></tr></thead><tbody><tr><td><strong>File List</strong> (center)</td><td>Thumbnails of files. When filters are applied, this list shows just the matching files.</td></tr><tr><td><strong>Tag Library</strong> (bottom-left)</td><td>All tag families and tags you have created.</td></tr><tr><td><strong>Tag Filters</strong> (top-left)</td><td>Active filters that narrow the file list.</td></tr><tr><td><strong>Tag Assignment</strong> (center-bottom)</td><td>Tags applied to the currently selected files.</td></tr><tr><td><strong>Previewer</strong> (right)</td><td>Examine and scroll through individual images at full size, with zoom and pan.</td></tr></tbody></table>

### Creating Tags

Tags are organized into **Tag Families**, which are groups that share a theme or color. For example, you might have a families called _People_, _Places_, _Year_, and _Events_. Inside those you'd place _Susan, Dad, Mom_, or _Paris, New York, Yosemite,_ and _2025, 2026_ etc.

#### Create a Tag Family

1. Click in the Tag Library area to add a family.
2. Enter a name for the family and press _Enter_.

#### Create a Tag

1. An empty tag will appear. If it doesn't or you lose it, just click anywhere in the body of the Tag Family box.
2. Enter a tag name and press _Enter_.

### Applying Tags to Files

#### By dragging from the library onto File(s)

* Drag and drop a tag onto an individual file to apply it to that file
* Select a set of files and drag a tag onto any selected file to apply it to all selected files

#### By dragging into the Tag Assignment area

The bottom area of the interface, below the file list, is the _Tag Assignment Area._ One of the most powerful ways to assign tags is to drag tags here - but note that a tag added in this way will be applied to _all_ the visible files.

The idea here is that you filter the images you want, by name or by folder or by other means, and then you can assign tags to larger batches of files, essential for organizing a large library.

### Removing a tag

Click the **×** button on any tag in the Tag Assignment area to remove that tag from **all** the visible files.

Click the **×** button on any tag in the Tag Library (bottom left) to remove it completely from all files in your project.

### Filtering by Tag

To narrow the file list to only files that have a specific tag:

1. Drag a tag from the Tag Library and drop it into the **Tag Filters** area (top-left).
2. The file list updates immediately to show only files that have all the active filter tags.
3. To remove a filter, click the **×** on the tag in the Tag Filters area. This removes it from the filter, but not from your files.

A powerful filtering technique is to "stack" multiple tags in the filter area. The filter can be set to use _and_ or _or_ logic, so you can zero in on only images that have sets of tags.&#x20;

Example: if you want pictures of "Denise" in "Paris" in "2005," you can put all three tags into the filter area and choose the ALL option. The file list will only show images that are tagged Denise _and_ Paris _and_ 2005.

On the other hand, if you want to see pictures from Paris _or_ New York, put both tags in the filter area but choose the option ANY of these tags.

### Searching by Name

Type in the search boxes at the top of the file list to filter by file or folder name. This works alongside tag filters, and both are applied at the same time. This is very handy if you have a large set of folders but some naming criteria that you can use to isolate some, like the last batch of pictures from your phone, or a date that appears in the folder name.

### Saving Your Work

Tag data is saved by clicking **Save** in the top area, or using **File → Save**. Each images's metadata and tags are stored in a `.json` sidecar file in the same folder as the image.

When you have unsaved changes, save before closing to avoid losing your work.

## Next Steps

Once you have tagged your library you can:

* Use multiple simultaneous tag filters to find exactly the files you need.
* Reopen the same folder later. Compendia reads the sidecar files and restores all your tags.
* Reorganise your tag families and rename tags at any time; existing assignments update automatically.

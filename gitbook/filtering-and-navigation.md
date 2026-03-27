---
description: >-
  How to use tag, name, folder, date, and ratings filters - separately and in
  combination
---

# Filtering and Navigation

## Overview

Compendia implements several types of filtering: by tags, by star rating, by file or folder name, and by capture date. All active filters work simultaneously, so you can combine them to pinpoint exactly the files you need.

## Tag Filters

The **Tag Filter** area is the upper-left panel in the main window. Drag and drop tags here to narrow the displayed files to only the ones with certain tags.

<figure><img src=".gitbook/assets/filteringandnav-dragdrop.png" alt=""><figcaption></figcaption></figure>

Active filters are shown as colored tags grouped by family, using the same visual style as the Tag Library and Tag Assignment area. Note under the file list we have a filtered view showin 4 out of a total of 28 files.

To remove a filter, just click the **×** button on the tag, being careful to do it in the filter area and not the library. The tag will disappear from the filter set and the files displayed will no longer be restricted to that tag.

## Multiple Tags

One of the more powerful ways to filter is to stack multiple tags in the filter.

When more than one tag filter is active, Compendia can apply them together, using either OR or AND logic.&#x20;

Example: if you have tags for years, you could place 2024 and 2025 into the filter area to see only photos taken in 2024 _or_ 2025 but not other years. Here you would set the filter preference just above the tags to "Show files with **ANY** of these tags." (The "or" filter.)

Alternatively, if you have tags for People and Events and Places, you could add "Meredith," "European Vacation," and "Germany" to show only pictures of Meredith, in Germany, during the European Vacation. Here set the filter preference to "Show files with **ALL** of these tags." (The "and" filter.)

There is no limit to the number of filters you can stack.

## Removing a Filter

To remove a tag filter, click the **×** button on the tag in the Tag Filter area. The file list immediately updates to reflect the remaining filters.

To clear all active filters at once, you can use **Filter > Clear All Filters**. This removes all tag filters, as well as any active star rating, folder or file name, and date filters. It does not affect the tags assigned to your files.

## Folder Navigation

When a folder is loaded, Compendia shows all files from that folder and its subfolders. The folder navigation panel lets you drill down into a specific subfolder to limit the file list to just those files.

Selecting a subfolder acts as an additional filter on top of any active tag filters.

## Combining Tag Filters with Folder Navigation

Tag filters and folder navigation work together. You can select a subfolder to limit the scope to that location, then add tag filters to narrow further within it. You can also set tag filters first and then navigate into a subfolder. Either way, the file list always reflects all active constraints at once.

## Advanced: Isolating Sets of Files

Compendia provides three methods for focusing on a specific subset of files. Each is suited to a different situation.

### Selection Isolation

Selection isolation temporarily filters the file list down to exactly the files you have selected. To use it, select the files you want to work with using click, **Ctrl+click**, or **Shift+click**, then activate selection isolation. The file list will show only those files until you clear the isolation.

This is useful when you need to apply a unique set of tags to a specific group of files that do not share a common folder or tag. For example, you might hand-pick 20 files from across your library and then tag them all without affecting anything else.

### Folder Isolation

Folder isolation filters the file list to the folder that a specific file belongs to, including its subdirectories. To use it, right-click a file and choose **Isolate Folder**. The file list will update to show only files in that folder and below.

This is a quick way to focus on a particular location in your folder tree without having to navigate to it manually. To clear folder isolation, use the clear folder isolation button or the corresponding menu command. If you also want to clear any other active filters at the same time, use **Filter > Clear All Filters**.

### Drill

Drill is designed for very large projects where keeping the entire library in memory is slow or impractical. Rather than just filtering the view, drill physically opens a subfolder as if it were its own project, unloading everything outside that location from memory. This frees up resources and makes working with that portion of the library faster.

Each folder in Compendia is self-contained with its own tag data, so you can drill into any subfolder and work on it independently. For example, in a project with 50,000 images across 600 folders, you might drill into a section covering one year or one location, work through it, and then move on to another section. To move back up through the folder tree, use the **Drill Up** button. This moves up one directory level at a time, up to the root folder where you originally opened the project.

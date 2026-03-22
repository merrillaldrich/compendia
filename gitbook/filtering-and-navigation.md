---
description: How to use tag filters and folder navigation to find your files
---

# Filtering and Navigation

## Overview

Filtering is how you narrow the file list down to just the files you want to see. Compendia supports several types of filtering: by tag, by star rating, by file or folder name, by date, and by folder. All active filters work simultaneously, so you can combine them to pinpoint exactly the files you need.

## The Tag Filter Area

The Tag Filter area is the upper-left panel in the main window. It shows all the tag filters that are currently active. When no filters are set, the file list shows all files in the loaded folder. As you add filters, the list narrows to show only files that match.

Active filters are shown as colored tags grouped by family, using the same visual style as the Tag Library and Tag Assignment area.

## Adding a Filter

To filter by a tag, drag it from the Tag Library and drop it into the Tag Filter area. The file list updates immediately to show only files that have that tag assigned.

You can add filters from different families in the same way. Just drag and drop each tag you want to filter by.

## Stacking Multiple Filters

When more than one tag filter is active, Compendia applies them together using AND logic: only files that have **all** of the active filter tags are shown. This lets you progressively narrow the list. For example, you could filter first by _Location: Paris_ and then by _Subject: Portrait_ to find portraits taken in Paris.

There is no limit to the number of filters you can stack.

## Removing a Filter

To remove a tag filter, click the **×** button on the tag in the Tag Filter area. The file list immediately updates to reflect the remaining filters.

To clear all active filters at once, use **Filter > Clear All Filters**. This removes all tag filters, as well as any active star rating, filename, and date filters. It does not affect the tags assigned to your files.

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

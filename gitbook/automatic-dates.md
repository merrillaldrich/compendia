---
description: How Compendia reads and uses date information from your files
---

# Automatic Dates

## Overview

Compendia automatically reads date and time information from your image files and makes it available for display and filtering. This means you can browse your library by when photos were taken without having to manually enter or manage any date information.

## How Dates Are Read

When you load a folder, Compendia reads the EXIF metadata embedded in each image file. The capture date and time are extracted automatically. This works for standard image formats such as JPEG and TIFF, as well as HEIC and HEIF files captured by modern smartphones and cameras.

The date shown in Compendia reflects when the photo was taken according to the camera, not when the file was created or last modified on your computer.

## Supported Date Fields

Compendia reads the following date-related EXIF fields when present:

* **Date Taken** (DateTimeOriginal) — the date and time the shutter was pressed
* **Date Digitized** (DateTimeDigitized) — the date the image was scanned or digitized, relevant for film scans
* **File Modification Date** — used as a fallback when EXIF date fields are absent

Where multiple fields are present, Compendia prioritises the original capture date.

## Using Dates for Organisation

The capture date is displayed in the preview panel when you open an image. You can also filter the file list by date range, making it easy to find photos from a specific trip, event, or time period. Date filtering works alongside tag filters and search fields, so you can combine all of them to narrow your results precisely.

## Automatic Date Tagging

Because Compendia knows the capture date of most images, it can automatically apply year and month tags to your files based on that date. This gives you a fast way to browse and filter your library by time period without any manual tagging.

You can choose to tag by year, by month, or both. When you run automatic date tagging, Compendia reads the capture date from each file and applies the corresponding tags. For example, a photo taken in March 2022 would receive a year tag of _2022_ and a month tag of _March_ (or _2022-03_, depending on your settings).

Once the tags are applied they behave like any other tag in your library. You can use them as filters, combine them with other tag filters, and remove them individually if needed. This makes browsing a large library by year or year-and-month combination quick and straightforward.

Files that do not have a readable capture date will be skipped during automatic date tagging.

## When Date Information Is Missing

Some files do not contain EXIF metadata, or the date fields may not have been set correctly by the camera. In these cases Compendia will fall back to the file modification date where possible. If no date information is available at all, the date fields will appear empty for that file.

If you find that dates are incorrect or missing, the most reliable fix is to use a dedicated EXIF editing tool to correct the metadata in the original files before loading them in Compendia.

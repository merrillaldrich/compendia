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

The capture date is displayed in the file list and the preview panel. You can also filter the file list by date range, making it easy to find photos from a specific trip, event, or time period. Date filtering works alongside tag filters and search fields, so you can combine all of them to narrow your results precisely.

## When Date Information Is Missing

Some files do not contain EXIF metadata, or the date fields may not have been set correctly by the camera. In these cases Compendia will fall back to the file modification date where possible. If no date information is available at all, the date fields will appear empty for that file.

If you find that dates are incorrect or missing, the most reliable fix is to use a dedicated EXIF editing tool to correct the metadata in the original files before loading them in Compendia.

---
description: Answers to common questions about Compendia
---

# FAQ

## Where are my tags stored?

Tag data is stored as `.json` sidecar files in the same folder as your images. This means your tags travel with your files if you move the folder, as long as the sidecar files move with them.

## Can I use Compendia with files on an external drive or network share?

Yes. Compendia works with any folder that your operating system can access, including external drives and network shares. Just use **File > Open Folder** and navigate to the location as you normally would. Keep in mind that performance may be slower when working over a network connection, particularly when generating thumbnails for large libraries.

## What image formats are supported?

Compendia supports JPEG, PNG, TIFF, and HEIC/HEIF files. Video files will appear in the file list as thumbnails, with playback support depending on the codecs available on your system.

## Can I rename or reorganise my tag families after I've already tagged files?

Yes. You can rename a tag family or individual tags at any time by left-clicking the name in the Tag Library. All existing tag assignments update automatically. No re-tagging is required.

## My tags disappeared after moving my folder. What happened?

Compendia stores tag data in `.json` sidecar files alongside your images. If you moved your images without also moving the sidecar files, or if the sidecar files were left behind in the original location, Compendia will not be able to find them. To recover your tags, move the sidecar files to the same folder as the images and reload the folder in Compendia.

## Does Compendia modify my original image files?

No. Compendia never modifies your original image files. All tag and metadata information is stored in separate sidecar files. Your images are read-only as far as Compendia is concerned.

## How do I back up my tags?

Because tags are stored as `.json` sidecar files alongside your images, they are included automatically in any backup that covers your image folders. There is no separate export step required. If you use a backup tool that syncs your photo folders, your tags will be backed up along with your images.

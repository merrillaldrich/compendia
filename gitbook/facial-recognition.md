---
description: Automatically identifying people in your photos
---

# Facial Recognition

## Overview

Compendia can scan your images to detect faces and help you identify the people in your library. Once faces are detected you can confirm or correct the identifications and apply person tags to your files, making it easy to find all photos of a specific person using the standard tag filtering system.

## How It Works

Compendia uses a local face detection and recognition model to analyse your images. All processing happens on your own machine and no image data is sent to any external service.

When you run facial recognition, Compendia scans the visible files for faces. It groups faces that appear to belong to the same person and presents them for your review. The more photos of a person that are in your library, the more accurately the model can identify them across different images.

## Suggested Workflow: Build a Known Set First

Facial recognition works best when Compendia has some known examples to work from before the automated scan runs. The recommended approach is to manually identify a handful of people first, which gives the model a reference set to match against.

**Step 1: Create a tag for each person you want to identify.**
In the Tag Library, create a tag family called _People_ if you do not already have one. Add a tag for each person, for example _People: Susan_ or _People: Dad_.

**Step 2: Find a clear photo of each person.**
Browse your library and open a photo where the person's face is clearly visible in the preview panel.

**Step 3: Drag the person's tag onto their face in the preview.**
Drag the tag from the Tag Library and drop it directly onto the person's face in the image preview. Compendia will place a face rectangle around the detected face.

**Step 4: Adjust the face rectangle.**
Resize and reposition the rectangle so it fits snugly around the person's face.

**Step 5: Repeat for each person.**
Work through a few example photos for each person you want to identify. You do not need to manually tag every photo, just enough to establish a clear reference for each individual.

**Step 6: Run Find Faces.**
Once you have built up a known set, run **Autos > Face Detection > Find Faces**. Compendia will use your manually tagged examples to help identify those same people across the rest of your library.

Taking a few minutes to build a known set before running the automation significantly improves the quality of the results.

## Running Facial Recognition

To run facial recognition, use **Autos > Face Detection > Find Faces**. You can choose to process the selected files, all currently visible files, or all files in your entire project. Depending on the number of files and the speed of your computer, this may take some time.

You can continue using the application while recognition runs in the background.

## Reviewing Results

After processing, Compendia presents the detected faces grouped by identity for you to review. For each group you can confirm that the faces belong to the same person, split incorrectly grouped faces into separate identities, or discard faces that were detected in error.

Taking time to review and correct the results improves the accuracy of future recognition runs on the same library.

## Tagging People

Once you are satisfied with the identified groups, you can assign a name to each identity. Compendia will create a tag for that person and apply it to all files where their face was detected. These tags work like any other tag in your library and can be used for filtering, searching, and organising your photos.

## Correcting Detected Faces

After running automatic face detection you will likely need to review and clean up the results. There are three common situations and a straightforward fix for each.

**The same person appears across many images under an auto-generated name.**
When Compendia detects a face it does not recognise, it creates a tag with a placeholder name. If you see a group of detections that all belong to the same person, just rename the auto-generated tag in the Tag Library by left-clicking it and typing the correct name. All images that have that tag will update immediately.

**A face has been tagged with the wrong person's name.**
People can look similar, and the model may occasionally assign the wrong identity. To correct a single face, drag the correct tag from the Tag Library and drop it onto the face rectangle in the image preview. The tag will be reassigned to the correct person.

**A face region does not correspond to a real person you want to track.**
Automatic detection sometimes picks up faces in crowds, strangers in the background, or false positives on textures like foliage or patterned fabric. There are two ways to handle these:

* To remove a single face region from one image, right-click the face rectangle in the preview and select **Delete**.
* To remove an entire auto-generated tag and all of its detections across every image, click the **×** on the tag in the Tag Library. This deletes the tag and removes it from all files it was assigned to.

## Privacy Considerations

All facial recognition processing is performed locally on your computer. Your photos and the generated face data are never uploaded or shared. The recognition model and any derived data remain on your machine at all times.

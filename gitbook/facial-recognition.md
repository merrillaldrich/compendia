---
description: Automatically identifying people in your photos
---

# Facial Recognition

## Overview

Compendia can scan your images to detect faces and help you identify the people in your library. Once faces are detected you can confirm or correct the identifications and apply person tags to your files, making it easy to find all photos of a specific person using the standard tag filtering system.

## How It Works

Compendia uses a local face detection and recognition model to analyse your images. All processing happens on your own machine and no image data is sent to any external service.

When you run facial recognition, Compendia scans the visible files for faces. It groups faces that appear to belong to the same person and presents them for your review. The more photos of a person that are in your library, the more accurately the model can identify them across different images.

## Running Facial Recognition

To run facial recognition, use the **Tools** menu and select the facial recognition option. Compendia will process the currently loaded files. Depending on the size of your library and the speed of your computer, this may take some time.

You can continue using the application while recognition runs in the background.

## Reviewing Results

After processing, Compendia presents the detected faces grouped by identity for you to review. For each group you can confirm that the faces belong to the same person, split incorrectly grouped faces into separate identities, or discard faces that were detected in error.

Taking time to review and correct the results improves the accuracy of future recognition runs on the same library.

## Tagging People

Once you are satisfied with the identified groups, you can assign a name to each identity. Compendia will create a tag for that person and apply it to all files where their face was detected. These tags work like any other tag in your library and can be used for filtering, searching, and organising your photos.

## Privacy Considerations

All facial recognition processing is performed locally on your computer. Your photos and the generated face data are never uploaded or shared. The recognition model and any derived data remain on your machine at all times.

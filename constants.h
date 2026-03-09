#ifndef CONSTANTS_H
#define CONSTANTS_H

/*! \brief Project-wide compile-time constants for Luminism. */
namespace Luminism {

/*! \brief Name of the hidden cache folder created alongside media files.
 *
 *  The leading dot hides the folder on Linux/macOS. Both IconGenerator and
 *  ExifParser write into this directory; MainWindow checks for its existence
 *  before loading a folder for the first time.
 */
constexpr const char* CacheFolderName = ".luminism_cache";

/*! \brief Default normalized side length for a manually dropped tag region. */
constexpr qreal DefaultTagRectSize = 0.15;

/*! \brief Filename suffix for the per-image face-descriptor cache file. */
constexpr const char* FaceDescriptorCacheSuffix = "-face-descriptors.json";

/*! \brief Filename suffix for the per-image known-face embedding cache file. */
constexpr const char* KnownFaceCacheSuffix = "-known-faces.json";

/*! \brief Prefix shared by all auto-detected face tags created during a recognition sweep. */
constexpr const char* AutoFaceTagPrefix = "Auto Detected Face ";

/*! \brief Maximum L2 distance between two face embeddings to be considered the same person. */
constexpr double FaceMatchThreshold = 0.6;

/*! \brief Default HOG detector adjust_threshold for face detection.
 *
 * Higher values require stronger evidence before declaring a face, reducing
 * false positives at the cost of potentially missing smaller faces.
 * Passed as the third argument to dlib::frontal_face_detector::operator().
 */
constexpr double FaceDetectionThreshold = 0.0;

/*! \brief Maximum number of auto-detected faces tagged per image during a recognition sweep.
 *
 *  Prevents crowd or large-group photos from flooding the library with dozens of
 *  auto-generated tags that the user is unlikely to label or care about.
 */
constexpr int MaxAutoFacesPerImage = 5;

/*! \brief Minimum number of affected files that triggers a confirmation dialog before
 *  removing a tag via the assignment-area close button.
 */
constexpr int BulkUntagWarningThreshold = 5;

} // namespace Luminism

#endif // CONSTANTS_H

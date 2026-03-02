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

/*! \brief Prefix shared by all auto-detected face tags created during a recognition sweep. */
constexpr const char* AutoFaceTagPrefix = "Auto Detected Face ";

/*! \brief Maximum L2 distance between two face embeddings to be considered the same person. */
constexpr double FaceMatchThreshold = 0.6;

/*! \brief Maximum number of auto-detected faces tagged per image during a recognition sweep.
 *
 *  Prevents crowd or large-group photos from flooding the library with dozens of
 *  auto-generated tags that the user is unlikely to label or care about.
 */
constexpr int MaxAutoFacesPerImage = 5;

} // namespace Luminism

#endif // CONSTANTS_H

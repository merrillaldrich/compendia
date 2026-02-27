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

} // namespace Luminism

#endif // CONSTANTS_H

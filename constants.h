#ifndef CONSTANTS_H
#define CONSTANTS_H

/*! \brief Project-wide compile-time constants for Compendia. */
namespace Compendia {

/*! \brief Name of the hidden cache folder created alongside media files.
 *
 *  The leading dot hides the folder on Linux/macOS. Both IconGenerator and
 *  ExifParser write into this directory; MainWindow checks for its existence
 *  before loading a folder for the first time.
 */
constexpr const char* CacheFolderName = ".compendia_cache";

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

/*! \brief When \c true, a pre-release warning dialog is shown before the first folder load.
 *
 *  Set to \c true for alpha/beta builds and \c false for production releases.
 */
constexpr bool ShowPreReleaseWarning = false;

/*! \brief When \c true and running on Linux, video frames are decoded to \c QImage and
 *  scaled in software before display, eliminating the moiré and aliasing artefacts
 *  produced by the platform video renderer (GStreamer) when downscaling large frames.
 *
 *  Has no effect on non-Linux platforms.  Set to \c false to revert to the native
 *  \c QGraphicsVideoItem path on all platforms.
 */
constexpr bool SoftwareVideoScaling = true;

/*! \brief Maximum Hamming distance between two pHash values for the images to be
 *  considered near-duplicates by the "Find Similar Images" feature.
 */
constexpr int SimilarImageThreshold = 10;

/*! \brief Milliseconds of idle time after the last rect adjustment before the
 *  background face-cache warmup is triggered.
 *
 *  Increase to give the user more time to adjust before the expensive inference
 *  starts; decrease for faster cache warming at the cost of potentially warming
 *  an intermediate rect.
 */
constexpr int RectWarmupDelayMs = 3000;

// ---------------------------------------------------------------------------
// Geography / map display
// ---------------------------------------------------------------------------

/*! \brief Mapbox raster tile URL template. {z}, {x}, {y}, and {token} are substituted at request time. */
constexpr const char* MapboxTileUrlTemplate =
    "https://api.mapbox.com/styles/v1/mapbox/streets-v11/tiles/256/{z}/{x}/{y}?access_token={token}";

/*! \brief Mapbox reverse-geocode endpoint. {lon}, {lat}, and {token} are substituted at request time. */
constexpr const char* MapboxReverseGeoUrl =
    "https://api.mapbox.com/geocoding/v5/mapbox.places/{lon},{lat}.json"
    "?types=place,locality,district&access_token={token}";

/*! \brief QSettings key storing the Mapbox API token. */
constexpr const char* MapApiTokenSettingsKey = "map/apiToken";

/*! \brief QSettings key storing the tile URL template (defaults to MapboxTileUrlTemplate). */
constexpr const char* MapTileUrlSettingsKey  = "map/tileUrlTemplate";

// ---------------------------------------------------------------------------
// Compendia free-tier proxy (Azure Functions — fill in your Function App name)
// ---------------------------------------------------------------------------

/*! \brief Tile proxy URL template. Replace \<functionapp\> with your Azure Function App name. */
constexpr const char* CompendiaTileProxyUrlTemplate =
    "https://<functionapp>.azurewebsites.net/api/tiles/{z}/{x}/{y}";

/*! \brief Geocode proxy URL. Replace \<functionapp\> with your Azure Function App name. */
constexpr const char* CompendiaGeocodeProxyUrl =
    "https://<functionapp>.azurewebsites.net/api/geocode";

/*! \brief Shared app key sent in the X-Compendia-Key request header.
 *  Must match the COMPENDIA_APP_KEY value set in the Azure Function App's Application Settings. */
constexpr const char* CompendiaProxyAppKey = "compendia-v1-replace-with-key";

/*! \brief QSettings key storing whether the Compendia free-tier proxy is active. */
constexpr const char* MapUseFreeTierSettingsKey = "map/useFreeTier";

/*! \brief HTTP User-Agent header sent with all map/geocode requests. */
constexpr const char* MapNetworkUserAgent = "Compendia/0.1 (contact: merrillaldrich@gmail.com)";

/*! \brief Default zoom level for the small map overlay (city-level). */
constexpr int MapOverlayZoom = 13;

/*! \brief Width of the small map overlay widget in pixels. */
constexpr int MapOverlayWidth = 160;

/*! \brief Height of the small map overlay widget in pixels. */
constexpr int MapOverlayHeight = 128;

/*! \brief Margin from the bottom-right corner of the preview for the map overlay (px). */
constexpr int MapOverlayMargin = 10;

} // namespace Compendia

#endif // CONSTANTS_H

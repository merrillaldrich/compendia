#include "exifparser.h"
#include "constants.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <vector>

/*! \brief Constructs an ExifParser (rarely needed; prefer the static interface).
 *
 * \param parent Optional Qt parent object.
 */
ExifParser::ExifParser(QObject *parent)
    : QObject{parent}
{}

/*! \brief Returns the absolute path to the JSON EXIF cache file for a given source image.
 *
 * \param filePath Absolute path to the source image file.
 * \return Absolute path to the corresponding EXIF cache file.
 */
QString ExifParser::cacheFilePath(const QString &filePath)
{
    QFileInfo fi(filePath);
    return fi.absolutePath() + "/" + Compendia::CacheFolderName + "/" + fi.completeBaseName() + "_exif.json";
}

/*! \brief Saves an EXIF map to the per-folder cache directory as a JSON file.
 *
 * \param filePath Absolute path to the original image (used to derive the cache path).
 * \param exifMap  The EXIF key-value map to persist.
 * \return True if the file was written successfully.
 */
bool ExifParser::saveExifToCache(const QString &filePath, const QMap<QString, QString> &exifMap)
{
    QFileInfo fi(filePath);
    QString cachePath = fi.absolutePath() + "/" + Compendia::CacheFolderName;
    QDir dir(cachePath);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create cache directory:" << cachePath;
            return false;
        }
    }

    QJsonObject obj;
    for (auto it = exifMap.cbegin(); it != exifMap.cend(); ++it)
        obj.insert(it.key(), it.value());

    QFile file(cacheFilePath(filePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open EXIF cache file for writing:" << file.errorString();
        return false;
    }

    file.write(QJsonDocument(obj).toJson());
    return true;
}

/*! \brief Attempts to load a cached EXIF map for the given source file.
 *
 * \param filePath Absolute path to the original image (used to derive the cache path).
 * \return The cached EXIF map, or an empty map if no valid cache entry exists.
 */
QMap<QString, QString> ExifParser::loadExifFromCache(const QString &filePath)
{
    QFile file(cacheFilePath(filePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open EXIF cache file for reading:" << file.errorString();
        return {};
    }

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    QMap<QString, QString> exifMap;
    for (auto it = obj.begin(); it != obj.end(); ++it)
        exifMap.insert(it.key(), it.value().toString());

    return exifMap;
}

/*! \brief Returns a map of EXIF key-value pairs for the given image file.
 *
 * Checks the on-disk JSON cache first. The cache is bypassed when the source
 * image has been modified more recently than the cached file. On a miss or
 * stale cache the EXIF data is parsed from the image and saved to cache.
 *
 * \param filePath Absolute path to the image file.
 * \return A map of EXIF tag name strings to value strings.
 */
QMap<QString, QString> ExifParser::getExifMap(QString filePath){

    QFileInfo sourceInfo(filePath);
    QFileInfo cacheInfo(cacheFilePath(filePath));

    bool cacheCurrent = cacheInfo.exists() &&
                        cacheInfo.lastModified() >= sourceInfo.lastModified();

    if (cacheCurrent)
        return loadExifFromCache(filePath);

    QMap<QString, QString> exifMap;

    ExifData* ed = nullptr;

    if (filePath.toLower().endsWith(".jpg")){
        ed = getExifStandard(filePath);
    } else if (filePath.toLower().endsWith(".heic")) {
        ed = getExifHeif(filePath);
    }

    if (ed != nullptr){
        exifMap = exifTagsToMap(ed);
        exif_data_unref(ed);
    }

    if (!exifMap.isEmpty())
        saveExifToCache(filePath, exifMap);

    return exifMap;
}

/*! \brief Loads raw EXIF data from a standard JPEG file using libexif.
 *
 * \param filePath Absolute path to the JPEG file.
 * \return A heap-allocated ExifData pointer, or nullptr on failure.
 */
ExifData* ExifParser::getExifStandard(QString filePath){

    // Read file into memory
    QFile file(filePath);
    if (!file.exists()) {
        qCritical() << "File does not exist:" << filePath;
        return nullptr;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file:" << file.errorString();
        return nullptr;
    }

    QByteArray imageData = file.readAll();
    file.close();

    if (imageData.isEmpty()) {
        qCritical() << "File is empty or could not be read.";
        return nullptr;
    }

    // Load EXIF data from memory
    ExifData *exifData = exif_data_new_from_data(
        reinterpret_cast<const unsigned char*>(imageData.constData()),
        static_cast<unsigned int>(imageData.size())
        );

    if (!exifData) {
        qCritical() << "No EXIF data found in file.";
        return nullptr;
    }

    return exifData;
}

/*! \brief Loads raw EXIF data from a HEIF/HEIC file using libheif then libexif.
 *
 * \param filePath Absolute path to the HEIC file.
 * \return A heap-allocated ExifData pointer, or nullptr on failure.
 */
ExifData* ExifParser::getExifHeif(QString filePath){

    // Change QString to char array
    QByteArray byteArray = filePath.toUtf8();
    const char* filename = byteArray.constData();

    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        qWarning() << "Failed to allocate HEIF context.\n";
        return nullptr;
    }

    // Read HEIF file
    heif_error err = heif_context_read_from_file(ctx, filename, nullptr);
    if (err.code != heif_error_Ok) {
        qWarning() << "Error reading HEIF file: " << err.message << "\n";
        heif_context_free(ctx);
        return nullptr;
    }

    // Get primary image handle
    heif_image_handle* handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok) {
        qWarning() << "Error getting primary image handle: " << err.message << "\n";
        heif_context_free(ctx);
        return nullptr;
    }

    // Get list of EXIF metadata IDs
    int count = heif_image_handle_get_number_of_metadata_blocks(handle, "Exif");
    if (count <= 0) {
        qWarning() << "No EXIF metadata found.\n";
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return nullptr;
    }

    std::vector<heif_item_id> ids(count);
    heif_image_handle_get_list_of_metadata_block_IDs(handle, "Exif", ids.data(), count);

    // Read the first EXIF block
    size_t exif_size = heif_image_handle_get_metadata_size(handle, ids[0]);
    std::vector<uint8_t> exif_buf(exif_size);

    err = heif_image_handle_get_metadata(handle, ids[0], exif_buf.data());
    if (err.code != heif_error_Ok) {
        qWarning() << "Error reading EXIF metadata: " << err.message << "\n";
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return nullptr;
    }

    // Skip the first 4 bytes (offset to TIFF header)
    if (exif_buf.size() <= 4) {
        qWarning() << "Invalid EXIF data size.\n";
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return nullptr;
    }

    const unsigned char* exif_bytes = exif_buf.data() + 4;
    size_t exif_len = exif_buf.size() - 4;

    // Parse EXIF using libexif
    ExifData* ed = exif_data_new_from_data(exif_bytes, exif_len);
    if (!ed) {
        qWarning() << "Failed to parse EXIF data.\n";
    }

    // Clean up
    heif_image_handle_release(handle);
    heif_context_free(ctx);

    return ed;
}

/*! \brief Decodes a HEIF/HEIC file to a QImage using libheif.
 *
 * \param filePath Absolute path to the HEIC file.
 * \return The decoded image, or a null QImage on failure.
 */
QImage ExifParser::loadHeifImage(const QString &filePath)
{
    QByteArray byteArray = filePath.toUtf8();
    const char* filename = byteArray.constData();

    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        qWarning() << "loadHeifImage: failed to allocate HEIF context";
        return {};
    }

    heif_error err = heif_context_read_from_file(ctx, filename, nullptr);
    if (err.code != heif_error_Ok) {
        qWarning() << "loadHeifImage: error reading file:" << err.message;
        heif_context_free(ctx);
        return {};
    }

    heif_image_handle* handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok) {
        qWarning() << "loadHeifImage: error getting image handle:" << err.message;
        heif_context_free(ctx);
        return {};
    }

    heif_image* himg = nullptr;
    err = heif_decode_image(handle, &himg,
                            heif_colorspace_RGB,
                            heif_chroma_interleaved_RGBA,
                            nullptr);
    if (err.code != heif_error_Ok) {
        qWarning() << "loadHeifImage: error decoding image:" << err.message;
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return {};
    }

    int stride = 0;
    const uint8_t* data = heif_image_get_plane_readonly(himg, heif_channel_interleaved, &stride);
    int width  = heif_image_get_width(himg,  heif_channel_interleaved);
    int height = heif_image_get_height(himg, heif_channel_interleaved);

    // Copy pixel data out before releasing libheif resources
    QImage result(data, width, height, stride, QImage::Format_RGBA8888);
    result = result.copy();

    heif_image_release(himg);
    heif_image_handle_release(handle);
    heif_context_free(ctx);

    return result;
}

/*! \brief Converts a libexif ExifData structure into a Qt key-value string map.
 *
 * \param ed The libexif data structure to iterate.
 * \return A map of EXIF tag names to their string values.
 */
QMap<QString, QString> ExifParser::exifTagsToMap(ExifData* ed) {
    QMap<QString, QString> map;

    for (int i = 0; i < EXIF_IFD_COUNT; i++) {
        ExifContent* content = ed->ifd[i];
        if (!content) continue;

        for (unsigned int j = 0; j < content->count; j++) {
            ExifEntry* entry = content->entries[j];
            char value[1024];
            exif_entry_get_value(entry, value, sizeof(value));
            if (*value) {
                map.insert(exif_tag_get_name(entry->tag), value);
            }
        }
    }
    return map;
}

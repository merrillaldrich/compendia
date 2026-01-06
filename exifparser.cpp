#include "exifparser.h"

ExifParser::ExifParser(QObject *parent)
    : QObject{parent}
{}

QMap<QString, QString> ExifParser::getExifMap(QString filePath){
    QMap<QString, QString> exifMap;

    ExifData* ed = nullptr;

    if (filePath.toLower().endsWith(".jpg")){
        ed = getExifStandard(filePath);
    } else if (filePath.toLower().endsWith(".heic")) {
        ed = getExifHeif(filePath);
    }

    if(ed != nullptr){
        exifMap = exifTagsToMap(ed);
    }
    exif_data_unref(ed);

    return exifMap;
}

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

    // Iterate and print EXIF tags
    //exif_data_foreach_content(exifData, printExifContent, nullptr);

    // Free EXIF data
    //exif_data_unref(exifData);

    return exifData;
}

ExifData* ExifParser::getExifHeif(QString filePath){

    // Change QString to char array
    // Convert to UTF-8 QByteArray
    QByteArray byteArray = filePath.toUtf8();

    // Allocate and copy
    char* filename = new char[byteArray.size() + 1];
    std::strcpy(filename, byteArray.constData());

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
    delete[] filename;

    return ed;
}

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
                qDebug() << exif_tag_get_name(entry->tag) << ": " << value << "\n";
                map.insert(exif_tag_get_name(entry->tag), value);
            }
        }
    }
    return map;
}

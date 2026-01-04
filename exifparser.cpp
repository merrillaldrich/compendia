#include "exifparser.h"

ExifParser::ExifParser(QObject *parent)
    : QObject{parent}
{}

int ExifParser::readEXIF(QString filePath){

    // Read file into memory
    QFile file(filePath);
    if (!file.exists()) {
        qCritical() << "File does not exist:" << filePath;
        return 1;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file:" << file.errorString();
        return 1;
    }

    QByteArray imageData = file.readAll();
    file.close();

    if (imageData.isEmpty()) {
        qCritical() << "File is empty or could not be read.";
        return 1;
    }

    // Load EXIF data from memory
    ExifData *exifData = exif_data_new_from_data(
        reinterpret_cast<const unsigned char*>(imageData.constData()),
        static_cast<unsigned int>(imageData.size())
        );

    if (!exifData) {
        qCritical() << "No EXIF data found in file.";
        return 1;
    }

    // Iterate and print EXIF tags
    exif_data_foreach_content(exifData, printExifContent, nullptr);

    // Free EXIF data
    exif_data_unref(exifData);

    return 0;
}

// Iterate over all EXIF content
void ExifParser::printExifContent(ExifContent *content, void *user_data) {
    exif_content_foreach_entry(content, printExifEntry, user_data);
}

// Callback function for iterating over EXIF entries
void ExifParser::printExifEntry(ExifEntry *entry, void *user_data) {
    char value[1024];
    exif_entry_get_value(entry, value, sizeof(value));

    if (*value) { // Only print if value is not empty
        QString tagName = QString::fromUtf8(exif_tag_get_name(entry->tag));
        QString tagValue = QString::fromUtf8(value);
        qDebug().noquote() << tagName << ":" << tagValue;
    }
}

int ExifParser::getExifHeif(QString absoluteFileName){

    // Change QString to char array
    // Convert to UTF-8 QByteArray
    QByteArray byteArray = absoluteFileName.toUtf8();

    // Allocate and copy
    char* filename = new char[byteArray.size() + 1];
    std::strcpy(filename, byteArray.constData());

    heif_context* ctx = heif_context_alloc();
    if (!ctx) {
        qWarning() << "Failed to allocate HEIF context.\n";
        return 1;
    }

    // Read HEIF file
    heif_error err = heif_context_read_from_file(ctx, filename, nullptr);
    if (err.code != heif_error_Ok) {
        qWarning() << "Error reading HEIF file: " << err.message << "\n";
        heif_context_free(ctx);
        return 1;
    }

    // Get primary image handle
    heif_image_handle* handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok) {
        qWarning() << "Error getting primary image handle: " << err.message << "\n";
        heif_context_free(ctx);
        return 1;
    }

    // Get list of EXIF metadata IDs
    int count = heif_image_handle_get_number_of_metadata_blocks(handle, "Exif");
    if (count <= 0) {
        qWarning() << "No EXIF metadata found.\n";
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return 1;
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
        return 1;
    }

    // Skip the first 4 bytes (offset to TIFF header)
    if (exif_buf.size() <= 4) {
        qWarning() << "Invalid EXIF data size.\n";
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        return 1;
    }

    const unsigned char* exif_bytes = exif_buf.data() + 4;
    size_t exif_len = exif_buf.size() - 4;

    // Parse EXIF using libexif
    ExifData* ed = exif_data_new_from_data(exif_bytes, exif_len);
    if (!ed) {
        qWarning() << "Failed to parse EXIF data.\n";
    } else {
        print_exif_tags(ed);
        exif_data_unref(ed);
    }

    // Cleanup
    heif_image_handle_release(handle);
    heif_context_free(ctx);

    return 0;
}

// Function to print EXIF tags
void ExifParser::print_exif_tags(ExifData* ed) {
    for (int i = 0; i < EXIF_IFD_COUNT; i++) {
        ExifContent* content = ed->ifd[i];
        if (!content) continue;

        for (unsigned int j = 0; j < content->count; j++) {
            ExifEntry* entry = content->entries[j];
            char value[1024];
            exif_entry_get_value(entry, value, sizeof(value));
            if (*value) {
                qDebug() << exif_tag_get_name(entry->tag) << ": " << value << "\n";
            }
        }
    }
}


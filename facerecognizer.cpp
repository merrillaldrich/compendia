#include "facerecognizer.h"
#include "constants.h"

#include <limits>

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

// ---------------------------------------------------------------------------
// Internal helper: build the cache file path for a given image
// ---------------------------------------------------------------------------

/*! \brief Returns the absolute path of the face-descriptor cache file for \p imagePath.
 *
 * The cache lives at: {imageDir}/.luminism_cache/{baseName}-face-descriptors.json
 *
 * \param imagePath Absolute path to the source image.
 * \return Absolute path to the corresponding cache file.
 */
static QString descriptorCacheFilePath(const QString &imagePath)
{
    QFileInfo fi(imagePath);
    return fi.dir().absolutePath()
           + "/" + Luminism::CacheFolderName
           + "/" + fi.completeBaseName()
           + Luminism::FaceDescriptorCacheSuffix;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

/*! \brief Constructs a FaceRecognizer.
 *
 * \param parent Optional Qt parent object.
 */
FaceRecognizer::FaceRecognizer(QObject *parent)
    : QObject{parent}
{}

// ---------------------------------------------------------------------------
// Model loading
// ---------------------------------------------------------------------------

/*! \brief Loads the shape predictor and face recognition model from \p modelsDir.
 *
 * \param modelsDir Absolute path to the directory containing the .dat files.
 * \return True on success; false if any file is missing or fails to deserialize.
 */
bool FaceRecognizer::loadModels(const QString &modelsDir)
{
    try {
        std::string dir = modelsDir.toStdString();
        dlib::deserialize(dir + "/shape_predictor_5_face_landmarks.dat") >> sp_;
        dlib::deserialize(dir + "/dlib_face_recognition_resnet_model_v1.dat") >> net_;
        detector_ = dlib::get_frontal_face_detector();
        models_loaded_ = true;
        return true;
    } catch (const std::exception &e) {
        qDebug() << "FaceRecognizer::loadModels failed:" << e.what();
        models_loaded_ = false;
        return false;
    }
}

/*! \brief Returns true if the model files have been loaded successfully. */
bool FaceRecognizer::modelsLoaded() const
{
    return models_loaded_;
}

// ---------------------------------------------------------------------------
// Image conversion
// ---------------------------------------------------------------------------

/*! \brief Converts a QImage to a dlib RGB pixel array for use with the detector.
 *
 * \param qimg The source QImage (any format; will be converted internally to RGB888).
 * \return A dlib array2d of rgb_pixel values.
 */
dlib::array2d<dlib::rgb_pixel> FaceRecognizer::qimageToDlibArray(const QImage &qimg)
{
    QImage img = qimg.convertToFormat(QImage::Format_RGB888);

    dlib::array2d<dlib::rgb_pixel> dlibImage;
    dlibImage.set_size(img.height(), img.width());

    for (int y = 0; y < img.height(); ++y) {
        const uchar *line = img.constScanLine(y);
        for (int x = 0; x < img.width(); ++x)
            dlibImage[y][x] = dlib::rgb_pixel(line[x * 3], line[x * 3 + 1], line[x * 3 + 2]);
    }
    return dlibImage;
}

// ---------------------------------------------------------------------------
// Core embedding computation
// ---------------------------------------------------------------------------

/*! \brief Computes a 128-d face embedding for a single detected face rectangle.
 *
 * Runs the 5-point shape predictor on \p rect, extracts a 150×150 aligned chip,
 * and passes it through the ResNet network.
 *
 * \param dlibImg The source image as a dlib rgb_pixel array.
 * \param rect    The face bounding rectangle returned by the HOG detector.
 * \return A 128-d column matrix, or an empty matrix on failure.
 */
dlib::matrix<float,0,1> FaceRecognizer::computeEmbedding(
    const dlib::array2d<dlib::rgb_pixel> &dlibImg,
    const dlib::rectangle &rect)
{
    dlib::full_object_detection shape = sp_(dlibImg, rect);

    dlib::matrix<dlib::rgb_pixel> faceChip;
    dlib::extract_image_chip(dlibImg,
                             dlib::get_face_chip_details(shape, 150, 0.25),
                             faceChip);

    std::vector<dlib::matrix<dlib::rgb_pixel>> batch;
    batch.push_back(faceChip);

    std::vector<dlib::matrix<float,0,1>> embeddings = net_(batch);
    if (embeddings.empty())
        return dlib::matrix<float,0,1>();
    return embeddings[0];
}

// ---------------------------------------------------------------------------
// Public detection methods
// ---------------------------------------------------------------------------

/*! \brief Runs face detection on the source image and returns normalised bounding rectangles.
 *
 * \param sourceImage The image to run face detection on.
 * \return A list of QRectF values, one per detected face, in normalised image coordinates (0.0–1.0).
 */
QList<QRectF> FaceRecognizer::detectFaces(const QImage &sourceImage)
{
    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(sourceImage);

    std::vector<dlib::rectangle> faces;
    if (models_loaded_) {
        faces = detector_(dlibImg);
    } else {
        dlib::frontal_face_detector localDetector = dlib::get_frontal_face_detector();
        faces = localDetector(dlibImg);
    }

    const double w = sourceImage.width();
    const double h = sourceImage.height();

    QList<QRectF> rects;
    for (const auto &face : faces)
        rects.append(QRectF(face.left() / w, face.top() / h,
                            face.width() / w, face.height() / h));
    return rects;
}

/*! \brief Detects faces in \p img and returns each face's normalised rect and embedding.
 *
 * \param img The source image.
 * \return A vector of (normalised QRectF, 128-d embedding) pairs, one per detected face.
 */
QVector<QPair<QRectF, dlib::matrix<float,0,1>>>
FaceRecognizer::detectFacesWithEmbeddings(const QImage &img)
{
    if (!models_loaded_)
        return {};

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(img);
    std::vector<dlib::rectangle> dets = detector_(dlibImg);

    const double w = img.width();
    const double h = img.height();

    QVector<QPair<QRectF, dlib::matrix<float,0,1>>> results;
    for (const auto &det : dets) {
        try {
            dlib::matrix<float,0,1> emb = computeEmbedding(dlibImg, det);
            if (emb.size() == 0)
                continue;
            QRectF normRect(det.left() / w, det.top() / h,
                            det.width() / w, det.height() / h);
            results.append({normRect, emb});
        } catch (const std::exception &) {
            continue;
        }
    }
    return results;
}

/*! \brief Computes a face embedding for a user-drawn region in \p img.
 *
 * Runs the HOG detector and picks the detection with the best IoU against \p normalizedRect.
 * Falls back to a synthetic rectangle from \p normalizedRect if no detection has IoU > 0.3.
 *
 * \param img            The source image.
 * \param normalizedRect The user-drawn region in normalised image coordinates (0.0–1.0).
 * \return A 128-d embedding, or an empty matrix if the embedding could not be computed.
 */
dlib::matrix<float,0,1> FaceRecognizer::computeEmbeddingFromRegion(
    const QImage &img, const QRectF &normalizedRect)
{
    if (!models_loaded_)
        return dlib::matrix<float,0,1>();

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(img);

    const double w = img.width();
    const double h = img.height();

    // Convert normalised user rect to pixel space
    long left   = static_cast<long>(normalizedRect.x()                          * w);
    long top    = static_cast<long>(normalizedRect.y()                          * h);
    long right  = static_cast<long>((normalizedRect.x() + normalizedRect.width())  * w);
    long bottom = static_cast<long>((normalizedRect.y() + normalizedRect.height()) * h);
    dlib::rectangle userRect(left, top, right, bottom);

    // Find HOG detection with best IoU against the user rect
    std::vector<dlib::rectangle> dets = detector_(dlibImg);
    double bestIoU = 0.0;
    dlib::rectangle bestDet;

    for (const auto &det : dets) {
        dlib::rectangle inter(
            std::max(det.left(),   userRect.left()),
            std::max(det.top(),    userRect.top()),
            std::min(det.right(),  userRect.right()),
            std::min(det.bottom(), userRect.bottom())
        );
        if (inter.is_empty())
            continue;
        double interArea = static_cast<double>(inter.width())  * inter.height();
        double unionArea = static_cast<double>(det.area())
                         + static_cast<double>(userRect.area()) - interArea;
        double iou = interArea / unionArea;
        if (iou > bestIoU) {
            bestIoU = iou;
            bestDet = det;
        }
    }

    dlib::rectangle chosenRect = (bestIoU > 0.3) ? bestDet : userRect;

    try {
        return computeEmbedding(dlibImg, chosenRect);
    } catch (const std::exception &) {
        return dlib::matrix<float,0,1>();
    }
}

// ---------------------------------------------------------------------------
// Descriptor cache
// ---------------------------------------------------------------------------

/*! \brief Attempts to load cached face descriptors for \p imagePath.
 *
 * The cache is considered valid only when the stored source_file_mtime matches
 * the image file's current last-modified timestamp.
 *
 * \param imagePath Absolute path to the source image file.
 * \return The cached descriptors on a cache hit; std::nullopt on a miss or stale cache.
 */
std::optional<QVector<QPair<QRectF, dlib::matrix<float,0,1>>>>
FaceRecognizer::loadDescriptorCache(const QString &imagePath)
{
    QString cachePath = descriptorCacheFilePath(imagePath);
    QFile f(cachePath);
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    if (!doc.isObject())
        return std::nullopt;

    QJsonObject root = doc.object();

    // Validate mtime
    QString storedMtime  = root["source_file_mtime"].toString();
    QString currentMtime = QFileInfo(imagePath).lastModified().toString(Qt::ISODate);
    if (storedMtime != currentMtime)
        return std::nullopt;

    QVector<QPair<QRectF, dlib::matrix<float,0,1>>> results;
    const QJsonArray facesArr = root["faces"].toArray();

    for (const QJsonValue &faceVal : facesArr) {
        QJsonObject faceObj = faceVal.toObject();

        const QJsonArray rectArr = faceObj["rect"].toArray();
        if (rectArr.size() != 4)
            continue;
        QRectF rect(rectArr[0].toDouble(), rectArr[1].toDouble(),
                    rectArr[2].toDouble(), rectArr[3].toDouble());

        const QJsonArray embArr = faceObj["embedding"].toArray();
        if (embArr.size() != 128)
            continue;
        dlib::matrix<float,0,1> emb(128);
        for (int i = 0; i < 128; ++i)
            emb(i) = static_cast<float>(embArr[i].toDouble());

        results.append({rect, emb});
    }

    return results;
}

/*! \brief Writes face descriptors for \p imagePath to the descriptor cache.
 *
 * The cache file is written into the .luminism_cache sub-directory next to
 * the image file. The directory is created if it does not exist.
 *
 * \param imagePath   Absolute path to the source image file.
 * \param descriptors The (normalised rect, embedding) pairs to cache.
 */
void FaceRecognizer::saveDescriptorCache(
    const QString &imagePath,
    const QVector<QPair<QRectF, dlib::matrix<float,0,1>>> &descriptors)
{
    QString cachePath = descriptorCacheFilePath(imagePath);

    // Ensure the cache directory exists
    QDir().mkpath(QFileInfo(cachePath).dir().absolutePath());

    QJsonObject root;
    root["source_file_mtime"] = QFileInfo(imagePath).lastModified().toString(Qt::ISODate);

    QJsonArray facesArr;
    for (const auto &pair : descriptors) {
        QJsonObject faceObj;

        QJsonArray rectArr;
        rectArr.append(pair.first.x());
        rectArr.append(pair.first.y());
        rectArr.append(pair.first.width());
        rectArr.append(pair.first.height());
        faceObj["rect"] = rectArr;

        QJsonArray embArr;
        for (long i = 0; i < pair.second.size(); ++i)
            embArr.append(static_cast<double>(pair.second(i)));
        faceObj["embedding"] = embArr;

        facesArr.append(faceObj);
    }
    root["faces"] = facesArr;

    QFile f(cachePath);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

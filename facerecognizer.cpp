#include "facerecognizer.h"
#include "constants.h"

#include <limits>

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QSet>
#include <QDebug>
#include <QtConcurrent>

static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};

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

/*! \brief Returns the absolute path of the known-face embedding cache file for \p imagePath.
 *
 * The cache lives at: {imageDir}/.luminism_cache/{baseName}-known-faces.json
 *
 * \param imagePath Absolute path to the source image.
 * \return Absolute path to the corresponding cache file.
 */
static QString knownFaceCacheFilePath(const QString &imagePath)
{
    QFileInfo fi(imagePath);
    return fi.dir().absolutePath()
           + "/" + Luminism::CacheFolderName
           + "/" + fi.completeBaseName()
           + Luminism::KnownFaceCacheSuffix;
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

/*! \brief Returns the current HOG detector adjust_threshold. */
double FaceRecognizer::detectionThreshold() const
{
    return detectionThreshold_;
}

/*! \brief Sets the HOG detector adjust_threshold used during face detection.
 *
 * \param threshold The new adjust_threshold value.
 */
void FaceRecognizer::setDetectionThreshold(double threshold)
{
    detectionThreshold_ = threshold;
}

/*! \brief Returns the current Euclidean-distance threshold for face matching. */
double FaceRecognizer::matchThreshold() const
{
    return matchThreshold_;
}

/*! \brief Sets the Euclidean-distance threshold used when matching detected faces
 *         against known-person embeddings during a recognition sweep.
 *
 * \param threshold The new match threshold value.
 */
void FaceRecognizer::setMatchThreshold(double threshold)
{
    matchThreshold_ = threshold;
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
    QElapsedTimer t;
    t.start();

    dlib::full_object_detection shape = sp_(dlibImg, rect);
    qDebug() << "[FaceRecognizer]   shape predictor:  " << t.restart() << "ms";

    dlib::matrix<dlib::rgb_pixel> faceChip;
    dlib::extract_image_chip(dlibImg,
                             dlib::get_face_chip_details(shape, 150, 0.25),
                             faceChip);
    qDebug() << "[FaceRecognizer]   extract chip:     " << t.restart() << "ms";

    std::vector<dlib::matrix<dlib::rgb_pixel>> batch;
    batch.push_back(faceChip);

    std::vector<dlib::matrix<float,0,1>> embeddings = net_(batch);
    qDebug() << "[FaceRecognizer]   ResNet inference: " << t.elapsed() << "ms";

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
        std::vector<dlib::frontal_face_detector::rect_detection> rawDets;
        detector_(dlibImg, rawDets, detectionThreshold_);
        faces.reserve(rawDets.size());
        for (const auto &rd : rawDets)
            faces.push_back(rd.rect);
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
 * The image is downscaled to at most kMaxDetectionDim on its longest edge before detection
 * so the HOG sliding-window scan is fast even on large camera images.  Because the scale
 * is uniform, normalised coordinates derived from the downscaled dimensions are identical
 * to those that would be obtained from the original, so no remapping is required.
 *
 * \param img The source image.
 * \return A vector of (normalised QRectF, 128-d embedding) pairs, one per detected face.
 */
QVector<QPair<QRectF, dlib::matrix<float,0,1>>>
FaceRecognizer::detectFacesWithEmbeddings(const QImage &img)
{
    if (!models_loaded_)
        return {};

    QMutexLocker lock(&embeddingMutex_);

    QElapsedTimer t;
    t.start();
    qDebug() << "[FaceRecognizer] detectFacesWithEmbeddings — input:" << img.width() << "x" << img.height();

    static constexpr int kMaxDetectionDim = 1200;
    const int longEdge = qMax(img.width(), img.height());
    const QImage detectImg = (longEdge > kMaxDetectionDim)
        ? img.scaled(kMaxDetectionDim * img.width()  / longEdge,
                     kMaxDetectionDim * img.height() / longEdge,
                     Qt::IgnoreAspectRatio,
                     Qt::SmoothTransformation)
        : img;
    qDebug() << "[FaceRecognizer] downscale to" << detectImg.width() << "x" << detectImg.height() << ":" << t.restart() << "ms";

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(detectImg);
    qDebug() << "[FaceRecognizer] qimageToDlibArray:            " << t.restart() << "ms";

    std::vector<dlib::frontal_face_detector::rect_detection> rawDets;
    detector_(dlibImg, rawDets, detectionThreshold_);
    qDebug() << "[FaceRecognizer] HOG detector (" << rawDets.size() << "faces found):" << t.restart() << "ms";

    std::vector<dlib::rectangle> dets;
    dets.reserve(rawDets.size());
    for (const auto &rd : rawDets)
        dets.push_back(rd.rect);

    const double w = detectImg.width();
    const double h = detectImg.height();

    // Phase A: run the shape predictor and extract a 150×150 chip for every
    // detection.  Keep the normalised rect paired with each chip so we can
    // reassemble the results after the batch network call.
    std::vector<dlib::matrix<dlib::rgb_pixel>> chips;
    std::vector<QRectF> normRects;
    chips.reserve(dets.size());
    normRects.reserve(dets.size());

    for (const auto &det : dets) {
        try {
            dlib::full_object_detection shape = sp_(dlibImg, det);
            dlib::matrix<dlib::rgb_pixel> chip;
            dlib::extract_image_chip(dlibImg,
                                     dlib::get_face_chip_details(shape, 150, 0.25),
                                     chip);
            chips.push_back(std::move(chip));
            normRects.emplace_back(det.left() / w, det.top() / h,
                                   det.width() / w, det.height() / h);
        } catch (const std::exception &) {
            // skip malformed detection
        }
    }
    qDebug() << "[FaceRecognizer] shape predictor + chip extraction (" << chips.size() << "chips):" << t.restart() << "ms";

    if (chips.empty()) {
        qDebug() << "[FaceRecognizer] detectFacesWithEmbeddings done, 0 faces";
        return {};
    }

    // Phase B: single batched ResNet forward pass over all chips at once.
    std::vector<dlib::matrix<float,0,1>> embeddings = net_(chips);
    qDebug() << "[FaceRecognizer] ResNet batch inference (" << chips.size() << "faces):" << t.elapsed() << "ms";

    // Phase C: pair each embedding with its normalised rect.
    QVector<QPair<QRectF, dlib::matrix<float,0,1>>> results;
    results.reserve(static_cast<int>(embeddings.size()));
    for (size_t i = 0; i < embeddings.size(); ++i)
        results.append({normRects[i], embeddings[i]});

    qDebug() << "[FaceRecognizer] detectFacesWithEmbeddings done, total faces:" << results.size();
    return results;
}

/*! \brief Non-locking core of computeEmbeddingFromRegion; must be called under embeddingMutex_.
 *
 * Runs the HOG detector and picks the detection with the best IoU against \p normalizedRect.
 * Falls back to a synthetic rectangle from \p normalizedRect if no detection has IoU > 0.3.
 *
 * \param img            The source image.
 * \param normalizedRect The user-drawn region in normalised image coordinates (0.0–1.0).
 * \return A 128-d embedding, or an empty matrix if the embedding could not be computed.
 */
dlib::matrix<float,0,1> FaceRecognizer::computeEmbeddingFromRegionUnlocked(
    const QImage &img, const QRectF &normalizedRect)
{
    QElapsedTimer t;
    t.start();
    qDebug() << "[FaceRecognizer] computeEmbeddingFromRegion — input:" << img.width() << "x" << img.height();

    // Downscale to at most kMaxDetectionDim on the longest edge, same as
    // detectFacesWithEmbeddings.  The normalised rect is dimensionless so it
    // maps directly into whatever pixel space we work in.
    static constexpr int kMaxDetectionDim = 1200;
    const int longEdge = qMax(img.width(), img.height());
    const QImage detectImg = (longEdge > kMaxDetectionDim)
        ? img.scaled(kMaxDetectionDim * img.width()  / longEdge,
                     kMaxDetectionDim * img.height() / longEdge,
                     Qt::IgnoreAspectRatio,
                     Qt::SmoothTransformation)
        : img;
    qDebug() << "[FaceRecognizer] downscale to" << detectImg.width() << "x" << detectImg.height() << ":" << t.restart() << "ms";

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(detectImg);
    qDebug() << "[FaceRecognizer] qimageToDlibArray:            " << t.restart() << "ms";

    const double w = detectImg.width();
    const double h = detectImg.height();

    // Convert normalised user rect to pixel space
    long left   = static_cast<long>(normalizedRect.x()                           * w);
    long top    = static_cast<long>(normalizedRect.y()                           * h);
    long right  = static_cast<long>((normalizedRect.x() + normalizedRect.width()) * w);
    long bottom = static_cast<long>((normalizedRect.y() + normalizedRect.height()) * h);
    dlib::rectangle userRect(left, top, right, bottom);

    // Find HOG detection with best IoU against the user rect
    std::vector<dlib::frontal_face_detector::rect_detection> rawDets;
    detector_(dlibImg, rawDets, detectionThreshold_);
    qDebug() << "[FaceRecognizer] HOG detector (" << rawDets.size() << "dets):" << t.restart() << "ms";

    std::vector<dlib::rectangle> dets;
    dets.reserve(rawDets.size());
    for (const auto &rd : rawDets)
        dets.push_back(rd.rect);
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

/*! \brief Computes a face embedding for a user-drawn region in \p img.
 *
 * Runs the HOG detector and picks the detection with the best IoU against \p normalizedRect.
 * Falls back to a synthetic rectangle from \p normalizedRect if no detection has IoU > 0.3.
 * Thread-safe: acquires embeddingMutex_ before calling dlib inference.
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

    QMutexLocker lock(&embeddingMutex_);
    return computeEmbeddingFromRegionUnlocked(img, normalizedRect);
}

// ---------------------------------------------------------------------------
// Embedding warmup scheduling
// ---------------------------------------------------------------------------

/*! \brief Triggers background known-face embedding warmup for \p tf if models are loaded.
 *
 * Collects all user-labeled tag regions from \p tf, checks the known-face
 * cache for missing embeddings, and launches a QtConcurrent task to compute
 * them.  Emits warmupScheduled(misses) just before the task starts so the
 * caller can display a progress bar.  Does nothing when a warmup is already
 * running.
 *
 * \param tf The file being deselected. May be nullptr.
 */
void FaceRecognizer::scheduleEmbeddingWarmup(TaggedFile* tf)
{
    if (!tf || !models_loaded_)
        return;

    // Don't start a new warmup while one is still running — the previous task is
    // still emitting signals, and resetting the progress bar max mid-flight would
    // corrupt the counter and potentially trigger an early finishProcess.
    if (warmupFuture_.isRunning())
        return;

    QVector<std::tuple<QString, QString, QRectF>> regions;
    for (Tag* tag : *tf->tags()) {
        if (tag->getName().startsWith(Luminism::AutoFaceTagPrefix))
            continue;
        auto r = tf->tagRect(tag);
        if (!r.has_value())
            continue;
        regions.append({ tag->tagFamily->getName(), tag->getName(), r.value() });
    }
    if (regions.isEmpty())
        return;

    const QString imagePath = tf->filePath + "/" + tf->fileName;

    // Pre-check: skip if everything is already cached.
    auto cached = FaceRecognizer::loadKnownFaceCache(imagePath);
    int misses = regions.size();
    if (cached.has_value()) {
        int hits = 0;
        for (const auto &[family, name, rect] : regions) {
            for (const KnownFaceCacheEntry &e : *cached) {
                if (e.tagFamily == family && e.tagName == name && e.rect == rect) {
                    ++hits; break;
                }
            }
        }
        if (hits == regions.size())
            return;
        misses = regions.size() - hits;
    }

    emit warmupScheduled(misses);

    warmupFuture_ = QtConcurrent::run([this, imagePath, regions]() {
        warmupKnownFaceEmbeddings(imagePath, regions);
    });
}

// ---------------------------------------------------------------------------
// Face recognition sweep
// ---------------------------------------------------------------------------

/*! \brief Runs Phases 1–3 of the face recognition sweep synchronously on the calling thread.
 *
 * Phase 1 seeds known-person embeddings from user-labeled regions across \p allFiles.
 * Phase 2 clears all existing auto-detected face tags from \p targetFiles.
 * Phase 3 scans every image in \p targetFiles, matches detected faces against
 *         known embeddings using matchThreshold_, and calls \p tagFactory to
 *         create new tags for unrecognised faces.
 *
 * \param allFiles      All files in the model (used to seed known embeddings).
 * \param targetFiles   The files to actually scan and tag.
 * \param tagFactory    Factory called as tagFactory(familyName, tagName) to create tags.
 * \param shouldCancel  Returns true when the operation should abort.
 * \param processEvents Called periodically to keep the UI responsive.
 */
void FaceRecognizer::runSweep(
    const QVector<TaggedFile*> &allFiles,
    const QVector<TaggedFile*> &targetFiles,
    std::function<Tag*(const QString&, const QString&)> tagFactory,
    std::function<bool()> shouldCancel,
    std::function<void()> processEvents)
{
    // ----- Phase 1: Seed known-person embeddings from user-labeled regions -----
    QVector<FaceDescriptor> knownFaces;
    for (TaggedFile* tf : allFiles) {
        if (shouldCancel())
            break;
        processEvents();

        QVector<QPair<Tag*, QRectF>> pendingTags;
        for (Tag* tag : *tf->tags()) {
            if (tag->getName().startsWith(Luminism::AutoFaceTagPrefix))
                continue;
            auto r = tf->tagRect(tag);
            if (!r.has_value())
                continue;
            pendingTags.append({tag, r.value()});
        }
        if (pendingTags.isEmpty())
            continue;

        const QString imagePath = tf->filePath + "/" + tf->fileName;

        auto cachedEntries = FaceRecognizer::loadKnownFaceCache(imagePath);

        QVector<KnownFaceCacheEntry> updatedCache;
        bool cacheNeedsWrite = false;
        QImage img;  // loaded lazily — at most once per file

        for (const auto &[tag, rect] : pendingTags) {
            const QString tagFamily = tag->tagFamily->getName();
            const QString tagName   = tag->getName();

            bool hitFound = false;
            if (cachedEntries.has_value()) {
                for (const KnownFaceCacheEntry &entry : *cachedEntries) {
                    if (entry.tagFamily == tagFamily
                            && entry.tagName == tagName
                            && entry.rect   == rect) {
                        qDebug() << "[Phase1] cache hit:" << tagName;
                        knownFaces.append({entry.embedding, tag});
                        updatedCache.append(entry);
                        hitFound = true;
                        break;
                    }
                }
            }
            if (hitFound)
                continue;

            // Cache miss — load image lazily
            if (img.isNull()) {
                QImageReader ir(imagePath);
                ir.setAutoTransform(true);
                img = ir.read();
                if (img.isNull()) {
                    qDebug() << "[Phase1] failed to load image:" << imagePath;
                    break;  // skip all remaining tags for this file
                }
            }

            qDebug() << "[Phase1] cache miss, computing embedding for:" << tagName;
            dlib::matrix<float,0,1> emb = computeEmbeddingFromRegion(img, rect);
            if (emb.size() == 0)
                continue;

            knownFaces.append({emb, tag});
            KnownFaceCacheEntry newEntry;
            newEntry.tagFamily = tagFamily;
            newEntry.tagName   = tagName;
            newEntry.rect      = rect;
            newEntry.embedding = emb;
            updatedCache.append(newEntry);
            cacheNeedsWrite = true;
        }

        if (cacheNeedsWrite)
            FaceRecognizer::saveKnownFaceCache(imagePath, updatedCache);
    }

    // ----- Phase 2: Clear all previous auto-detected face tags -----
    for (TaggedFile* tf : targetFiles) {
        QSet<Tag*> toRemove;
        for (Tag* t : *tf->tags()) {
            if (t->getName().startsWith(Luminism::AutoFaceTagPrefix))
                toRemove.insert(t);
        }
        for (Tag* t : toRemove)
            tf->removeTag(t);
    }

    // ----- Phase 3: Recognition sweep across target image files -----
    int autoFaceCounter = 1;
    for (int i = 0; i < targetFiles.size(); ++i) {
        if (shouldCancel())
            break;
        emit sweepProgress(i, targetFiles.size());
        processEvents();

        TaggedFile* tf = targetFiles[i];
        if (videoExts.contains(QFileInfo(tf->fileName).suffix().toLower()))
            continue;

        QString imagePath = tf->filePath + "/" + tf->fileName;
        qDebug() << "[FindFaces] processing:" << tf->fileName;
        QElapsedTimer fileTimer;
        fileTimer.start();

        // Try the descriptor cache first
        auto cached = FaceRecognizer::loadDescriptorCache(imagePath);
        qDebug() << "[FindFaces]   cache lookup:  " << fileTimer.restart() << "ms"
                 << (cached.has_value() ? "(hit)" : "(miss)");

        QVector<QPair<QRectF, dlib::matrix<float,0,1>>> faces;
        if (cached.has_value()) {
            faces = cached.value();
        } else {
            QImageReader ir(imagePath);
            ir.setAutoTransform(true);
            QImage img = ir.read();
            qDebug() << "[FindFaces]   image load:   " << fileTimer.restart() << "ms"
                     << img.width() << "x" << img.height();
            if (img.isNull())
                continue;
            faces = detectFacesWithEmbeddings(img);
            qDebug() << "[FindFaces]   detection:    " << fileTimer.restart() << "ms";
            FaceRecognizer::saveDescriptorCache(imagePath, faces);
            qDebug() << "[FindFaces]   cache save:   " << fileTimer.elapsed() << "ms";
        }

        int autoFacesTaggedThisImage = 0;
        for (const auto &[rect, embedding] : faces) {
            // Find the closest known face by Euclidean distance
            double minDist = std::numeric_limits<double>::max();
            Tag*   bestTag = nullptr;
            for (const FaceDescriptor &fd : knownFaces) {
                double dist = dlib::length(fd.embedding - embedding);
                if (dist < minDist) {
                    minDist = dist;
                    bestTag = fd.tag;
                }
            }

            const bool isUserNamedMatch = minDist < matchThreshold_
                                          && bestTag != nullptr
                                          && !bestTag->getName().startsWith(Luminism::AutoFaceTagPrefix);

            if (isUserNamedMatch) {
                // Always tag faces that match a user-named person, regardless of limit
                tf->addTag(bestTag, rect);
            } else {
                // Auto-named match or unrecognised — apply the per-image limit.
                if (autoFacesTaggedThisImage >= Luminism::MaxAutoFacesPerImage)
                    continue;

                if (minDist < matchThreshold_ && bestTag != nullptr) {
                    // Recognised, but matched an auto-named tag
                    tf->addTag(bestTag, rect);
                } else {
                    // New person — create a sequential auto tag and seed future matching
                    QString tagName = QString("%1%2")
                        .arg(Luminism::AutoFaceTagPrefix)
                        .arg(autoFaceCounter++, 2, 10, QChar('0'));
                    Tag* newTag = tagFactory("People", tagName);
                    tf->addTag(newTag, rect);
                    knownFaces.append({embedding, newTag});
                }
                ++autoFacesTaggedThisImage;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Known-face embedding warmup
// ---------------------------------------------------------------------------

/*! \brief Checks the known-face cache for each region in \p tagRegions and computes
 *         any missing embeddings, saving the updated cache to disk when done.
 *
 * \param imagePath  Absolute path to the source image.
 * \param tagRegions List of (tagFamilyName, tagName, normalizedRect) tuples for
 *                   each user-labeled face region on this file.
 */
void FaceRecognizer::warmupKnownFaceEmbeddings(
    const QString &imagePath,
    const QVector<std::tuple<QString, QString, QRectF>> &tagRegions)
{
    if (!models_loaded_ || tagRegions.isEmpty())
        return;

    auto cachedEntries = FaceRecognizer::loadKnownFaceCache(imagePath);
    QVector<KnownFaceCacheEntry> updatedCache =
        cachedEntries.has_value() ? *cachedEntries : QVector<KnownFaceCacheEntry>{};

    QImage img;  // loaded lazily on first cache miss
    bool cacheNeedsWrite = false;

    for (const auto &[family, name, rect] : tagRegions) {
        bool hit = false;
        for (const KnownFaceCacheEntry &e : updatedCache) {
            if (e.tagFamily == family && e.tagName == name && e.rect == rect) {
                hit = true;
                break;
            }
        }
        if (hit)
            continue;

        if (img.isNull()) {
            QImageReader ir(imagePath);
            ir.setAutoTransform(true);
            img = ir.read();
            if (img.isNull())
                return;
        }

        dlib::matrix<float,0,1> emb;
        {
            QMutexLocker lock(&embeddingMutex_);
            emb = computeEmbeddingFromRegionUnlocked(img, rect);
        }
        if (emb.size() == 0)
            continue;

        KnownFaceCacheEntry e;
        e.tagFamily = family;
        e.tagName   = name;
        e.rect      = rect;
        e.embedding = emb;
        updatedCache.append(e);
        cacheNeedsWrite = true;

        emit embeddingWarmedUp();
    }

    if (cacheNeedsWrite)
        FaceRecognizer::saveKnownFaceCache(imagePath, updatedCache);
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

// ---------------------------------------------------------------------------
// Known-face embedding cache
// ---------------------------------------------------------------------------

/*! \brief Attempts to load the known-face embedding cache for \p imagePath.
 *
 * The cache is considered valid only when the stored source_file_mtime matches
 * the image file's current last-modified timestamp.  On a mismatch the entire
 * cache for that file is considered stale.
 *
 * \param imagePath Absolute path to the source image file.
 * \return The cached entries on a cache hit; std::nullopt on a miss or stale cache.
 */
std::optional<QVector<KnownFaceCacheEntry>>
FaceRecognizer::loadKnownFaceCache(const QString &imagePath)
{
    QString cachePath = knownFaceCacheFilePath(imagePath);
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

    QVector<KnownFaceCacheEntry> results;
    const QJsonArray facesArr = root["known_faces"].toArray();

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

        KnownFaceCacheEntry entry;
        entry.tagFamily = faceObj["tag_family"].toString();
        entry.tagName   = faceObj["tag_name"].toString();
        entry.rect      = rect;
        entry.embedding = emb;
        results.append(entry);
    }

    return results;
}

/*! \brief Writes known-face embedding entries for \p imagePath to the cache.
 *
 * The cache file is written into the .luminism_cache sub-directory next to
 * the image file. The directory is created if it does not exist.
 *
 * \param imagePath Absolute path to the source image file.
 * \param entries   The known-face cache entries to persist.
 */
void FaceRecognizer::saveKnownFaceCache(
    const QString &imagePath,
    const QVector<KnownFaceCacheEntry> &entries)
{
    QString cachePath = knownFaceCacheFilePath(imagePath);

    // Ensure the cache directory exists
    QDir().mkpath(QFileInfo(cachePath).dir().absolutePath());

    QJsonObject root;
    root["source_file_mtime"] = QFileInfo(imagePath).lastModified().toString(Qt::ISODate);

    QJsonArray facesArr;
    for (const KnownFaceCacheEntry &entry : entries) {
        QJsonObject faceObj;
        faceObj["tag_family"] = entry.tagFamily;
        faceObj["tag_name"]   = entry.tagName;

        QJsonArray rectArr;
        rectArr.append(entry.rect.x());
        rectArr.append(entry.rect.y());
        rectArr.append(entry.rect.width());
        rectArr.append(entry.rect.height());
        faceObj["rect"] = rectArr;

        QJsonArray embArr;
        for (long i = 0; i < entry.embedding.size(); ++i)
            embArr.append(static_cast<double>(entry.embedding(i)));
        faceObj["embedding"] = embArr;

        facesArr.append(faceObj);
    }
    root["known_faces"] = facesArr;

    QFile f(cachePath);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

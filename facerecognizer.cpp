#include "facerecognizer.h"
#include "constants.h"

#include <limits>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMutexLocker>
#include <QSet>
#include <QThread>
#include <QDebug>
#include <QtConcurrent>

static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};

// ---------------------------------------------------------------------------
// Internal helper: build the cache file path for a given image
// ---------------------------------------------------------------------------

/*! \brief Returns the absolute path of the face-descriptor cache file for \p imagePath.
 *
 * The cache lives at: {imageDir}/.compendia_cache/{baseName}-face-descriptors.json
 *
 * \param imagePath Absolute path to the source image.
 * \return Absolute path to the corresponding cache file.
 */
static QString descriptorCacheFilePath(const QString &imagePath)
{
    QFileInfo fi(imagePath);
    return fi.dir().absolutePath()
           + "/" + Compendia::CacheFolderName
           + "/" + fi.completeBaseName()
           + Compendia::FaceDescriptorCacheSuffix;
}

/*! \brief Returns the absolute path of the known-face embedding cache file for \p imagePath.
 *
 * The cache lives at: {imageDir}/.compendia_cache/{baseName}-known-faces.json
 *
 * \param imagePath Absolute path to the source image.
 * \return Absolute path to the corresponding cache file.
 */
static QString knownFaceCacheFilePath(const QString &imagePath)
{
    QFileInfo fi(imagePath);
    return fi.dir().absolutePath()
           + "/" + Compendia::CacheFolderName
           + "/" + fi.completeBaseName()
           + Compendia::KnownFaceCacheSuffix;
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

    static constexpr int kMaxDetectionDim = 1200;
    const int longEdge = qMax(img.width(), img.height());
    const QImage detectImg = (longEdge > kMaxDetectionDim)
        ? img.scaled(kMaxDetectionDim * img.width()  / longEdge,
                     kMaxDetectionDim * img.height() / longEdge,
                     Qt::IgnoreAspectRatio,
                     Qt::SmoothTransformation)
        : img;

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(detectImg);

    std::vector<dlib::frontal_face_detector::rect_detection> rawDets;
    detector_(dlibImg, rawDets, detectionThreshold_);

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
    if (chips.empty())
        return {};

    // Phase B: single batched ResNet forward pass over all chips at once.
    std::vector<dlib::matrix<float,0,1>> embeddings = net_(chips);

    // Phase C: pair each embedding with its normalised rect.
    QVector<QPair<QRectF, dlib::matrix<float,0,1>>> results;
    results.reserve(static_cast<int>(embeddings.size()));
    for (size_t i = 0; i < embeddings.size(); ++i)
        results.append({normRects[i], embeddings[i]});

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

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(detectImg);

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
void FaceRecognizer::warmupDescriptorCache(const QString &imagePath)
{
    if (loadDescriptorCache(imagePath).has_value())
        return;

    qDebug() << "[FaceRecognizer] cache miss (descriptor) — warming:" << QFileInfo(imagePath).fileName();

    QImageReader ir(imagePath);
    ir.setAutoTransform(true);
    QImage img = ir.read();
    if (img.isNull()) {
        qWarning() << "[FaceRecognizer] failed to load image for descriptor warmup:" << imagePath;
        return;
    }

    auto faces = detectFacesWithEmbeddings(img);
    qDebug() << "[FaceRecognizer] saving descriptor cache:" << QFileInfo(imagePath).fileName();
    saveDescriptorCache(imagePath, faces);
}

void FaceRecognizer::scheduleRectAdjustWarmup(TaggedFile* tf)
{
    if (!tf || !models_loaded_)
        return;

    if (rectWarmupFuture_.isRunning())
        return;

    const QString imagePath = tf->filePath + "/" + tf->fileName;

    // Extract tag regions on the main thread — TaggedFile must not be accessed
    // from the background task.
    QVector<std::tuple<QString, QString, QRectF>> regions;
    for (Tag* tag : *tf->tags()) {
        auto r = tf->tagRect(tag);
        if (r.has_value())
            regions.append({tag->tagFamily->getName(), tag->getName(), r.value()});
    }

    rectWarmupFuture_ = QtConcurrent::run([this, imagePath, regions]() {
        warmupDescriptorCache(imagePath);
        warmupKnownFaceEmbeddings(imagePath, regions);
    });
}

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
        if (tag->getName().startsWith(Compendia::AutoFaceTagPrefix))
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
// Face recognition sweep — background parallel implementation
// ---------------------------------------------------------------------------

// Internal per-embedding descriptor used in the coordinator thread only.
struct PendingDescriptor {
    dlib::matrix<float,0,1> embedding;
    QString tagFamily;
    QString tagName;
};

// Pool of FaceRecognizer instances, each owning its own dlib objects.
// Lives on the stack inside the coordinator lambda; destroyed when the sweep ends.
struct FaceWorkerPool {
    QMutex              mutex;
    QWaitCondition      cond;
    QList<FaceRecognizer*> available;
    QVector<FaceRecognizer*> all;

    FaceWorkerPool(const QString &modelsDir, int n) {
        all.reserve(n);
        for (int i = 0; i < n; ++i) {
            FaceRecognizer* w = new FaceRecognizer(nullptr);
            w->loadModels(modelsDir);
            all.append(w);
            available.append(w);
        }
    }

    ~FaceWorkerPool() {
        qDeleteAll(all);
    }

    FaceRecognizer* acquire() {
        QMutexLocker lk(&mutex);
        while (available.isEmpty())
            cond.wait(&mutex);
        return available.takeFirst();
    }

    void release(FaceRecognizer* w) {
        QMutexLocker lk(&mutex);
        available.append(w);
        cond.wakeOne();
    }
};

/*! \brief Sets the maximum number of parallel Phase 3 workers. */
void FaceRecognizer::setMaxWorkers(int n)
{
    maxWorkers_ = qBound(1, n, 8);
}

/*! \brief Returns true if a background sweep is currently running. */
bool FaceRecognizer::isSweepRunning() const
{
    return sweepThread_ != nullptr && sweepThread_->isRunning();
}

/*! \brief Requests cancellation of the active background sweep. */
void FaceRecognizer::cancelSweep()
{
    sweepCancelFlag_.storeRelaxed(1);
}

/*! \brief Launches a background face recognition sweep.
 *
 * Phase 1 (serial) seeds known-person embeddings on the coordinator thread.
 * Phase 3 (parallel) detects and matches faces using a per-worker FaceRecognizer pool.
 * Phase 2 (clear auto tags) must be done on the main thread before calling this.
 *
 * Results arrive via sweepFileResult() on the main thread (Qt::QueuedConnection).
 */
void FaceRecognizer::startBackgroundSweep(
    const QVector<Phase1FileInput> &phase1Input,
    const QStringList &phase3Paths,
    int numWorkers)
{
    if (isSweepRunning())
        return;

    sweepCancelFlag_.storeRelaxed(0);

    const int workerCount = (numWorkers > 0)
        ? qBound(1, numWorkers, maxWorkers_)
        : qBound(1, QThread::idealThreadCount(), maxWorkers_);

    const QString modelsDir = QCoreApplication::applicationDirPath() + "/models";
    const double  matchThr  = matchThreshold_;

    // Tell OpenBLAS/OpenMP to use 1 thread per dlib inference call so that
    // application-level worker parallelism (not BLAS-internal threads) owns
    // the cores.  Without this, each net_(chips) call grabs all nproc threads
    // and workers effectively serialize at the BLAS level.
#ifdef _OPENMP
    omp_set_num_threads(1);
#endif

    // Capture by value: phase1Input, phase3Paths are plain Qt value types (safe to copy).
    sweepThread_ = QThread::create([this, phase1Input, phase3Paths,
                                    workerCount, modelsDir, matchThr]()
    {
        // ---- Phase 1: seed known-face embeddings (serial, on coordinator thread) ----
        // We use THIS instance's dlib objects here, so we need the mutex.
        QVector<PendingDescriptor> knownFaces;

        const int phase1Total = phase1Input.size();
        if (phase1Total > 0)
            emit phase1Started(phase1Total);

        int phase1Done = 0;
        for (const Phase1FileInput &fileInput : phase1Input) {
            if (sweepCancelFlag_.loadRelaxed())
                break;

            if (fileInput.tagRegions.isEmpty()) {
                emit phase1Progress(++phase1Done, phase1Total);
                continue;
            }

            qDebug() << "[FaceRecognizer] processing (known-face):" << QFileInfo(fileInput.imagePath).fileName();

            auto cachedEntries = FaceRecognizer::loadKnownFaceCache(fileInput.imagePath);

            QVector<KnownFaceCacheEntry> updatedCache;
            bool cacheNeedsWrite = false;
            QImage img;

            for (const auto &[family, name, rect] : fileInput.tagRegions) {
                bool hitFound = false;
                if (cachedEntries.has_value()) {
                    for (const KnownFaceCacheEntry &entry : *cachedEntries) {
                        if (entry.tagFamily == family
                                && entry.tagName == name
                                && entry.rect   == rect) {
                            qDebug() << "[FaceRecognizer] cache hit (known-face):" << name;
                            knownFaces.append({entry.embedding, family, name});
                            updatedCache.append(entry);
                            hitFound = true;
                            break;
                        }
                    }
                }
                if (hitFound)
                    continue;

                if (img.isNull()) {
                    QImageReader ir(fileInput.imagePath);
                    ir.setAutoTransform(true);
                    img = ir.read();
                    if (img.isNull()) {
                        qWarning() << "[FaceRecognizer] failed to load image:" << fileInput.imagePath;
                        break;
                    }
                }

                qDebug() << "[FaceRecognizer] cache miss (known-face): computing embedding for:" << name;
                // Use this instance's dlib objects under the mutex.
                dlib::matrix<float,0,1> emb;
                {
                    QMutexLocker lock(&embeddingMutex_);
                    emb = computeEmbeddingFromRegionUnlocked(img, rect);
                }
                if (emb.size() == 0)
                    continue;

                knownFaces.append({emb, family, name});
                KnownFaceCacheEntry newEntry;
                newEntry.tagFamily = family;
                newEntry.tagName   = name;
                newEntry.rect      = rect;
                newEntry.embedding = emb;
                updatedCache.append(newEntry);
                cacheNeedsWrite = true;
            }

            if (cacheNeedsWrite) {
                qDebug() << "[FaceRecognizer] saving known-face cache:" << QFileInfo(fileInput.imagePath).fileName();
                FaceRecognizer::saveKnownFaceCache(fileInput.imagePath, updatedCache);
            }

            emit phase1Progress(++phase1Done, phase1Total);
        }

        emit sweepStarted(phase3Paths.size());

        if (sweepCancelFlag_.loadRelaxed()) {
            emit sweepCancelled();
            return;
        }

        // ---- Phase 3: parallel detection + matching ----
        FaceWorkerPool pool(modelsDir, workerCount);

        // Dedicated thread pool so face tasks don't compete with icon generation
        // (which also uses QThreadPool::globalInstance() via QtConcurrent).
        QThreadPool taskPool;
        taskPool.setMaxThreadCount(workerCount);

        // Shared state for auto-face counters and newly discovered auto-face embeddings.
        QMutex sharedMutex;
        QVector<PendingDescriptor> sharedKnownFaces = knownFaces;
        QAtomicInt autoFaceCounter {1};

        const int total = phase3Paths.size();
        QAtomicInt doneCount {0};

        for (const QString &imagePath : phase3Paths) {
            if (sweepCancelFlag_.loadRelaxed())
                break;

            (void)QtConcurrent::run(&taskPool, [this, imagePath, total, &pool, &sharedMutex,
                               &sharedKnownFaces, &autoFaceCounter, &doneCount,
                               matchThr]()
            {
                if (sweepCancelFlag_.loadRelaxed()) {
                    int d = ++doneCount;
                    emit sweepProgress(d, total);
                    return;
                }

                qDebug() << "[FaceRecognizer] processing:" << QFileInfo(imagePath).fileName();

                auto cached = FaceRecognizer::loadDescriptorCache(imagePath);
                QVector<QPair<QRectF, dlib::matrix<float,0,1>>> faces;

                if (cached.has_value()) {
                    qDebug() << "[FaceRecognizer] cache hit (descriptor):" << QFileInfo(imagePath).fileName();
                    faces = cached.value();
                } else {
                    qDebug() << "[FaceRecognizer] cache miss (descriptor):" << QFileInfo(imagePath).fileName();
                    QImageReader ir(imagePath);
                    ir.setAutoTransform(true);
                    QImage img = ir.read();
                    if (!img.isNull()) {
                        FaceRecognizer* worker = pool.acquire();
                        faces = worker->detectFacesWithEmbeddings(img);
                        pool.release(worker);
                        qDebug() << "[FaceRecognizer] saving descriptor cache:" << QFileInfo(imagePath).fileName();
                        FaceRecognizer::saveDescriptorCache(imagePath, faces);
                    }
                }

                // Match faces against known embeddings.
                QVector<FaceTagAssignment> assignments;
                QVector<KnownFaceCacheEntry> newKnownEntries; // user-named matches to persist
                int autoFacesThisImage = 0;

                for (const auto &[rect, embedding] : faces) {
                    double minDist = std::numeric_limits<double>::max();
                    QString bestFamily;
                    QString bestName;

                    {
                        QMutexLocker lk(&sharedMutex);
                        for (const PendingDescriptor &pd : sharedKnownFaces) {
                            double dist = dlib::length(pd.embedding - embedding);
                            if (dist < minDist) {
                                minDist   = dist;
                                bestFamily = pd.tagFamily;
                                bestName   = pd.tagName;
                            }
                        }
                    }

                    const bool isUserNamedMatch = minDist < matchThr
                                                  && !bestName.isEmpty()
                                                  && !bestName.startsWith(Compendia::AutoFaceTagPrefix);

                    if (isUserNamedMatch) {
                        assignments.append({bestFamily, bestName, rect});
                        // Accumulate for known-face cache so Phase 1 of the next
                        // sweep finds a hit instead of recomputing this embedding.
                        KnownFaceCacheEntry e;
                        e.tagFamily = bestFamily;
                        e.tagName   = bestName;
                        e.rect      = rect;
                        e.embedding = embedding;
                        newKnownEntries.append(e);
                    } else {
                        if (autoFacesThisImage >= Compendia::MaxAutoFacesPerImage)
                            continue;

                        if (minDist < matchThr && !bestName.isEmpty()) {
                            // Matched an existing auto-face tag
                            assignments.append({bestFamily, bestName, rect});
                        } else {
                            // New unknown face — claim a unique ID
                            int id = autoFaceCounter.fetchAndAddOrdered(1);
                            QString tagName = QString("%1%2")
                                .arg(Compendia::AutoFaceTagPrefix)
                                .arg(id, 2, 10, QChar('0'));
                            assignments.append({"People", tagName, rect});

                            // Register in shared known faces for future workers
                            PendingDescriptor newDesc;
                            newDesc.embedding = embedding;
                            newDesc.tagFamily = "People";
                            newDesc.tagName   = tagName;
                            QMutexLocker lk(&sharedMutex);
                            sharedKnownFaces.append(newDesc);
                        }
                        ++autoFacesThisImage;
                    }
                }

                // Persist user-named sweep matches to the known-face cache so
                // Phase 1 of the next sweep finds hits for these files.
                if (!newKnownEntries.isEmpty()) {
                    auto existing = FaceRecognizer::loadKnownFaceCache(imagePath);
                    QVector<KnownFaceCacheEntry> updatedKnown =
                        existing.has_value() ? *existing : QVector<KnownFaceCacheEntry>{};
                    for (const KnownFaceCacheEntry &e : newKnownEntries) {
                        const bool already = std::any_of(
                            updatedKnown.cbegin(), updatedKnown.cend(),
                            [&e](const KnownFaceCacheEntry &k) {
                                return k.tagFamily == e.tagFamily
                                    && k.tagName   == e.tagName
                                    && k.rect      == e.rect;
                            });
                        if (!already)
                            updatedKnown.append(e);
                    }
                    qDebug() << "[FaceRecognizer] saving known-face cache (sweep match):" << QFileInfo(imagePath).fileName();
                    FaceRecognizer::saveKnownFaceCache(imagePath, updatedKnown);
                }

                // Deliver result to main thread
                QMetaObject::invokeMethod(this,
                    [this, imagePath, assignments]() {
                        emit sweepFileResult(imagePath, assignments);
                    },
                    Qt::QueuedConnection);

                int d = ++doneCount;
                emit sweepProgress(d, total);
            });
        }

        // Wait for all tasks in the dedicated pool to complete.
        taskPool.waitForDone();

        if (sweepCancelFlag_.loadRelaxed())
            emit sweepCancelled();
        else
            emit sweepFinished();
    });

    connect(sweepThread_, &QThread::finished, this, [this]() {
        sweepThread_->deleteLater();
        sweepThread_ = nullptr;
    });

    sweepThread_->start();
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
    const QVector<KnownFaceCacheEntry> existingCache =
        cachedEntries.has_value() ? *cachedEntries : QVector<KnownFaceCacheEntry>{};

    // Rebuild the cache to match exactly the current tagRegions.
    // Carrying over only entries whose (family, name, rect) still exist ensures
    // that moved or deleted rects are dropped rather than left as stale entries.
    QImage img;  // loaded lazily on first cache miss
    bool cacheNeedsWrite = false;
    QVector<KnownFaceCacheEntry> newCache;
    newCache.reserve(tagRegions.size());

    for (const auto &[family, name, rect] : tagRegions) {
        // Look for a matching entry in the existing cache.
        bool hit = false;
        for (const KnownFaceCacheEntry &e : existingCache) {
            if (e.tagFamily == family && e.tagName == name && e.rect == rect) {
                newCache.append(e);
                hit = true;
                break;
            }
        }
        if (hit)
            continue;

        // Cache miss — compute the embedding for this region.
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
        newCache.append(e);
        cacheNeedsWrite = true;

        emit embeddingWarmedUp();
    }

    // Write if new embeddings were computed or stale entries were removed.
    if (cacheNeedsWrite || newCache.size() != existingCache.size())
        FaceRecognizer::saveKnownFaceCache(imagePath, newCache);
}

// ---------------------------------------------------------------------------
// Descriptor → known-face promotion
// ---------------------------------------------------------------------------

/*! \brief Promotes face embeddings from the descriptor cache into the known-face cache.
 *
 * Groups entries by image path, then for each image loads the descriptor cache
 * and the existing known-face cache.  For each entry whose rect matches a
 * descriptor (within a small epsilon), the embedding is copied into the
 * known-face cache under the entry's current tag name.  Skips silently if the
 * descriptor cache is missing or stale for an image.
 */
void FaceRecognizer::promoteDescriptorEmbeddings(const QVector<PromotionEntry> &entries)
{
    // Group entries by image path so we load each cache file at most once.
    QMap<QString, QVector<PromotionEntry>> byPath;
    for (const PromotionEntry &e : entries)
        byPath[e.imagePath].append(e);

    for (auto it = byPath.cbegin(); it != byPath.cend(); ++it) {
        const QString &imagePath = it.key();

        auto descCache = FaceRecognizer::loadDescriptorCache(imagePath);
        if (!descCache.has_value())
            continue;  // no descriptor cache for this image — skip silently

        auto knownCache = FaceRecognizer::loadKnownFaceCache(imagePath);
        QVector<KnownFaceCacheEntry> updatedKnown =
            knownCache.has_value() ? *knownCache : QVector<KnownFaceCacheEntry>{};

        bool changed = false;

        for (const PromotionEntry &entry : it.value()) {
            // Skip if this (family, name, rect) is already in the known-face cache.
            bool alreadyPresent = false;
            for (const KnownFaceCacheEntry &kf : updatedKnown) {
                if (kf.tagFamily == entry.tagFamily
                        && kf.tagName == entry.tagName
                        && kf.rect   == entry.tagRect) {
                    alreadyPresent = true;
                    break;
                }
            }
            if (alreadyPresent)
                continue;

            // Find the descriptor entry whose rect best matches the tag rect.
            // Both rects went through the same QJsonArray double serialization so
            // values should be identical; a small epsilon guards against any edge cases.
            const dlib::matrix<float,0,1>* bestEmb = nullptr;
            double bestDist = 1e-6;  // only accept a near-exact match

            for (const auto &[descRect, descEmb] : *descCache) {
                double dx = descRect.x()      - entry.tagRect.x();
                double dy = descRect.y()      - entry.tagRect.y();
                double dw = descRect.width()  - entry.tagRect.width();
                double dh = descRect.height() - entry.tagRect.height();
                double dist = dx*dx + dy*dy + dw*dw + dh*dh;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestEmb  = &descEmb;
                }
            }

            if (!bestEmb) {
                qDebug() << "[FaceRecognizer] promote: no descriptor match for" << entry.tagName
                         << "in" << QFileInfo(imagePath).fileName();
                continue;
            }

            KnownFaceCacheEntry newEntry;
            newEntry.tagFamily = entry.tagFamily;
            newEntry.tagName   = entry.tagName;
            newEntry.rect      = entry.tagRect;
            newEntry.embedding = *bestEmb;
            updatedKnown.append(newEntry);
            changed = true;
            qDebug() << "[FaceRecognizer] promote: promoted" << entry.tagName
                     << "in" << QFileInfo(imagePath).fileName();
        }

        if (changed)
            FaceRecognizer::saveKnownFaceCache(imagePath, updatedKnown);
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
 * The cache file is written into the .compendia_cache sub-directory next to
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
 * The cache file is written into the .compendia_cache sub-directory next to
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

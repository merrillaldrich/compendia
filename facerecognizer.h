#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <functional>
#include <optional>

#include <QAtomicInt>
#include <QFuture>
#include <QObject>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QDebug>
#include <QVector>
#include <QPair>
#include <QJsonArray>
#include <vector>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>
#include <dlib/image_processing/full_object_detection.h>
#include <dlib/image_transforms.h>
#include <dlib/pixel.h>
#include <dlib/array2d.h>
#include <dlib/dnn.h>

#include "tag.h"
#include "taggedfile.h"
#include "constants.h"

/*! \brief Standard dlib ResNet-29 face recognition network type (128-d embeddings).
 *
 * This is the architecture used by dlib's dlib_face_recognition_resnet_model_v1.dat.
 * Defined here following the canonical dlib face_recognition_ex example.
 */
namespace FaceNet {
    using namespace dlib;

    template <template <int,template<typename>class,int,typename> class block,
              int N, template<typename>class BN, typename SUBNET>
    using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

    template <template <int,template<typename>class,int,typename> class block,
              int N, template<typename>class BN, typename SUBNET>
    using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

    template <int N, template <typename> class BN, int stride, typename SUBNET>
    using block = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

    template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
    template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

    template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
    template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
    template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
    template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
    template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

    using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
        alevel0<
        alevel1<
        alevel2<
        alevel3<
        alevel4<
        max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
        input_rgb_image_sized<150>
        >>>>>>>>>>>>;
} // namespace FaceNet

/*! \brief A face embedding paired with the Tag it is associated with.
 *
 * Used during the recognition sweep to match detected faces against
 * known-person embeddings seeded from user-labeled regions.
 */
struct FaceDescriptor {
    dlib::matrix<float,0,1> embedding; /*!< 128-dimensional face embedding. */
    Tag* tag = nullptr;                /*!< The tag representing this person. */
};

/*! \brief One entry in the known-face embedding cache for a single source image.
 *
 * Cached alongside the source image's mtime so that Phase 1 of the face
 * recognition sweep can skip the expensive HOG + ResNet recomputation when
 * neither the image nor its labeled region has changed.
 */
struct KnownFaceCacheEntry {
    QString tagFamily;                 /*!< Tag family name — used as cache key. */
    QString tagName;                   /*!< Tag name — used as cache key. */
    QRectF  rect;                      /*!< Stored rect; mismatch triggers recompute. */
    dlib::matrix<float,0,1> embedding; /*!< The cached 128-d embedding. */
};

/*! \brief One entry for the descriptor→known-face promotion pass.
 *
 * Carries enough information to locate the embedding in the descriptor cache
 * and write it into the known-face cache under the tag's current name.
 * All fields are plain data — safe to copy across threads.
 */
struct PromotionEntry {
    QString imagePath; /*!< Absolute path to the source image. */
    QString tagFamily; /*!< Tag family name. */
    QString tagName;   /*!< Current (new) tag name. */
    QRectF  tagRect;   /*!< Normalized face bounding rect stored on the TaggedFile. */
};

/*! \brief A pending tag assignment from the background sweep — plain data, no Qt object pointers. */
struct FaceTagAssignment {
    QString family; /*!< Tag family name. */
    QString name;   /*!< Tag name. */
    QRectF  rect;   /*!< Normalized face bounding rect in image coordinates (0.0–1.0). */
};

/*! \brief Per-file input extracted from TaggedFile* on the main thread before a background sweep.
 *
 * Avoids cross-thread access to TaggedFile during Phase 1. */
struct Phase1FileInput {
    QString imagePath; /*!< Absolute path to the source image. */
    QVector<std::tuple<QString, QString, QRectF>> tagRegions; /*!< (family, name, rect) tuples. */
};

/*! \brief Detects and recognises faces across images using dlib's HOG detector and ResNet embeddings.
 *
 * Converts QImages to the dlib pixel format, runs the HOG-based frontal face detector,
 * aligns detected faces via a 5-point shape predictor, and computes 128-d embeddings
 * using a pre-trained ResNet model.  Results can be cached to disk to avoid redundant
 * computation on repeated scans.
 */
class FaceRecognizer : public QObject
{
    Q_OBJECT

private:
    bool models_loaded_ = false;                              ///< True after loadModels() succeeds.
    dlib::frontal_face_detector detector_;                    ///< HOG-based frontal face detector.
    dlib::shape_predictor sp_;                                ///< 5-point face landmark predictor for chip alignment.
    FaceNet::anet_type net_;                                  ///< ResNet face recognition network producing 128-d embeddings.
    double detectionThreshold_ = Luminism::FaceDetectionThreshold; ///< HOG adjust_threshold; higher reduces false positives.
    double matchThreshold_ = Luminism::FaceMatchThreshold; ///< Euclidean-distance threshold for recognising a known face.
    QFuture<void> warmupFuture_;     ///< Handle to the current background embedding warmup task.
    QFuture<void> rectWarmupFuture_; ///< Handle to the current background rect-adjust warmup task.
    QMutex embeddingMutex_; ///< Serialises all dlib inference calls; prevents concurrent access to non-thread-safe dlib models.
    QAtomicInt sweepCancelFlag_ {0}; ///< Set to 1 to request cancellation of the background sweep.
    QThread* sweepThread_ = nullptr; ///< The coordinator thread running the background sweep, or nullptr when idle.
    int maxWorkers_ = 4; ///< Maximum number of parallel Phase 3 workers.

    /*! \brief Computes a 128-d face embedding for a single detected face rectangle.
     *
     * Runs the 5-point shape predictor on \p rect, extracts a 150×150 aligned chip,
     * and passes it through the ResNet network.
     *
     * \param dlibImg The source image as a dlib rgb_pixel array.
     * \param rect    The face bounding rectangle returned by the HOG detector.
     * \return A 128-d column matrix, or an empty matrix on failure.
     */
    dlib::matrix<float,0,1> computeEmbedding(const dlib::array2d<dlib::rgb_pixel> &dlibImg,
                                              const dlib::rectangle &rect);

    /*! \brief Non-locking core of computeEmbeddingFromRegion, called under embeddingMutex_.
     *
     * Runs the HOG detector and picks the best-IoU detection against \p normalizedRect,
     * then delegates to computeEmbedding.  Must only be called while holding embeddingMutex_.
     *
     * \param img            The source image.
     * \param normalizedRect The user-drawn region in normalised image coordinates (0.0–1.0).
     * \return A 128-d embedding, or an empty matrix on failure.
     */
    dlib::matrix<float,0,1> computeEmbeddingFromRegionUnlocked(const QImage &img,
                                                                const QRectF &normalizedRect);

public:
    /*! \brief Converts a QImage to a dlib RGB pixel array for use with the detector.
     *
     * \param qimg The source QImage (any format; will be converted internally to RGB888).
     * \return A dlib array2d of rgb_pixel values.
     */
    dlib::array2d<dlib::rgb_pixel> qimageToDlibArray(const QImage &qimg);

    /*! \brief Constructs a FaceRecognizer.
     *
     * \param parent Optional Qt parent object.
     */
    explicit FaceRecognizer(QObject *parent = nullptr);

    /*! \brief Loads the shape predictor and face recognition model from \p modelsDir.
     *
     * Expects the following files in \p modelsDir:
     *   - shape_predictor_5_face_landmarks.dat
     *   - dlib_face_recognition_resnet_model_v1.dat
     *
     * \param modelsDir Absolute path to the directory containing the .dat files.
     * \return True on success; false if any file is missing or fails to deserialize.
     */
    bool loadModels(const QString &modelsDir);

    /*! \brief Returns true if the model files have been loaded successfully. */
    bool modelsLoaded() const;

    /*! \brief Returns the current HOG detector adjust_threshold. */
    double detectionThreshold() const;

    /*! \brief Sets the HOG detector adjust_threshold used during face detection.
     *
     * Higher values reduce false positives; lower values increase recall.
     * Takes effect on the next call to detectFaces(), detectFacesWithEmbeddings(),
     * or computeEmbeddingFromRegion().
     *
     * \param threshold The new adjust_threshold value.
     */
    void setDetectionThreshold(double threshold);

    /*! \brief Returns the current Euclidean-distance threshold for face matching. */
    double matchThreshold() const;

    /*! \brief Sets the Euclidean-distance threshold used when matching detected faces
     *         against known-person embeddings during a recognition sweep.
     *
     * Lower values require a closer match; higher values are more permissive.
     * Takes effect on the next call to runSweep().
     *
     * \param threshold The new match threshold value.
     */
    void setMatchThreshold(double threshold);

    /*! \brief Warms the descriptor cache for a single image if it is missing or stale.
     *
     * Runs HOG detection + ResNet inference on \p imagePath and writes the result
     * to the descriptor cache.  No-op if a valid cache already exists.
     * Safe to call from a background thread; acquires embeddingMutex_ internally.
     *
     * \param imagePath Absolute path to the source image.
     */
    void warmupDescriptorCache(const QString &imagePath);

    /*! \brief Schedules background warmup of both face caches for \p tf after a rect adjustment.
     *
     * Extracts the file path and tag regions from \p tf on the calling (main) thread,
     * then launches a QtConcurrent task that warms the descriptor cache followed by
     * the known-face cache.  Does nothing if models are not loaded or a rect warmup
     * is already running.
     *
     * \param tf The file whose caches need warming. May be nullptr.
     */
    void scheduleRectAdjustWarmup(TaggedFile* tf);

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
    void scheduleEmbeddingWarmup(TaggedFile* tf);

    /*! \brief Launches a background face recognition sweep.
     *
     * Phase 1 seeds known-person embeddings (serial, coordinator thread).
     * Phase 3 detects + matches faces in parallel via a per-worker FaceRecognizer pool.
     * Phase 2 (clear auto tags) must be done on the main thread *before* calling this.
     *
     * Results arrive via sweepFileResult() on the main thread (Qt::QueuedConnection).
     * Completion is signalled by sweepFinished() or sweepCancelled().
     *
     * \param phase1Input   Per-file known-face data extracted from TaggedFile*s on the main thread.
     * \param phase3Paths   Image file paths to scan (video files excluded by the caller).
     * \param numWorkers    Number of parallel workers (0 = auto: idealThreadCount(), capped at maxWorkers_).
     */
    void startBackgroundSweep(
        const QVector<Phase1FileInput> &phase1Input,
        const QStringList &phase3Paths,
        int numWorkers = 0);

    /*! \brief Requests cancellation of the active background sweep.  No-op if none is running. */
    void cancelSweep();

    /*! \brief Returns true if a background sweep is currently running. */
    bool isSweepRunning() const;

    /*! \brief Sets the maximum number of parallel Phase 3 workers.
     *
     * \param n Maximum worker count (clamped to [1, 8]).
     */
    void setMaxWorkers(int n);

    /*! \brief Promotes face embeddings from the descriptor cache into the known-face cache.
     *
     * For each entry, looks up the descriptor cache for the image, finds the detection
     * whose rect best matches \c entry.tagRect, and writes that embedding into the
     * known-face cache under \c entry.tagFamily / \c entry.tagName.  No dlib inference
     * is performed — this is pure JSON read/write and is safe to call from any thread.
     *
     * Intended to be called after a tag is renamed so that Phase 1 of the next sweep
     * finds cache hits instead of falling back to expensive embedding recomputation.
     *
     * \param entries Per-file promotion entries, typically all files carrying the renamed tag.
     */
    static void promoteDescriptorEmbeddings(const QVector<PromotionEntry> &entries);

    /*! \brief Runs face detection on the source image and returns normalised bounding rectangles.
     *
     * \param sourceImage The image to run face detection on.
     * \return A list of QRectF values, one per detected face, in normalised image coordinates (0.0–1.0).
     */
    QList<QRectF> detectFaces(const QImage &sourceImage);

    /*! \brief Detects faces in \p img and returns each face's normalised rect and embedding.
     *
     * \param img The source image.
     * \return A vector of (normalised QRectF, 128-d embedding) pairs, one per detected face.
     */
    QVector<QPair<QRectF, dlib::matrix<float,0,1>>> detectFacesWithEmbeddings(const QImage &img);

    /*! \brief Computes a face embedding for a user-drawn region in \p img.
     *
     * Runs the HOG detector and picks the detection with the best IoU against \p normalizedRect.
     * Falls back to a synthetic rectangle from \p normalizedRect if no detection has IoU > 0.3.
     *
     * \param img            The source image.
     * \param normalizedRect The user-drawn region in normalised image coordinates (0.0–1.0).
     * \return A 128-d embedding, or an empty matrix if the embedding could not be computed.
     */
    dlib::matrix<float,0,1> computeEmbeddingFromRegion(const QImage &img, const QRectF &normalizedRect);

    /*! \brief Checks the known-face cache for each region in \p tagRegions and computes
     *         any missing embeddings, saving the updated cache to disk when done.
     *
     * Intended to be called from a background thread (e.g. via QtConcurrent::run).
     * Uses an internal mutex to prevent concurrent dlib inference from multiple threads.
     * Emits embeddingWarmedUp() after each successfully computed (not cached) embedding.
     *
     * \param imagePath  Absolute path to the source image.
     * \param tagRegions List of (tagFamilyName, tagName, normalizedRect) tuples for
     *                   each user-labeled face region on this file.
     */
    void warmupKnownFaceEmbeddings(
        const QString &imagePath,
        const QVector<std::tuple<QString, QString, QRectF>> &tagRegions);

    /*! \brief Attempts to load cached face descriptors for \p imagePath.
     *
     * The cache is considered valid only when the stored source_file_mtime matches
     * the image file's current last-modified timestamp.
     *
     * \param imagePath Absolute path to the source image file.
     * \return The cached descriptors on a cache hit; std::nullopt on a miss or stale cache.
     */
    static std::optional<QVector<QPair<QRectF, dlib::matrix<float,0,1>>>>
        loadDescriptorCache(const QString &imagePath);

    /*! \brief Writes face descriptors for \p imagePath to the descriptor cache.
     *
     * The cache file is written into the .luminism_cache sub-directory next to
     * the image file. The directory is created if it does not exist.
     *
     * \param imagePath   Absolute path to the source image file.
     * \param descriptors The (normalised rect, embedding) pairs to cache.
     */
    static void saveDescriptorCache(const QString &imagePath,
                                    const QVector<QPair<QRectF, dlib::matrix<float,0,1>>> &descriptors);

    /*! \brief Attempts to load the known-face embedding cache for \p imagePath.
     *
     * The cache is considered valid only when the stored source_file_mtime matches
     * the image file's current last-modified timestamp.  On a mismatch the entire
     * cache for that file is considered stale.
     *
     * \param imagePath Absolute path to the source image file.
     * \return The cached entries on a cache hit; std::nullopt on a miss or stale cache.
     */
    static std::optional<QVector<KnownFaceCacheEntry>>
        loadKnownFaceCache(const QString &imagePath);

    /*! \brief Writes known-face embedding entries for \p imagePath to the cache.
     *
     * The cache file is written into the .luminism_cache sub-directory next to
     * the image file. The directory is created if it does not exist.
     *
     * \param imagePath Absolute path to the source image file.
     * \param entries   The known-face cache entries to persist.
     */
    static void saveKnownFaceCache(const QString &imagePath,
                                   const QVector<KnownFaceCacheEntry> &entries);

signals:
    /*! \brief Emitted once per embedding that was computed (cache miss) during warmup. */
    void embeddingWarmedUp();

    /*! \brief Emitted by scheduleEmbeddingWarmup() just before the background task launches.
     *
     * \param misses Number of embeddings that need to be (re)computed.
     */
    void warmupScheduled(int misses);

    /*! \brief Emitted once per file processed during Phase 3.
     *
     * \param filesProcessed Number of files processed so far.
     * \param totalFiles     Total number of target files in the sweep.
     */
    void sweepProgress(int filesProcessed, int totalFiles);

    /*! \brief Emitted before Phase 1 begins, only when there are files to process.
     *
     * \param totalFiles Number of files whose known-face embeddings need seeding.
     */
    void phase1Started(int totalFiles);

    /*! \brief Emitted after each file is processed during Phase 1.
     *
     * \param done  Files processed so far.
     * \param total Total files in Phase 1.
     */
    void phase1Progress(int done, int total);

    /*! \brief Emitted when the background sweep has started (Phase 3).
     *
     * \param totalFiles Total number of files to process in Phase 3.
     */
    void sweepStarted(int totalFiles);

    /*! \brief Emitted from the coordinator thread when a file has been processed.
     *
     * Delivered via Qt::QueuedConnection so the slot runs on the main thread.
     * \param filePath    Absolute path to the processed file.
     * \param assignments Tag assignments detected for this file.
     */
    void sweepFileResult(QString filePath, QVector<FaceTagAssignment> assignments);

    /*! \brief Emitted when all files have been processed successfully. */
    void sweepFinished();

    /*! \brief Emitted when the sweep was cancelled before all files were processed. */
    void sweepCancelled();
};

#endif // FACERECOGNIZER_H

#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <optional>

#include <QObject>
#include <QImage>
#include <QList>
#include <QRect>
#include <QString>
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
    bool models_loaded_ = false;
    dlib::frontal_face_detector detector_;
    dlib::shape_predictor sp_;
    FaceNet::anet_type net_;

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

signals:
};

#endif // FACERECOGNIZER_H

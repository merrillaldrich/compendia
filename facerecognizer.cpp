#include "facerecognizer.h"

/*! \brief Constructs a FaceRecognizer.
 *
 * \param parent Optional Qt parent object.
 */
FaceRecognizer::FaceRecognizer(QObject *parent)
    : QObject{parent}
{}

/*! \brief Runs face detection on the source image and returns normalised bounding rectangles.
 *
 * \param sourceImage The image to run face detection on.
 * \return A list of QRectF values, one per detected face, in normalised image coordinates (0.0–1.0).
 */
QList<QRectF> FaceRecognizer::detectFaces(const QImage &sourceImage){

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(sourceImage);

    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();

    qDebug() << "Start face detection";
    std::vector<dlib::rectangle> faces = detector(dlibImg);
    qDebug() << "End face detection";

    const double w = sourceImage.width();
    const double h = sourceImage.height();

    QList<QRectF> rects;
    for (const auto &face : faces)
        rects.append(QRectF(face.left() / w, face.top() / h,
                            face.width() / w, face.height() / h));

    return rects;
}

/*! \brief Converts a QImage to a dlib RGB pixel array for use with the detector.
 *
 * \param qimg The source QImage (any format; will be converted internally to RGB888).
 * \return A dlib array2d of rgb_pixel values.
 */
dlib::array2d<dlib::rgb_pixel> FaceRecognizer::qimageToDlibArray(const QImage &qimg) {

    QImage img = qimg.convertToFormat(QImage::Format_RGB888);

    dlib::array2d<dlib::rgb_pixel> dlibImage;

    dlibImage.set_size(img.height(), img.width());
    for (int y = 0; y < img.height(); ++y) {
        const uchar *line = img.constScanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            dlib::rgb_pixel pixel(line[x * 3], line[x * 3 + 1], line[x * 3 + 2]);
            dlibImage[y][x] = pixel;
        }
    }
    return dlibImage;
}



#include "facerecognizer.h"

/*! \brief Constructs a FaceRecognizer.
 *
 * \param parent Optional Qt parent object.
 */
FaceRecognizer::FaceRecognizer(QObject *parent)
    : QObject{parent}
{}

/*! \brief Returns a copy of the source image with face bounding boxes drawn on it.
 *
 * \param sourceImage The image to run face detection on.
 * \return A copy of the source image annotated with green face rectangles.
 */
QImage FaceRecognizer::imageWithFaceBoxes(QImage sourceImage){

    dlib::array2d<dlib::rgb_pixel> dlibImg = qimageToDlibArray(sourceImage);

    // Create face detector
    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();

    // Detect faces
    qDebug() << "Start face detection";
    std::vector<dlib::rectangle> faces = detector(dlibImg);
    qDebug() << "End face detection";

    drawFaceBoxes(sourceImage, faces);

    return sourceImage;
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

/*! \brief Draws green bounding-box rectangles for each detected face onto a QImage.
 *
 * \param img   The image to annotate (modified in place).
 * \param faces A vector of dlib rectangle structures describing detected faces.
 */
void FaceRecognizer::drawFaceBoxes(QImage &img, const std::vector<dlib::rectangle> &faces) {
    QPainter painter(&img);
    painter.setPen(QPen(Qt::green, 2));
    for (const auto &face : faces) {
        QRect rect(face.left(), face.top(),
                   face.width(), face.height());
        painter.drawRect(rect);
    }
    painter.end();
}

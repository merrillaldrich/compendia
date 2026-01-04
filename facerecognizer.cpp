#include "facerecognizer.h"

FaceRecognizer::FaceRecognizer(QObject *parent)
    : QObject{parent}
{}


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

// Convert QImage (Format_RGB888) to dlib::array2d<rgb_pixel>
dlib::array2d<dlib::rgb_pixel> FaceRecognizer::qimageToDlibArray(const QImage &qimg) {

    //if (qimg.format() != QImage::Format_RGB888) {
    //    qDebug() << "Converting QImage to RGB888 format";
    //}

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

// Draw rectangles on QImage
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


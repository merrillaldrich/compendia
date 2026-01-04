#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <QObject>

#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QString>
#include <QDebug>
#include <vector>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/pixel.h>
#include <dlib/array2d.h>

class FaceRecognizer : public QObject
{
    Q_OBJECT

private:
    dlib::array2d<dlib::rgb_pixel> qimageToDlibArray(const QImage &qimg);
    void drawFaceBoxes(QImage &img, const std::vector<dlib::rectangle> &faces);

public:
    explicit FaceRecognizer(QObject *parent = nullptr);
    QImage imageWithFaceBoxes(QImage sourceImage);

signals:
};

#endif // FACERECOGNIZER_H

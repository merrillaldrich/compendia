#ifndef FACERECOGNIZER_H
#define FACERECOGNIZER_H

#include <QObject>

#include <QImage>
#include <QList>
#include <QRect>
#include <QString>
#include <QDebug>
#include <vector>

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/pixel.h>
#include <dlib/array2d.h>

/*! \brief Detects faces in a QImage using the dlib frontal-face detector.
 *
 * Converts a QImage to the dlib pixel format and runs the HOG-based frontal
 * face detector, returning a list of bounding rectangles for each detected face.
 */
class FaceRecognizer : public QObject
{
    Q_OBJECT

private:
    /*! \brief Converts a QImage to a dlib RGB pixel array for use with the detector.
     *
     * \param qimg The source QImage (any format; will be converted internally to RGB888).
     * \return A dlib array2d of rgb_pixel values.
     */
    dlib::array2d<dlib::rgb_pixel> qimageToDlibArray(const QImage &qimg);

public:
    /*! \brief Constructs a FaceRecognizer.
     *
     * \param parent Optional Qt parent object.
     */
    explicit FaceRecognizer(QObject *parent = nullptr);

    /*! \brief Runs face detection on the source image and returns normalised bounding rectangles.
     *
     * \param sourceImage The image to run face detection on.
     * \return A list of QRectF values, one per detected face, in normalised image coordinates (0.0–1.0).
     */
    QList<QRectF> detectFaces(const QImage &sourceImage);

signals:
};

#endif // FACERECOGNIZER_H

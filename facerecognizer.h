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

/*! \brief Detects faces in a QImage using the dlib frontal-face detector.
 *
 * Converts a QImage to the dlib pixel format, runs the HOG-based frontal face
 * detector, and can return an annotated copy of the image with bounding boxes
 * drawn around each detected face.
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

    /*! \brief Draws green bounding-box rectangles for each detected face onto a QImage.
     *
     * \param img   The image to annotate (modified in place).
     * \param faces A vector of dlib rectangle structures describing detected faces.
     */
    void drawFaceBoxes(QImage &img, const std::vector<dlib::rectangle> &faces);

public:
    /*! \brief Constructs a FaceRecognizer.
     *
     * \param parent Optional Qt parent object.
     */
    explicit FaceRecognizer(QObject *parent = nullptr);

    /*! \brief Returns a copy of the source image with face bounding boxes drawn on it.
     *
     * \param sourceImage The image to run face detection on.
     * \return A copy of the source image annotated with green face rectangles.
     */
    QImage imageWithFaceBoxes(QImage sourceImage);

signals:
};

#endif // FACERECOGNIZER_H

#ifndef FACERECOGNITIONSETTINGSDIALOG_H
#define FACERECOGNITIONSETTINGSDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QLabel>

/*! \brief Modal dialog for adjusting face recognition thresholds at runtime.
 *
 * Exposes two tuneable parameters:
 *   - Detection threshold: the HOG detector's adjust_threshold (higher = fewer false positives).
 *   - Match threshold: maximum L2 distance for two embeddings to be considered the same person.
 *
 * Both thresholds are presented as labelled sliders with live value readouts.
 * The dialog is built programmatically — no .ui file.
 */
class FaceRecognitionSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /*! \brief Constructs the dialog, initialising sliders from the current threshold values.
     *
     * \param detectionThreshold The current HOG adjust_threshold (0.0–3.0 range expected).
     * \param matchThreshold     The current face-match L2 distance threshold (0.1–1.0 range expected).
     * \param parent             Optional Qt parent widget.
     */
    explicit FaceRecognitionSettingsDialog(double detectionThreshold,
                                           double matchThreshold,
                                           QWidget *parent = nullptr);

    /*! \brief Returns the detection threshold selected by the user (0.0–3.0). */
    double detectionThreshold() const;

    /*! \brief Returns the match threshold selected by the user (0.1–1.0). */
    double matchThreshold() const;

private:
    QSlider *detectionSlider_;     /*!< Slider for the HOG detection threshold (0–30 → 0.0–3.0). */
    QSlider *matchSlider_;         /*!< Slider for the face-match threshold (1–10 → 0.1–1.0). */
    QLabel  *detectionValueLabel_; /*!< Live readout of the current detection threshold value. */
    QLabel  *matchValueLabel_;     /*!< Live readout of the current match threshold value. */
};

#endif // FACERECOGNITIONSETTINGSDIALOG_H

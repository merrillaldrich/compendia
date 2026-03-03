#include "facerecognitionsettingsdialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSlider>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

/*! \brief Constructs the dialog, initialising sliders from the current threshold values.
 *
 * Slider integer ranges map to double thresholds as follows:
 *   - Detection: slider 0–30  →  threshold 0.0–3.0  (divide by 10)
 *   - Match:     slider 1–10  →  threshold 0.1–1.0  (divide by 10)
 *
 * \param detectionThreshold The current HOG adjust_threshold.
 * \param matchThreshold     The current face-match L2 distance threshold.
 * \param parent             Optional Qt parent widget.
 */
FaceRecognitionSettingsDialog::FaceRecognitionSettingsDialog(double detectionThreshold,
                                                             double matchThreshold,
                                                             QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Face Recognition Settings"));

    // --- Detection threshold slider ---
    detectionSlider_ = new QSlider(Qt::Horizontal, this);
    detectionSlider_->setRange(0, 30);
    detectionSlider_->setValue(static_cast<int>(detectionThreshold * 10.0));

    detectionValueLabel_ = new QLabel(
        QString::number(detectionSlider_->value() / 10.0, 'f', 1), this);
    detectionValueLabel_->setMinimumWidth(30);

    connect(detectionSlider_, &QSlider::valueChanged, this, [this](int v) {
        detectionValueLabel_->setText(QString::number(v / 10.0, 'f', 1));
    });

    auto *detectionRow = new QHBoxLayout;
    detectionRow->addWidget(detectionSlider_);
    detectionRow->addWidget(detectionValueLabel_);

    // --- Match threshold slider ---
    matchSlider_ = new QSlider(Qt::Horizontal, this);
    matchSlider_->setRange(1, 10);
    matchSlider_->setValue(static_cast<int>(matchThreshold * 10.0));

    matchValueLabel_ = new QLabel(
        QString::number(matchSlider_->value() / 10.0, 'f', 1), this);
    matchValueLabel_->setMinimumWidth(30);

    connect(matchSlider_, &QSlider::valueChanged, this, [this](int v) {
        matchValueLabel_->setText(QString::number(v / 10.0, 'f', 1));
    });

    auto *matchRow = new QHBoxLayout;
    matchRow->addWidget(matchSlider_);
    matchRow->addWidget(matchValueLabel_);

    // --- Layout ---
    auto *form = new QFormLayout;
    form->addRow(tr("Detection Threshold"), detectionRow);
    form->addRow(new QLabel(tr("Higher values reduce false positives (trees, buildings)."), this));
    form->addRow(tr("Match Threshold"), matchRow);
    form->addRow(new QLabel(tr("Lower values require closer face similarity to match a known person."), this));

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *root = new QVBoxLayout(this);
    root->addLayout(form);
    root->addWidget(buttons);
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

/*! \brief Returns the detection threshold selected by the user (0.0–3.0). */
double FaceRecognitionSettingsDialog::detectionThreshold() const
{
    return detectionSlider_->value() / 10.0;
}

/*! \brief Returns the match threshold selected by the user (0.1–1.0). */
double FaceRecognitionSettingsDialog::matchThreshold() const
{
    return matchSlider_->value() / 10.0;
}

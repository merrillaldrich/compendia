#include "mapsettingsdialog.h"
#include "constants.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

MapSettingsDialog::MapSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Map Service Settings"));
    setMinimumWidth(500);

    QSettings s(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
    const bool storedFreeTier = s.value(Compendia::MapUseFreeTierSettingsKey, false).toBool();

    // --- Free-tier checkbox ---
    freeTierCheck_ = new QCheckBox(tr("Use Compendia free geo lookup"), this);
    freeTierCheck_->setChecked(storedFreeTier);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    // --- Tile URL ---
    tileUrlEdit_ = new QLineEdit(this);
    const QString storedUrl = s.value(Compendia::MapTileUrlSettingsKey).toString();
    tileUrlEdit_->setText(storedUrl.isEmpty()
                          ? QString::fromLatin1(Compendia::MapboxTileUrlTemplate)
                          : storedUrl);

    auto *urlHelp = new QLabel(tr("Use {z}, {x}, {y}, and {token} placeholders."), this);
    QFont helpFont = urlHelp->font();
    helpFont.setPointSizeF(helpFont.pointSizeF() * 0.85);
    urlHelp->setFont(helpFont);

    // --- API token ---
    tokenEdit_ = new QLineEdit(this);
    tokenEdit_->setEchoMode(QLineEdit::Password);
    tokenEdit_->setText(s.value(Compendia::MapApiTokenSettingsKey).toString());

    showHideBtn_ = new QPushButton(tr("Show"), this);
    showHideBtn_->setCheckable(true);
    connect(showHideBtn_, &QPushButton::toggled, this, [this](bool checked) {
        tokenEdit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        showHideBtn_->setText(checked ? tr("Hide") : tr("Show"));
    });

    auto *tokenRow = new QHBoxLayout;
    tokenRow->addWidget(tokenEdit_);
    tokenRow->addWidget(showHideBtn_);

    // --- Form ---
    auto *form = new QFormLayout;
    form->addRow(tr("Provider tile URL template:"), tileUrlEdit_);
    form->addRow(QString(), urlHelp);
    form->addRow(tr("API token:"), tokenRow);

    // --- Enable/disable wiring ---
    auto updateEnabled = [this](bool freeTier) {
        tileUrlEdit_->setEnabled(!freeTier);
        tokenEdit_->setEnabled(!freeTier);
        showHideBtn_->setEnabled(!freeTier);
    };
    connect(freeTierCheck_, &QCheckBox::toggled, this, updateEnabled);
    updateEnabled(storedFreeTier);

    // --- Buttons ---
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        QSettings s2(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
        s2.setValue(Compendia::MapUseFreeTierSettingsKey, freeTierCheck_->isChecked());
        s2.setValue(Compendia::MapTileUrlSettingsKey,     tileUrlEdit_->text().trimmed());
        s2.setValue(Compendia::MapApiTokenSettingsKey,    tokenEdit_->text().trimmed());
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // --- Root layout ---
    auto *root = new QVBoxLayout(this);
    root->addWidget(freeTierCheck_);
    root->addWidget(separator);
    root->addLayout(form);
    root->addWidget(buttons);
}

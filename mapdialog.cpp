#include "mapdialog.h"
#include "constants.h"

#include <cmath>

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

MapDialog::MapDialog(double lat, double lon, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Photo Location"));
    setMinimumSize(768, 624);
    resize(768, 624);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(8, 8, 8, 8);

    // Coordinate label
    const QString ns = (lat >= 0.0) ? QStringLiteral("N") : QStringLiteral("S");
    const QString ew = (lon >= 0.0) ? QStringLiteral("E") : QStringLiteral("W");
    coordLabel_ = new QLabel(
        QStringLiteral("%1°%2,  %3°%4")
            .arg(std::abs(lat), 0, 'f', 5).arg(ns)
            .arg(std::abs(lon), 0, 'f', 5).arg(ew),
        this);
    coordLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(coordLabel_);

    // Interactive map widget
    mapWidget_ = new MapWidget(/*interactive=*/true, this);
    mapWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapWidget_->setZoomLevel(Compendia::MapOverlayZoom);
    mapWidget_->setLocation(lat, lon);
    layout->addWidget(mapWidget_);

    // Close button
    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(bbox);
}

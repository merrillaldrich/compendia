#include "geofilterdialog.h"
#include "compendiacore.h"
#include "geo.h"
#include "taggedfile.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QStandardItemModel>
#include <QVBoxLayout>

GeoFilterDialog::GeoFilterDialog(CompendiaCore *core, QWidget *parent)
    : QDialog(parent)
    , core_(core)
{
    setWindowTitle(tr("Geo Filter"));
    setMinimumSize(768, 650);

    // --- Collect GPS coordinates from all files in the model ---
    QStandardItemModel *model = core_->getItemModel();
    for (int r = 0; r < model->rowCount(); ++r) {
        TaggedFile *tf = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;
        auto coords = Geo::parseGpsCoordinates(tf->exifMap());
        if (coords)
            geoPoints_.append(*coords);  // QPointF(lat, lon)
    }

    // --- Layout ---
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(8, 8, 8, 8);

    statusLabel_ = new QLabel(this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel_);

    mapWidget_ = new MapWidget(true /*interactive*/, this);
    mapWidget_->setMinimumHeight(500);
    layout->addWidget(mapWidget_, 1);

    auto *buttonBox = new QDialogButtonBox(this);
    auto *applyBtn  = buttonBox->addButton(tr("Filter Images to This Area"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(QDialogButtonBox::Close);
    connect(applyBtn,  &QPushButton::clicked,    this, &GeoFilterDialog::applyGeoFilter);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (geoPoints_.isEmpty()) {
        statusLabel_->setText(tr("No photos with location data found in this folder."));
        applyBtn->setEnabled(false);
        return;
    }

    // --- Auto-fit: compute bounding box and find a zoom level that shows all points ---
    double minLat =  90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;
    for (const QPointF &pt : geoPoints_) {
        minLat = qMin(minLat, pt.x());
        maxLat = qMax(maxLat, pt.x());
        minLon = qMin(minLon, pt.y());
        maxLon = qMax(maxLon, pt.y());
    }

    const double centerLat = (minLat + maxLat) / 2.0;
    const double centerLon = (minLon + maxLon) / 2.0;

    // Descend from zoom 15 until the bbox fits in 80% of the map area.
    // Use the minimum height (500 px) as the proxy since the widget isn't sized yet.
    const double targetPx = mapWidget_->minimumHeight() * 0.8;
    int fitZoom = 13;
    if (geoPoints_.size() > 1) {
        for (int z = 15; z >= 1; --z) {
            const QPointF tl = Geo::latLonToPixel(maxLat, minLon, z);
            const QPointF br = Geo::latLonToPixel(minLat, maxLon, z);
            if ((br.x() - tl.x()) <= targetPx && (br.y() - tl.y()) <= targetPx) {
                fitZoom = z;
                break;
            }
        }
    }

    mapWidget_->setGeoPoints(geoPoints_);
    mapWidget_->setLocation(centerLat, centerLon);
    mapWidget_->setZoomLevel(fitZoom);

    connect(mapWidget_, &MapWidget::viewportChanged, this, &GeoFilterDialog::updateStatusLabel);
    updateStatusLabel();
}

void GeoFilterDialog::updateStatusLabel()
{
    if (geoPoints_.isEmpty()) {
        statusLabel_->setText(tr("No photos with location data found in this folder."));
        return;
    }

    const MapWidget::ViewportBounds b = mapWidget_->viewportBounds();
    int visible = 0;
    for (const QPointF &pt : geoPoints_) {
        if (pt.x() >= b.minLat && pt.x() <= b.maxLat &&
            pt.y() >= b.minLon && pt.y() <= b.maxLon)
            ++visible;
    }
    statusLabel_->setText(tr("%1 of %2 geolocated photos visible in current map view")
                          .arg(visible).arg(geoPoints_.size()));
}

void GeoFilterDialog::applyGeoFilter()
{
    const MapWidget::ViewportBounds b = mapWidget_->viewportBounds();

    QSet<TaggedFile*> files;
    QStandardItemModel *model = core_->getItemModel();
    for (int r = 0; r < model->rowCount(); ++r) {
        TaggedFile *tf = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;
        auto coords = Geo::parseGpsCoordinates(tf->exifMap());
        if (!coords) continue;
        const double lat = coords->x();
        const double lon = coords->y();
        if (lat >= b.minLat && lat <= b.maxLat && lon >= b.minLon && lon <= b.maxLon)
            files.insert(tf);
    }

    if (files.isEmpty()) {
        QMessageBox::information(this, tr("Geo Filter"),
            tr("No photos found in the current map view.\n"
               "Try zooming out or panning to an area with photos."));
        return;
    }

    core_->setIsolationSet(files);
    accept();
}

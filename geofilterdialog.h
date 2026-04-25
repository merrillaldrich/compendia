#ifndef GEOFILTERDIALOG_H
#define GEOFILTERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPointF>
#include <QVector>

#include "mapwidget.h"

class CompendiaCore;

/*! \brief Dialog showing all geolocated photos as a dot overlay on an interactive map.
 *
 * Collects GPS coordinates from every file in the model, renders them as blue dots
 * on a zoomable/pannable MapWidget, and exposes an "Apply Geo Filter" button that
 * isolates only the files whose coordinates fall within the current viewport.
 *
 * Integrates with the existing isolation-set mechanism: accepting the dialog calls
 * CompendiaCore::setIsolationSet(), just like Isolate Selection.
 */
class GeoFilterDialog : public QDialog
{
    Q_OBJECT

public:
    /*! \brief Constructs the dialog, collects GPS points from the model, and auto-fits the map.
     *
     * \param core   The application's central controller (provides model access and isolation).
     * \param parent Optional Qt parent widget.
     */
    explicit GeoFilterDialog(CompendiaCore *core, QWidget *parent = nullptr);

private slots:
    /*! \brief Builds the isolation set from files whose GPS coords are inside the current viewport
     *         and calls core->setIsolationSet(), then accepts the dialog. */
    void applyGeoFilter();

    /*! \brief Recomputes how many photo dots are visible in the current viewport and updates the label. */
    void updateStatusLabel();

private:
    CompendiaCore    *core_;
    MapWidget        *mapWidget_;
    QLabel           *statusLabel_;
    QVector<QPointF>  geoPoints_; ///< (lat, lon) for every file that has GPS data; x=lat, y=lon.
};

#endif // GEOFILTERDIALOG_H

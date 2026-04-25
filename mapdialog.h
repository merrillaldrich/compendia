#ifndef MAPDIALOG_H
#define MAPDIALOG_H

#include <QDialog>
#include <QLabel>

#include "mapwidget.h"

/*! \brief Modal dialog displaying a zoomable, pannable OpenStreetMap for a photo location.
 *
 * Shows a map centered at the photo's GPS coordinates with a red marker.
 * The user can drag to pan and use the scroll wheel to zoom.
 * Built entirely programmatically (no .ui file), following the same pattern
 * as FaceRecognitionSettingsDialog.
 */
class MapDialog : public QDialog
{
    Q_OBJECT

public:
    /*! \brief Constructs the MapDialog and displays the location at (lat, lon).
     *
     * \param lat    Latitude of the photo location in decimal degrees.
     * \param lon    Longitude of the photo location in decimal degrees.
     * \param parent Optional Qt parent widget.
     */
    MapDialog(double lat, double lon, QWidget *parent = nullptr);

private:
    MapWidget *mapWidget_;
    QLabel    *coordLabel_;
};

#endif // MAPDIALOG_H

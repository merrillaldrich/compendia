#ifndef MAPSETTINGSDIALOG_H
#define MAPSETTINGSDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

/*! \brief Modal dialog for configuring the map service provider.
 *
 * Users choose between the Compendia free-tier proxy and a bring-your-own
 * Mapbox token. Both choices are persisted to QSettings on OK; Cancel
 * discards all changes. The dialog is built programmatically — no .ui file.
 */
class MapSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /*! \brief Constructs the dialog, reading current settings from QSettings.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit MapSettingsDialog(QWidget *parent = nullptr);

private:
    QCheckBox   *freeTierCheck_; /*!< Enables the Compendia free-tier proxy when checked. */
    QLineEdit   *tileUrlEdit_;   /*!< Editable tile URL template (disabled in free-tier mode). */
    QLineEdit   *tokenEdit_;     /*!< API token field, password-masked (disabled in free-tier mode). */
    QPushButton *showHideBtn_;   /*!< Toggles token visibility (disabled in free-tier mode). */
};

#endif // MAPSETTINGSDIALOG_H

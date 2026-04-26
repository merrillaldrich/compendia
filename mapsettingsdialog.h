#ifndef MAPSETTINGSDIALOG_H
#define MAPSETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>

/*! \brief Modal dialog for configuring the Mapbox API token and tile URL template.
 *
 * Both values are persisted to QSettings on OK; Cancel discards all changes.
 * The dialog is built programmatically — no .ui file.
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
    QLineEdit *tileUrlEdit_; /*!< Editable tile URL template. */
    QLineEdit *tokenEdit_;   /*!< API token field (password-masked by default). */
};

#endif // MAPSETTINGSDIALOG_H

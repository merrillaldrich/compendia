#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QPaintEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class AboutDialog;
}
QT_END_NAMESPACE

/*! \brief Modal "About Luminism" dialog.
 *
 * Displays the application name, version number, third-party dependency
 * acknowledgements, license, and copyright notice.  Layout is defined in
 * aboutdialog.ui; the version string is injected at construction time from
 * the CMake-generated version.h.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    /*! \brief Constructs the About dialog and populates the version label.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit AboutDialog(QWidget *parent = nullptr);

    /*! \brief Destroys the dialog and releases the generated UI object. */
    ~AboutDialog();

protected:
    /*! \brief Renders the SVG background behind all child widgets. */
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::AboutDialog *ui; ///< Pointer to the generated UI object for this dialog.
};

#endif // ABOUTDIALOG_H

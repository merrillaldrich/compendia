#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

/*! \brief Modal "About Luminism" dialog.
 *
 * Displays the application name, version number, third-party dependency
 * acknowledgements, license, and copyright notice.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    /*! \brief Constructs the About dialog.
     *
     * \param parent Optional Qt parent widget.
     */
    explicit AboutDialog(QWidget *parent = nullptr);
};

#endif // ABOUTDIALOG_H

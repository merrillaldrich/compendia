#include "mainwindow.h"

#include <QApplication>
#include <QFile>

// ── Theme colours ──────────────────────────────────────────────────────────
static const char* NAV_BLUE    = "rgb(96, 174, 233)"; ///< Lighter blue — nav panel
static const char* MENU_BLUE = "rgb(0, 135, 200)";  ///< Darker blue  — menu/status bar
// ───────────────────────────────────────────────────────────────────────────

/*! \brief Application entry point; creates the QApplication and shows the MainWindow.
 *
 * \param argc Argument count passed by the operating system.
 * \param argv Argument vector passed by the operating system.
 * \return The exit code returned by QApplication::exec().
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qss(":/resources/luminism.qss");
    if (qss.open(QFile::ReadOnly)) {
        QString styles = qss.readAll();
        styles.replace("@NAV_BLUE",    NAV_BLUE);
        styles.replace("@MENU_BLUE", MENU_BLUE);
        a.setStyleSheet(styles);
    }
    MainWindow w;
    w.show();
    return a.exec();
}

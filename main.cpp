#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QStyleHints>
#include <QTimer>

// ── Theme colours ──────────────────────────────────────────────────────────
static const char* NAV_BLUE  = "rgb(96, 174, 233)"; ///< Lighter blue — nav panel
static const char* MENU_BLUE = "rgb(0, 135, 200)";  ///< Darker blue  — menu/status bar
// ───────────────────────────────────────────────────────────────────────────

/*! \brief Application entry point; creates the QApplication and shows the MainWindow.
 *
 * \param argc Argument count passed by the operating system.
 * \param argv Argument vector passed by the operating system.
 * \return The exit code returned by QApplication::exec().
 */
static void applyTheme(QApplication &a, bool dark)
{
    QFile qss(":/resources/compendia.qss");
    if (!qss.open(QFile::ReadOnly)) return;
    QString styles = qss.readAll();
    styles.replace("@NAV_BLUE",        NAV_BLUE);
    styles.replace("@MENU_BLUE",       MENU_BLUE);
    styles.replace("@MENU_BORDER",     dark ? "#555555" : "#DDDDDD");
    styles.replace("@PROGRESS_BORDER", dark ? "#666666" : "#AAAAAA");
    styles.replace("@PROGRESS_BG",     dark ? "#3A3A3A" : "#E0E0E0");
    styles.replace("@PROGRESS_TEXT",   dark ? "#CCCCCC" : "#555555");
    styles.replace("@WINDOW_BG",       a.palette().window().color().name());
    a.setStyleSheet(styles);

    // Qt sends QEvent::StyleChange to all widgets when the application stylesheet
    // changes, but some platform backends cache palette-derived colours (e.g.
    // palette(window) values and widget-level stylesheet combinations) and do not
    // fully repaint on a live scheme switch.  Walking every widget with an explicit
    // unpolish → polish → update cycle discards those stale caches and guarantees
    // that every widget repaints exactly as it would on a cold start.
    // At launch this loop is a no-op because no widgets have been created yet.
    for (QWidget *w : QApplication::allWidgets()) {
        w->style()->unpolish(w);
        w->style()->polish(w);
        w->update();
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/resources/compendia_icon.svg"));

    applyTheme(a, QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);

    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                     &a, [&a](Qt::ColorScheme scheme) {
        // Defer one event-loop tick so Qt finishes updating its own palette first.
        const bool dark = (scheme == Qt::ColorScheme::Dark);
        QTimer::singleShot(0, &a, [&a, dark]() { applyTheme(a, dark); });
    });

    MainWindow w;
    w.show();
    return a.exec();
}

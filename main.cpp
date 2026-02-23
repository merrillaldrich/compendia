#include "mainwindow.h"

#include <QApplication>

/*! \brief Application entry point; creates the QApplication and shows the MainWindow.
 *
 * \param argc Argument count passed by the operating system.
 * \param argv Argument vector passed by the operating system.
 * \return The exit code returned by QApplication::exec().
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

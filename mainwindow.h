#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemSelection>
#include <QResizeEvent>
#include <QGraphicsView>
#include "luminismcore.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    LuminismCore *core;

    void freshenPreview();

private slots:

    void on_actionOpen_Folder_triggered();
    void onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_previewSplitter_splitterMoved(int pos, int index);

    void on_windowBodySplitter_splitterMoved(int pos, int index);

    void on_mediaFolderBrowseButton_clicked();

    void on_mediaFolderLineEdit_returnPressed();

    void on_saveButton_clicked();

    void on_fileNameFilterLineEdit_textChanged(const QString &arg1);

protected:
    // Override resizeEvent to handle window resizing
    void resizeEvent(QResizeEvent *event) override;
    void setRootFolder();
    void clearPreview();
    void refreshNavTagLibrary();
    void refreshTagAssignmentArea();
};

#endif // MAINWINDOW_H

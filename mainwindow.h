#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemSelection>
#include <QResizeEvent>
#include <QGraphicsView>
#include <QImageReader>
#include <QMessageBox>
#include <QProgressBar>
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
    QProgressBar* progress_bar_;
    QLabel* progress_label_;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    LuminismCore *core;

    void setRootFolder();
    void freshenPreview();
    void clearPreview();
    void refreshNavTagLibrary();
    void refreshTagAssignmentArea();
    void refreshTagFilterArea();

private slots:

    void on_actionOpen_Folder_triggered();
    void onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_previewSplitter_splitterMoved(int pos, int index);

    void on_windowBodySplitter_splitterMoved(int pos, int index);

    void on_mediaFolderBrowseButton_clicked();

    void on_mediaFolderLineEdit_returnPressed();

    void on_saveButton_clicked();

    void on_tagUnassign_Requested(Tag* tag);

    void on_tagFilterRemove_Requested(Tag* tag);

    void on_fileNameFilterLineEdit_textChanged(const QString &arg1);

    void on_folderFilterLineEdit_textChanged(const QString &arg1);

    void on_icon_updated();
    void on_metadata_saved();

protected:
    // Override resizeEvent to handle window resizing
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luminismcore.h"
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "flowlayout.h"
#include <QFileDialog>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , core(new LuminismCore(this))
{

    ui->setupUi(this);

    // Default Root Folder
    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");
    le->setText("C:/Users/merri/OneDrive/Pictures/");

    // Set up the tag library area
    FlowLayout* navTagLibraryLayout = new FlowLayout(ui->navLibraryContainer);
    ui->navLibraryContainer->setLayout(navTagLibraryLayout);

    // Set up the tag assignment area
    FlowLayout* fileListTagAssignmentLayout = new FlowLayout(ui->fileListTagAssignmentContainer);
    ui->fileListTagAssignmentContainer->setLayout(fileListTagAssignmentLayout);

    // Set up the preview area
    QGraphicsScene* scene = new QGraphicsScene();
    ui->previewGraphicsView->setScene(scene);

    // Default pane sizes
    resize(1400, 900);

    QList<int> sizes = {350,650,400} ;
    ui->windowBodySplitter->setSizes(sizes);
    sizes = {100,500,300};
    ui->navSplitter->setSizes(sizes);
    sizes = {200,800,200};
    ui->fileListSplitter->setSizes(sizes);
    sizes = {800,200};
    ui->previewSplitter->setSizes(sizes);

    // Drag and drop setup
    ui->navFilterContainer->setAcceptDrops(true);
    ui->fileListTagAssignmentContainer->setAcceptDrops(true);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete core;
}


void MainWindow::on_actionOpen_Folder_triggered()
{
    setRootFolder();
}

void MainWindow::on_mediaFolderBrowseButton_clicked()
{
    setRootFolder();
}

void MainWindow::on_mediaFolderLineEdit_returnPressed()
{
    QObject *obj = sender();
    QLineEdit *le = qobject_cast<QLineEdit*>(obj);
    core->setRootDirectory(le->text());
    le->clearFocus();

    QListView* lv = ui->fileListView;
    lv->setModel(core->getItemModel());
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    lv->show();
}

void MainWindow::setRootFolder(){
    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");

    QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", le->text());
    core->setRootDirectory(folder);

    le->setText(folder);

    QListView* lv = ui->fileListView;
    lv->setModel(core->getItemModel());
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    lv->show();
    refreshNavTagLibrary();
    clearPreview();
}

void MainWindow::refreshNavTagLibrary(){

    TagFamilyWidget* w = nullptr;

    // Loop over all the tags in the library
    QList<Tag*>* libTags = core->getLibraryTags();
    for(int ti=0; ti < libTags->count(); ++ti){

        // If there are no tag families or there's no tagfamilywidget for the current tag's family, add a tag family widget
        Tag* currentTag = libTags->at(ti);

        TagFamily* currentTagFamily = currentTag->tagFamily;

        QList<TagFamilyWidget*> existingWidgets = ui->navLibraryContainer->findChildren<TagFamilyWidget*>();

        for (TagFamilyWidget* tfw : existingWidgets){
            if (tfw->tag_family_ == currentTagFamily) {
                // Tag family widget exists, use it
                w = tfw;
            }
        }

        if(w==nullptr){
            w = new TagFamilyWidget(currentTagFamily, this);
            ui->navLibraryContainer->layout()->addWidget(w);
            w->show();
        }
    }
}

void MainWindow::onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {

    QList<QModelIndex> sel = selected.indexes();

    int firstInd = sel.first().row();

    // Get the absolute path to the selected file
    QVariant selectedImage = core->getItemModel()->item(firstInd)->data();
    TaggedFile* itemAsTaggedFile = selectedImage.value<TaggedFile*>();

    // Get the previewer scene
    QGraphicsView* view = ui->previewGraphicsView;
    QGraphicsScene* scene = view->scene();

    // Load the image
    QPixmap pixmap(itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName);
    //    QMessageBox::critical(nullptr, "Error", "Failed to load image.");
    //    return -1;
    //}

    // Replace the image in the scene
    scene->clear();
    QGraphicsPixmapItem *item = scene->addPixmap(pixmap);
    item->setTransformationMode(Qt::SmoothTransformation);

    // Fix up zoom and such
    view->fitInView(item->boundingRect(), Qt::KeepAspectRatio);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->show();
}

void MainWindow::freshenPreview(){
    QGraphicsView* view = ui->previewGraphicsView;
    if (view->scene() != nullptr){
        view->fitInView(view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void MainWindow::clearPreview(){
    QGraphicsView* view = ui->previewGraphicsView;
    if (view->scene() != nullptr){
        view->scene()->clear();
    }
}

void MainWindow::on_previewSplitter_splitterMoved(int pos, int index)
{
    freshenPreview();
}

void MainWindow::on_windowBodySplitter_splitterMoved(int pos, int index)
{
    freshenPreview();
}

void MainWindow::resizeEvent(QResizeEvent *event) {

    // Be careful not to do anything before the window actually contains a view and image
    freshenPreview();

    // Always call base class implementation
    QMainWindow::resizeEvent(event);
}

void MainWindow::on_saveButton_clicked()
{
    core->writeFileMetadata();
}


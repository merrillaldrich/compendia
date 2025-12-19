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
    sizes = {700,300};
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
    lv->setModel(core->getItemModelProxy());
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    lv->show();
}

void MainWindow::setRootFolder(){
    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");

    QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", le->text());
    QListView* lv = ui->fileListView;
    lv->setModel(nullptr);

    core->setRootDirectory(folder);

    le->setText(folder);
    lv->setModel(core->getItemModelProxy());
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    lv->show();
    refreshNavTagLibrary();
    refreshTagAssignmentArea();
    clearPreview();
}

void MainWindow::refreshNavTagLibrary(){

    // Update the display of the tag library to show all the tag families and tags

    // Loop over all the tags in the library
    QList<Tag*>* libTags = core->getLibraryTags();
    for (int ti=0; ti < libTags->count(); ++ti){

        TagFamilyWidget* w = nullptr;
        TagWidget* t = nullptr;

        Tag* currentTag = libTags->at(ti);
        TagFamily* currentTagFamily = currentTag->tagFamily;

        // If there are no tag family widgets or there's no tagfamilywidget for the current tag's family, add a new tagfamilywidget
        QList<TagFamilyWidget*> existingTFWidgets = ui->navLibraryContainer->findChildren<TagFamilyWidget*>();

        //for(TagFamilyWidget* tfw : existingTFWidgets){
        for(int tfwi = 0; tfwi < existingTFWidgets.count(); ++tfwi){
            TagFamilyWidget* existingTFWidget = existingTFWidgets.at(tfwi);

            if (existingTFWidget->getTagFamily() == currentTagFamily) {
                // Tag family widget exists, use it
                w = existingTFWidget;
            }
        }

        if (w==nullptr){
            TagFamily* tf = core->getTagFamily(currentTag->tagFamily->getName());
            w = new TagFamilyWidget(tf, ui->navLibraryContainer);
            ui->navLibraryContainer->layout()->addWidget(w);
            w->show();
        }

        // If there are no tag widgets, or there's no tagwidget for the current tag in the current family widget, add a new tagwidget
        QList<TagWidget*> existingTWidgets = w->findChildren<TagWidget*>();

        for(int twi = 0; twi < existingTWidgets.count(); ++twi){
            TagWidget* existingTWidget = existingTWidgets.at(twi);
            if(existingTWidget->getTag() == currentTag){
                // Tag widget exists, use it
                t = existingTWidget;
            }
        }

        if (t==nullptr){
            t = new TagWidget(currentTag, w);
            w->layout()->addWidget(t);
            t->show();
        }
    }
}

void MainWindow::refreshTagAssignmentArea(){

    // Update the display of the tag assignment area to show all the tag families and tags that are
    // associated to files in the file list

    // Loop over all the tags assigned to files

    QList<Tag*>* assignedTags = core->getAssignedTags_FilteredFiles();

    // Remove existing tag assignment widgets
    QLayout* l = ui->fileListTagAssignmentContainer->layout();
    QLayoutItem* item;
    while ((item = l->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr); // Detach from parent
            widget->deleteLater();      // Schedule deletion
        }
    }

    for (int ti=0; ti < assignedTags->count(); ++ti){

        TagFamilyWidget* w = nullptr;
        TagWidget* t = nullptr;

        Tag* currentTag = assignedTags->at(ti);
        TagFamily* currentTagFamily = currentTag->tagFamily;

        // If there are no tag family widgets or there's no tagfamilywidget for the current tag's family, add a new tagfamilywidget
        QList<TagFamilyWidget*> existingTFWidgets = ui->fileListTagAssignmentContainer->findChildren<TagFamilyWidget*>();

        //for(TagFamilyWidget* tfw : existingTFWidgets){
        for(int tfwi = 0; tfwi < existingTFWidgets.count(); ++tfwi){
            TagFamilyWidget* existingTFWidget = existingTFWidgets.at(tfwi);

            if (existingTFWidget->getTagFamily() == currentTagFamily) {
                // Tag family widget exists, use it
                w = existingTFWidget;
            }
        }

        if (w==nullptr){
            TagFamily* tf = core->getTagFamily(currentTag->tagFamily->getName());
            w = new TagFamilyWidget(tf, ui->fileListTagAssignmentContainer);
            ui->fileListTagAssignmentContainer->layout()->addWidget(w);
            w->show();
        }

        // If there are no tag widgets, or there's no tagwidget for the current tag in the current family widget, add a new tagwidget
        QList<TagWidget*> existingTWidgets = w->findChildren<TagWidget*>();

        for(int twi = 0; twi < existingTWidgets.count(); ++twi){
            TagWidget* existingTWidget = existingTWidgets.at(twi);
            if(existingTWidget->getTag() == currentTag){
                // Tag widget exists, use it
                t = existingTWidget;
            }
        }

        if (t==nullptr){
            t = new TagWidget(currentTag, w);
            w->layout()->addWidget(t);
            t->show();
        }
    }
}

void MainWindow::onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {

    // Get the previewer scene
    QGraphicsView* view = ui->previewGraphicsView;
    QGraphicsScene* scene = view->scene();

    if( selected.isEmpty()){
        scene->clear();
    }
    else {
        QModelIndex proxyIndex = selected.indexes().first();

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = core->getItemModelProxy()->mapToSource(proxyIndex);

        // Get the absolute path to the selected file
        QVariant selectedImage = core->getItemModel()->data(sourceIndex, Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = selectedImage.value<TaggedFile*>();


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


void MainWindow::on_fileNameFilterLineEdit_textChanged(const QString &arg1)
{
    core->setFileNameFilter(ui->fileNameFilterLineEdit->text());
    if (ui->fileListView->selectionModel()) {
        ui->fileListView->selectionModel()->clearSelection();
    }
    refreshTagAssignmentArea();
}


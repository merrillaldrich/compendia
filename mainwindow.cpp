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

    // Set up the tag filter area
    FlowLayout* navFilterLayout = new FlowLayout(ui->navFilterContainer);
    ui->navFilterContainer->setLayout(navFilterLayout);
    if (connect(ui->navFilterContainer, &NavFilterContainer::tagDeleteRequested, this, &MainWindow::on_tagFilterRemove_Requested)){

    } else {
        qWarning() << "Connecting nav filter delete function didn't work";
    }

    // Set up the tag assignment area
    FlowLayout* fileListTagAssignmentLayout = new FlowLayout(ui->fileListTagAssignmentContainer);
    ui->fileListTagAssignmentContainer->setLayout(fileListTagAssignmentLayout);
    if (connect(ui->fileListTagAssignmentContainer, &TagAssignmentContainer::tagDeleteRequested, this, &MainWindow::on_tagUnassign_Requested)){

    } else {
        qWarning() << "Connecting assignment delete function didn't work";
    }

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

    // Settings so multithreading does not take over all cores
    int idealThreads = QThread::idealThreadCount();
    if (idealThreads <= 4){
        QThreadPool::globalInstance()->setMaxThreadCount(idealThreads);
    } else {
        QThreadPool::globalInstance()->setMaxThreadCount(idealThreads - 2);
    }
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

    QSet<Tag*>* libTags = core->getLibraryTags();
    ui->navLibraryContainer->refresh(libTags);
}

void MainWindow::refreshTagAssignmentArea(){

    // Update the display of the tag assignment area to show all the tag families and tags that are
    // associated to files in the file list

    QSet<Tag*>* assignedTags = core->getAssignedTags_FilteredFiles();
    ui->fileListTagAssignmentContainer->refresh(assignedTags);

}

void MainWindow::refreshTagFilterArea(){

    // Update the display of the tag filter area to show all the tag families and tags that are
    // currently set as filters

    QSet<Tag*>* filterTags = core->getFilterTags();
    ui->navFilterContainer->refresh(filterTags);

}

void MainWindow::onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {

    // Get the previewer scene
    QGraphicsView* view = ui->previewGraphicsView;
    QGraphicsScene* scene = view->scene();

    if( selected.isEmpty()){
        scene->clear();
        ui->previewFileNameValue->setText("-");
        ui->previewFileLocationValue->setText("-");
        ui->previewFileCreatedValue->setText("-");
        ui->previewFileModifiedValue->setText("-");
        ui->previewFileTagsValue->setText("-");
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

        QLocale locale = QLocale::system();

        ui->previewFileNameValue->setText(itemAsTaggedFile->fileName);
        ui->previewFileLocationValue->setText(itemAsTaggedFile->filePath);
        ui->previewFileCreatedValue->setText(locale.toString(itemAsTaggedFile->fileCreationDateTime, QLocale::ShortFormat));
        ui->previewFileModifiedValue->setText(locale.toString(itemAsTaggedFile->fileModificationDateTime, QLocale::ShortFormat));

        QString tagText("");
        QMap<QString, QList<QString>> dict;

        QSetIterator<Tag*> it(*itemAsTaggedFile->tags);
        while (it.hasNext()) {
            Tag* t = it.next(); // Advances iterator and returns the element
            QString famName = t->tagFamily->getName();
            if (!dict.contains(famName)){
                dict.insert(famName,QList<QString>(t->getName()));
            } else {
                dict[famName].append(t->getName());
            }
        }

        for (auto [key, value] : dict.asKeyValueRange()) {
            tagText += key;
            tagText += ": ";
            tagText += value.join(", ");
            tagText += " ";
        }

        ui->previewFileTagsValue->setText(tagText);

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
    QMainWindow::resizeEvent(event);
}

void MainWindow::on_saveButton_clicked(){
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

void MainWindow::on_tagUnassign_Requested(Tag* tag){
    core->unapplyTag(tag);
    refreshTagAssignmentArea();
}

void MainWindow::on_tagFilterRemove_Requested(Tag* tag){
    core->removeTagFilter(tag);
    refreshTagFilterArea();
}

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luminismcore.h"
#include "tagwidget.h"
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

    // Set up the tag library area
    FlowLayout* navTagLibraryLayout = new FlowLayout(ui->navLibraryContainer);

    Tag* t = new Tag(new TagFamily(),"Foo");
    TagWidget* tw = new TagWidget(t);
    navTagLibraryLayout->addWidget(tw);

    t = new Tag(new TagFamily(),"Foo");
    tw = new TagWidget(t);
    navTagLibraryLayout->addWidget(tw);

    t = new Tag(new TagFamily(),"Foo");
    tw = new TagWidget(t);
    navTagLibraryLayout->addWidget(tw);

    t = new Tag(new TagFamily(),"Foo");
    tw = new TagWidget(t);
    navTagLibraryLayout->addWidget(tw);

    ui->navLibraryContainer->setLayout(navTagLibraryLayout);

    // Set up the tag assignment area

    FlowLayout* fileListTagAssignmentLayout = new FlowLayout(ui->fileListTagAssignmentContainer);

    t = new Tag(new TagFamily(),"Bar");
    tw = new TagWidget(t);
    fileListTagAssignmentLayout->addWidget(tw);

    t = new Tag(new TagFamily(),"Baz");
    tw = new TagWidget(t);
    fileListTagAssignmentLayout->addWidget(tw);

    ui->fileListTagAssignmentContainer->setLayout(fileListTagAssignmentLayout);

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
}


void MainWindow::on_actionOpen_Folder_triggered()
{
   QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", "C:/Users/merri/Pictures");
   core->setRootDirectory(folder);

   QListView* lv = ui->fileListView;
   lv->setModel(core->tfc->getItemModel());
   connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
   lv->show();
}

void MainWindow::onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {

    QList<QModelIndex> sel = selected.indexes();

    int firstInd = sel.first().row();

    // Get the absolute path to the selected file
    QVariant selectedImage = core->tfc->getItemModel()->item(firstInd)->data();
    TaggedFile* itemAsTaggedFile = selectedImage.value<TaggedFile*>();

    // Create a scene
    QGraphicsScene* scene = new QGraphicsScene();

    // Load the image
    QPixmap pixmap(itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName); // Use resource or absolute path
    //if (pixmap.isNull()) {
    //    QMessageBox::critical(nullptr, "Error", "Failed to load image.");
    //    return -1;
    //}

    // Add the image to the scene
    QGraphicsPixmapItem *item = scene->addPixmap(pixmap);
    item->setTransformationMode(Qt::SmoothTransformation); // Better scaling

    // Get the ui graphics view, set the scene
    QGraphicsView* view = ui->previewGraphicsView;
    view->setScene(scene);
    view->fitInView(item->boundingRect(), Qt::KeepAspectRatio);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag); // Optional: drag to move
    view->show();
}

void MainWindow::freshenPreview(){
    QGraphicsView* view = ui->previewGraphicsView;
    if (view->scene() != nullptr){
        view->fitInView(view->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
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


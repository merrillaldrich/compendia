#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luminismcore.h"
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

    resize(1400, 900);

    QList<int> sizes = {350,650,400} ;
    ui->windowBodySplitter->setSizes(sizes);
    sizes = {100,500,300};
    ui->navSplitter->setSizes(sizes);
    sizes = {200,800,200};
    ui->fileListSplitter->setSizes(sizes);
    sizes = {800,200};
    ui->previewSplitter->setSizes(sizes);
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
    qDebug() << "Selection Changed";

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

    // Create a view, set the scene
    QGraphicsView* view = ui->previewGraphicsView;
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRenderHint(QPainter::SmoothPixmapTransform);
    view->setDragMode(QGraphicsView::ScrollHandDrag); // Optional: drag to move
    view->show();
}

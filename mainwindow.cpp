#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luminismcore.h"
#include <QFileDialog>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , core(new LuminismCore(this))
{

    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionOpen_Folder_triggered()
{
   QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", "C:/Users/merri/Pictures");
   core->setRootDirectory(folder);
}


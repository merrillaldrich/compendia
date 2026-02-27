#include "mainwindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPair>
#include <QDebug>

#include "./ui_mainwindow.h"
#include "luminismcore.h"
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "flowlayout.h"
#include "filenamedelegate.h"

/*! \brief Constructs the main window, sets up layouts, status bar, and default pane sizes.
 *
 * \param parent Optional Qt parent widget.
 */
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
    //ui->navLibraryScrollArea->setStyleSheet("background-color: rgb(96, 174, 233)");
    //ui->navLibraryScrollArea->viewport()->setStyleSheet("background-color: rgb(96, 174, 233)");
    ui->navLibraryContainer->setLayout(navTagLibraryLayout);

    // Set up the tag filter area
    FlowLayout* navFilterLayout = new FlowLayout(ui->navFilterContainer);
    ui->navFilterContainer->setLayout(navFilterLayout);
    //ui->navFilterScrollArea->setStyleSheet("background-color: rgb(96, 174, 233)");
    //ui->navFilterScrollArea->viewport()->setStyleSheet("background-color: rgb(96, 174, 233)");

    // Set up the tag assignment area
    FlowLayout* fileListTagAssignmentLayout = new FlowLayout(ui->fileListTagAssignmentContainer);
    ui->fileListTagAssignmentContainer->setLayout(fileListTagAssignmentLayout);

    // Set up the status bar
    progress_label_ = new QLabel("Progress:", this);
    ui->statusBar->addPermanentWidget(progress_label_);

    progress_bar_ = new QProgressBar(this);
    progress_bar_->setTextVisible(false);
    progress_bar_->setMinimum(0);
    progress_bar_->setMaximum(1000);
    progress_bar_->setFixedSize(300, 12);
    progress_bar_->setValue(0);

    ui->statusBar->addPermanentWidget(progress_bar_);
    connect(core, &LuminismCore::iconUpdated, this, &MainWindow::onIconUpdated);
    connect(core, &LuminismCore::metadataSaved, this, &MainWindow::onMetadataSaved);


    // Default pane sizes
    resize(1400, 900);

    ui->windowBodySplitter->setSizes({350,650,400});
    ui->navSplitter->setSizes({100,200,600});
    ui->fileListSplitter->setSizes({700,300});
    ui->previewSplitter->setSizes({800,200});

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

    // Install multi-line filename delegate on the file list view
    ui->fileListView->setItemDelegate(new FileNameDelegate(ui->fileListView));

    // on_iconZoomSlider_valueChanged is auto-connected after setupUi sets the slider
    // value, so the initial grid height is never updated from the .ui default.
    // Apply it explicitly here to match the slider's starting position.
    on_iconZoomSlider_valueChanged(ui->iconZoomSlider->value());

    // Same timing issue applies to the tag filter mode radio buttons: the toggled
    // signal fires before the auto-connection exists, so the proxy never receives
    // the initial state.  Sync it explicitly here and after every folder load.
    on_tagFilterAllRadio_toggled(ui->tagFilterAllRadio->isChecked());
}

/*! \brief Destroys the main window and releases owned UI and core resources. */
MainWindow::~MainWindow()
{
    delete ui;
    delete core;
}

/*! \brief Slot for the Open Folder menu action; delegates to setRootFolder(). */
void MainWindow::on_actionOpen_Folder_triggered()
{
    setRootFolder();
}

/*! \brief Slot for the browse button; delegates to setRootFolder(). */
void MainWindow::on_mediaFolderBrowseButton_clicked()
{
    setRootFolder();
}

/*! \brief Loads the folder path typed directly into the media folder line edit. */
void MainWindow::on_mediaFolderLineEdit_returnPressed()
{
    QObject *obj = sender();
    QLineEdit *le = qobject_cast<QLineEdit*>(obj);
    core->setRootDirectory(le->text());
    le->clearFocus();
    ui->dateEdit->setAvailableDates(core->getFileDates());
    ui->folderFilterLineEdit->setAvailablePaths(core->getFileFolders());

    QListView* lv = ui->fileListView;
    lv->setModel(core->getItemModelProxy());
    connectFileCountLabel();
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    on_tagFilterAllRadio_toggled(ui->tagFilterAllRadio->isChecked());
    lv->show();
}

/*! \brief Opens a folder-picker dialog and loads the selected folder into core. */
void MainWindow::setRootFolder(){
    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");

    QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", le->text());
    QListView* lv = ui->fileListView;
    lv->setModel(nullptr);

    ui->navFilterContainer->clear();
    ui->fileListTagAssignmentContainer->clear();
    ui->navLibraryContainer->clear();
    TagFamily::restartColorSequence();

    core->setRootDirectory(folder);
    ui->dateEdit->setAvailableDates(core->getFileDates());
    ui->folderFilterLineEdit->setAvailablePaths(core->getFileFolders());

    le->setText(folder);
    lv->setModel(core->getItemModelProxy());
    connectFileCountLabel();
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    on_tagFilterAllRadio_toggled(ui->tagFilterAllRadio->isChecked());
    lv->show();
    refreshNavTagLibrary();
    refreshTagAssignmentArea();
    clearPreview();
    progress_bar_->setMaximum(core->getItemModel()->rowCount());
}

/*! \brief Rebuilds the tag-library navigation area from the current library tags. */
void MainWindow::refreshNavTagLibrary(){

    // Update the display of the tag library to show all the tag families and tags

    QSet<Tag*>* libTags = core->getLibraryTags();
    ui->navLibraryContainer->refresh(libTags);
}

/*! \brief Rebuilds the tag-assignment area from tags on the currently filtered files. */
void MainWindow::refreshTagAssignmentArea(){

    // Update the display of the tag assignment area to show all the tag families and tags that are
    // associated to files in the file list

    QSet<Tag*>* assignedTags = core->getAssignedTags_FilteredFiles();
    ui->fileListTagAssignmentContainer->refresh(assignedTags);

}

/*! \brief Rebuilds the tag-filter area from the currently active filter tags. */
void MainWindow::refreshTagFilterArea(){

    // Update the display of the tag filter area to show all the tag families and tags that are
    // currently set as filters

    QSet<Tag*>* filterTags = core->getFilterTags();
    ui->navFilterContainer->refresh(filterTags);

}

/*! \brief Updates the preview and property panel when the file-list selection changes.
 *
 * \param selected   The newly selected model indexes.
 * \param deselected The previously selected model indexes that are now deselected.
 */
void MainWindow::onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {

    if( selected.isEmpty()){
        ui->previewContainer->clear();
        ui->previewFileNameValue->setText("-");
        ui->previewFileLocationValue->setText("-");
        ui->previewImageCapturedValue->setText("-");
        ui->previewFileCreatedValue->setText("-");
        ui->previewFileModifiedValue->setText("-");
        ui->previewFileTagsValue->setText("-");
        ui->previewExifValue->setText("-");
    }
    else {
        QModelIndex proxyIndex = selected.indexes().first();

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = core->getItemModelProxy()->mapToSource(proxyIndex);

        // Get the absolute path to the selected file
        QVariant selectedImage = core->getItemModel()->data(sourceIndex, Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = selectedImage.value<TaggedFile*>();

        ui->previewContainer->preview(itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName);

        // Build tag rect overlays and push to the preview container
        QList<QPair<QRectF, QColor>> tagRects;
        for (Tag* tag : *itemAsTaggedFile->tags()) {
            auto r = itemAsTaggedFile->tagRect(tag);
            if (r.has_value())
                tagRects.append({r.value(), tag->tagFamily->getColor()});
        }
        ui->previewContainer->setTagRects(tagRects);
        ui->previewContainer->setTagRectsVisible(ui->showTaggedRegionsCheckbox->isChecked());

        // Properties area
        QLocale locale = QLocale::system();

        ui->previewFileNameValue->setText(itemAsTaggedFile->fileName);
        ui->previewFileLocationValue->setText(itemAsTaggedFile->filePath);
        if (ui->previewImageCapturedValue != nullptr)
            ui->previewImageCapturedValue->setText(locale.toString(itemAsTaggedFile->imageCaptureDateTime, QLocale::ShortFormat));
        ui->previewFileCreatedValue->setText(locale.toString(itemAsTaggedFile->fileCreationDateTime, QLocale::ShortFormat));
        ui->previewFileModifiedValue->setText(locale.toString(itemAsTaggedFile->fileModificationDateTime, QLocale::ShortFormat));

        QString tagText("");
        QMap<QString, QList<QString>> dict;

        QSetIterator<Tag*> it(*itemAsTaggedFile->tags());
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

        QString exifText;
        const QMap<QString, QString> exifMap = itemAsTaggedFile->exifMap();
        for (auto [key, value] : exifMap.asKeyValueRange()) {
            exifText += key + ": " + value + "  ";
        }
        ui->previewExifValue->setText(exifText);

    }
}

/*! \brief Refreshes the preview pane to reflect the current viewport size. */
void MainWindow::freshenPreview(){
    ui->previewContainer->freshen();
}

/*! \brief Clears the preview pane of any displayed image. */
void MainWindow::clearPreview(){
    ui->previewContainer->clear();
}

/*! \brief Refreshes the preview when the preview splitter is moved.
 *
 * \param pos   New splitter handle position.
 * \param index Index of the splitter handle that moved.
 */
void MainWindow::on_previewSplitter_splitterMoved(int pos, int index)
{
    freshenPreview();
}

/*! \brief Refreshes the preview when the window body splitter is moved.
 *
 * \param pos   New splitter handle position.
 * \param index Index of the splitter handle that moved.
 */
void MainWindow::on_windowBodySplitter_splitterMoved(int pos, int index)
{
    freshenPreview();
}

/*! \brief Overrides the Qt base-class resize handler to freshen the preview on resize.
 *
 * \param event The resize event containing the new window size.
 */
void MainWindow::resizeEvent(QResizeEvent *event) {

    freshenPreview();
    QMainWindow::resizeEvent(event);
}

/*! \brief Slot for the Save button; writes metadata for all dirty files. */
void MainWindow::on_saveButton_clicked(){
    core->writeFileMetadata();
}

/*! \brief Updates the filename filter as the user types in the filter line edit.
 *
 * \param arg1 The current text of the filename filter line edit.
 */
void MainWindow::on_fileNameFilterLineEdit_textChanged(const QString &arg1)
{
    core->setFileNameFilter(ui->fileNameFilterLineEdit->text());
    if (ui->fileListView->selectionModel()) {
        ui->fileListView->selectionModel()->clearSelection();
    }
    refreshTagAssignmentArea();
}

/*! \brief Removes a tag from all filtered files when an unassign is requested.
 *
 * \param tag The Tag to unassign from the filtered files.
 */
void MainWindow::on_fileListTagAssignmentContainer_tagDeleteRequested(Tag* tag){
    core->unapplyTag(tag);
    refreshTagAssignmentArea();
}

/*! \brief Removes a tag from the active filter set when a filter-remove is requested.
 *
 * \param tag The Tag to remove from the filter.
 */
void MainWindow::on_navFilterContainer_tagDeleteRequested(Tag* tag){
    core->removeTagFilter(tag);
    refreshTagFilterArea();
}

/*! \brief Updates the folder filter as the user types in the folder filter line edit.
 *
 * \param arg1 The current text of the folder filter line edit.
 */
void MainWindow::on_folderFilterLineEdit_textChanged(const QString &arg1)
{
    core->setFolderFilter(ui->folderFilterLineEdit->text());
    if (ui->fileListView->selectionModel()) {
        ui->fileListView->selectionModel()->clearSelection();
    }
    refreshTagAssignmentArea();
}

/*! \brief Advances the icon-generation progress bar when a thumbnail is ready. */
/*! \brief Adjusts the file-list icon and grid size when the zoom slider moves.
 *
 * \param value The new slider position (0–100).
 */
void MainWindow::on_iconZoomSlider_valueChanged(int value)
{
    int iconSize = static_cast<int>(25 + value * 3.75);
    ui->fileListView->setIconSize(QSize(iconSize, iconSize));
    // Reserve height for up to 3 wrapped text lines below the icon.
    const int textH = 4 + 3 * ui->fileListView->fontMetrics().lineSpacing() + 4;
    ui->fileListView->setGridSize(QSize(iconSize + 20, iconSize + textH));
}

/*! \brief Updates the file-count label to show visible vs. total file counts.
 *
 * Displays "<total> Files" when all files pass the filter, or
 * "<shown> of <total> Files" when a filter is active.
 */
void MainWindow::updateFileCountLabel()
{
    int total = core->getItemModel()->rowCount();
    int shown = core->getItemModelProxy()->rowCount();
    if (shown == total)
        ui->fileCountLabel->setText(QString("%1 Files").arg(total));
    else
        ui->fileCountLabel->setText(QString("%1 of %2 Files").arg(shown).arg(total));
}

/*! \brief Connects the file-count label to the current proxy model's signals.
 *
 * Must be called after every folder load because loadRootDirectory() recreates
 * the proxy model, invalidating any previously established connections.
 */
void MainWindow::connectFileCountLabel()
{
    auto* proxy = core->getItemModelProxy();
    connect(proxy, &QAbstractItemModel::rowsInserted,  this, [this](const QModelIndex&, int, int){ updateFileCountLabel(); });
    connect(proxy, &QAbstractItemModel::rowsRemoved,   this, [this](const QModelIndex&, int, int){ updateFileCountLabel(); });
    connect(proxy, &QAbstractItemModel::modelReset,    this, [this](){ updateFileCountLabel(); });
    connect(proxy, &QAbstractItemModel::layoutChanged, this, [this](const QList<QPersistentModelIndex>&, QAbstractItemModel::LayoutChangeHint){ updateFileCountLabel(); });
    updateFileCountLabel();
}

void MainWindow::onIconUpdated(){
    if(progress_label_->text() != "Generating Icons")
        progress_label_->setText("Generating Icons");
    progress_bar_->setValue(progress_bar_->value() + 1);
    if(progress_bar_->value() >= progress_bar_->maximum()){
        progress_label_->setText("Icons Complete");
        progress_bar_->setValue(0);
        // EXIF capture dates are now fully loaded — refresh the autocomplete date list
        ui->dateEdit->setAvailableDates(core->getFileDates());
    }
}

/*! \brief Advances the save progress bar when a metadata file is written. */
void MainWindow::onMetadataSaved(){
    if(progress_label_->text() != "Saving")
        progress_label_->setText("Saving");
    progress_bar_->setValue(progress_bar_->value() + 1);
    if(progress_bar_->value() >= progress_bar_->maximum()){
        progress_label_->setText("Save Complete");
        progress_bar_->setValue(0);
    }
}

/*! \brief Runs face detection on every selected image and stores face regions as tagged rectangles. */
void MainWindow::on_actionFind_Faces_triggered(){

    FaceRecognizer fr(this);

    QItemSelectionModel *selModel = ui->fileListView->selectionModel();
    QModelIndexList selectedIndexes = selModel->selectedRows();

    if (selectedIndexes.isEmpty())
        return;

    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};

    for (const QModelIndex &proxyIndex : selectedIndexes) {

        QModelIndex sourceIndex = core->getItemModelProxy()->mapToSource(proxyIndex);
        QVariant var = core->getItemModel()->data(sourceIndex, Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = var.value<TaggedFile*>();

        if (videoExts.contains(QFileInfo(itemAsTaggedFile->fileName).suffix().toLower()))
            continue;

        QImageReader ir(itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName);
        ir.setAutoTransform(true);
        QImage sourceImage = ir.read();
        if (sourceImage.isNull()) {
            QMessageBox::critical(this, "Error", "Failed to load image: " + ir.errorString());
            continue;
        }

        // Remove any previously auto-located face tags from this file
        QSet<Tag*> toRemove;
        for (Tag* t : *itemAsTaggedFile->tags()) {
            if (t->tagFamily->getName() == "People" && t->getName().startsWith("AutoLocatedFace"))
                toRemove.insert(t);
        }
        for (Tag* t : toRemove)
            itemAsTaggedFile->removeTag(t);

        // Detect faces and assign a numbered tag with rect for each one
        const QList<QRectF> faces = fr.detectFaces(sourceImage);
        for (int i = 0; i < faces.size(); ++i) {
            QString tagName = QString("AutoLocatedFace%1").arg(i + 1, 2, 10, QChar('0'));
            Tag* tag = core->addLibraryTag("People", tagName);
            itemAsTaggedFile->addTag(tag, faces[i]);
        }
    }

    refreshNavTagLibrary();
    refreshTagAssignmentArea();
}

/*! \brief Shows or hides tag region overlays in the preview when the checkbox is toggled.
 *
 * \param state The new checkbox state (Qt::Checked or Qt::Unchecked).
 */
void MainWindow::on_showTaggedRegionsCheckbox_stateChanged(int state)
{
    ui->previewContainer->setTagRectsVisible(state == Qt::Checked);
}

/*! \brief Switches the tag filter between ALL-tags (AND) and ANY-tag (OR) mode.
 *
 * \param checked True when the "ALL tags" radio button is selected.
 */
void MainWindow::on_tagFilterAllRadio_toggled(bool checked)
{
    FilterProxyModel::TagFilterMode mode = checked
        ? FilterProxyModel::AllTags
        : FilterProxyModel::AnyTag;
    core->getItemModelProxy()->setTagFilterMode(mode);
}

/*! \brief Slot — forwards creation-date filter changes to the core.
 *
 * A date equal to the widget's minimum is treated as "no filter" (null QDate).
 * \param date The newly selected date.
 */
void MainWindow::on_dateEdit_dateChanged(const QDate &date)
{
    core->setCreationDateFilter(date);  // invalid QDate = no filter
}

/*! \brief Slot for the Quit menu action; closes the main window. */
void MainWindow::on_actionQuit_triggered()
{
    close();
}

/*! \brief Intercepts the window close event to prompt for unsaved changes.
 *
 * \param event The close event to accept or ignore.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!core->hasUnsavedChanges()) {
        event->accept();
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText("There are unsaved changes.");
    msgBox.setInformativeText("Would you like to save before quitting?");

    QPushButton *saveAndQuit = msgBox.addButton("Save and Quit", QMessageBox::AcceptRole);
    QPushButton *quitAnyway  = msgBox.addButton("Discard Changes and Quit",   QMessageBox::DestructiveRole);
    /*QPushButton *cancel    =*/ msgBox.addButton("Cancel",       QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == saveAndQuit) {
        core->writeFileMetadata();
        event->accept();
    } else if (msgBox.clickedButton() == quitAnyway) {
        event->accept();
    } else {
        event->ignore();
    }
}

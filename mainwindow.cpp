#include "mainwindow.h"
#include "constants.h"

#include <algorithm>
#include <limits>

#include <QCheckBox>
#include <QCoreApplication>
#include <QScrollBar>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QDesktopServices>
#include <QMenu>
#include <QProcess>
#include <QProgressDialog>

#include "./ui_mainwindow.h"
#include "compendiacore.h"
#include "tagwidget.h"
#include "tagfamilywidget.h"
#include "filenamedelegate.h"
#include "starratingwidget.h"

/*! \brief Constructs the main window, sets up layouts, status bar, and default pane sizes.
 *
 * \param parent Optional Qt parent widget.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , core(new CompendiaCore(this))
{

    ui->setupUi(this);

    // Default Root Folder
    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");
    {
        QSettings s(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
        le->setText(s.value("folder/lastPath").toString());
    }

    // Set up container welcome hints (FlowLayout is installed by WelcomeHintContainer constructor)
    ui->navLibraryContainer->setupWelcome(
        ui->navLibraryScrollArea,
        qobject_cast<QVBoxLayout*>(ui->navLibrarySection->layout()), 1,
        "<b>Click here</b> to create groups of tags to organize your images.");

    // Tag library search box
    ui->tagSearchLineEdit->addAction(
        QIcon(":/resources/tag-search.svg"), QLineEdit::LeadingPosition);
    connect(ui->tagSearchLineEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
        ui->navLibraryContainer->filter(text);
    });
    ui->navFilterContainer->setupWelcome(
        ui->navFilterScrollArea,
        qobject_cast<QVBoxLayout *>(ui->navFilterSection->layout()), 2,
        "Drag and drop tags here to <b>filter your view.</b>");
    ui->fileListTagAssignmentContainer->setupWelcome(
        ui->fileListTagAssignmentScrollArea, ui->fileListSplitter, 1,
        "Drag and drop tags here to <b>apply them</b> to all visible files.");

    // Set up the status bar
    folderStatsLabel_ = new QLabel(this);
    folderStatsLabel_->setContentsMargins(6, 0, 6, 0);
    ui->statusBar->addWidget(folderStatsLabel_);
    progress_ = new MultiProgressBar(this);
    ui->statusBar->addPermanentWidget(progress_);
    connect(core, &CompendiaCore::iconUpdated, this, &MainWindow::onIconUpdated);
    connect(core, &CompendiaCore::batchFinished, this, [this]() {
        progress_->finishProcess(MultiProgressBar::Process::IconGeneration);
        const bool hasUnreadable = core->hasUnreadableFiles();
        ui->actionUnreadableFiles->setEnabled(hasUnreadable);
        ui->actionUnreadableFiles->setIcon(hasUnreadable
            ? QIcon(":/resources/unreadable-files.svg") : QIcon());
    });
    connect(core, &CompendiaCore::metadataSaved, this, &MainWindow::onMetadataSaved);

    connect(core, &CompendiaCore::scanStarted, this, [this]() {
        folderStatsLabel_->clear();
        progress_->startProcess(MultiProgressBar::Process::FolderScan, 0, 0, "Scanning…");
    });
    connect(core, &CompendiaCore::scanProgress, this, [this](int n) {
        progress_->startProcess(MultiProgressBar::Process::FolderScan, 0, 0,
                                QString("Scanning… %L1 files").arg(n));
    });
    connect(core, &CompendiaCore::scanFinished, this, [this](int total, int toCache) {
        progress_->finishProcess(MultiProgressBar::Process::FolderScan);
        ui->dateEdit->setAvailableDates(core->getFileDates());
        ui->folderFilterLineEdit->setAvailablePaths(core->getFileFolders());
        if (!core->getLibraryTags()->isEmpty()) {
            ui->navLibraryContainer->dismissWelcome();
            ui->navFilterContainer->dismissWelcome();
            ui->fileListTagAssignmentContainer->dismissWelcome();
        }
        refreshNavTagLibrary();
        refreshTagAssignmentArea();
        updateFolderStatsLabel();
        progress_->startProcess(MultiProgressBar::Process::IconGeneration,
                                0, toCache,
                                "Caching icons…");
        Q_UNUSED(total);
    });
    connect(progress_, &MultiProgressBar::processFinished,
            this, [this](MultiProgressBar::Process p) {
        if (p == MultiProgressBar::Process::IconGeneration)
            ui->dateEdit->setAvailableDates(core->getFileDates());
    });
    connect(core, &CompendiaCore::tagLibraryChanged, this, &MainWindow::onTagLibraryChanged);
    connect(core, &CompendiaCore::snapshotRestored,  this, &MainWindow::onSnapshotRestored);

    // Undo / Redo — wire actions into the Edit menu
    {
        QAction* undoAct = core->undoManager()->createUndoAction(this, tr("Undo"));
        undoAct->setShortcut(QKeySequence::Undo);
        QAction* redoAct = core->undoManager()->createRedoAction(this, tr("Redo"));
        redoAct->setShortcut(QKeySequence::Redo);
        ui->menuEdit->addAction(undoAct);
        ui->menuEdit->addAction(redoAct);
    }

    // Sort Library — shared with the Sort button above the tag library panel
    {
        ui->menuEdit->addSeparator();
        QAction* sortAct = new QAction(tr("Sort Tag Library"), this);
        connect(sortAct, &QAction::triggered, this, &MainWindow::sortTagLibrary);
        ui->menuEdit->addAction(sortAct);
    }

    connect(ui->sortLibraryButton, &QPushButton::clicked,
            this, &MainWindow::sortTagLibrary);

    connect(core, &CompendiaCore::fileRemovedExternally,
            this, [this](TaggedFile* tf, bool, bool) {
        progress_->showNotification("Removed: " + tf->fileName);
        updateFolderStatsLabel();
    });
    connect(core, &CompendiaCore::fileMovedExternally,
            this, [this](TaggedFile* tf, const QString&, bool) {
        progress_->showNotification("Moved: " + tf->fileName);
    });
    connect(core, &CompendiaCore::fileAddedExternally,
            this, [this](const QString& path) {
        progress_->showNotification("Added: " + QFileInfo(path).fileName());
        updateFolderStatsLabel();
    });
    connect(ui->navLibraryContainer, &TagContainer::tagNameChanged,
            this, &MainWindow::onTagNameChanged);
    connect(ui->navLibraryContainer, &TagContainer::tagDeleteRequested,
            this, &MainWindow::onNavLibraryContainerTagDeleteRequested);
    connect(ui->fileListTagAssignmentContainer, &TagContainer::tagNameChanged,
            this, &MainWindow::onTagNameChanged);
    connect(ui->previewContainer, &PreviewContainer::tagDroppedOnPreview,
            this, &MainWindow::onTagDroppedOnPreview);
    connect(ui->previewContainer, &PreviewContainer::tagDroppedOnExistingRegion,
            this, &MainWindow::onTagDroppedOnExistingRegion);
    connect(ui->previewContainer, &PreviewContainer::tagRectResized,
            this, &MainWindow::onTagRectResized);
    connect(ui->previewContainer, &PreviewContainer::tagRectDeleteRequested,
            this, &MainWindow::onTagRectDeleteRequested);
    connect(ui->previewContainer, &PreviewContainer::tagRectFindPersonRequested,
            this, &MainWindow::onTagRectFindPersonRequested);
    connect(ui->previewContainer, &PreviewContainer::tagPreviewDragEntered,
            this, [this](const QString &family, const QString &tagName) {
        Tag* tag = core->getTag(family, tagName);
        if (tag)
            ui->previewContainer->setDropPreviewColor(tag->tagFamily->getColor());
        ui->showTaggedRegionsCheckbox->setChecked(true);
    });
    connect(ui->previewContainer, &PreviewContainer::navigateRequested,
            this, [this](int delta){ navigatePreview(delta); });

    // All star widgets start disabled until a folder is loaded
    ui->filterStarRating->setEnabled(false);
    ui->previewStarRating->setEnabled(false);
    ui->fileListStarRating->setEnabled(false);

    // File-list star rating — apply a rating to selected files, or all visible files with confirmation
    connect(ui->fileListStarRating, &StarRatingWidget::ratingChanged,
            this, [this](std::optional<int> rating) {
        auto *proxy    = core->getItemModelProxy();
        auto *model    = core->getItemModel();
        auto *selModel = ui->fileListView->selectionModel();
        QModelIndexList sel = selModel ? selModel->selectedIndexes() : QModelIndexList();

        if (sel.isEmpty()) {
            const int n = proxy->rowCount();
            const QString msg = QString(
                "<p>This will apply the rating to <b>all %1</b> visible file(s). Are you sure?</p>"
                "<p>Hint: You can choose one or a few files to rate if you don't want to rate them all at once.</p>").arg(n);
            QMessageBox mb(QMessageBox::Question, tr("Apply Rating"), msg,
                           QMessageBox::Ok | QMessageBox::Cancel, this);
            if (mb.exec() != QMessageBox::Ok) {
                // Reset the widget back to no-rating display without emitting again
                ui->fileListStarRating->setRating(std::nullopt);
                return;
            }
            core->checkpoint(tr("Set rating"));
            for (int i = 0; i < n; ++i) {
                TaggedFile *tf = model->data(
                    proxy->mapToSource(proxy->index(i, 0)), Qt::UserRole + 1).value<TaggedFile*>();
                if (tf) tf->setRating(rating);
            }
        } else {
            core->checkpoint(tr("Set rating"));
            for (const QModelIndex &idx : sel) {
                TaggedFile *tf = model->data(
                    proxy->mapToSource(idx), Qt::UserRole + 1).value<TaggedFile*>();
                if (tf) tf->setRating(rating);
            }
        }
        // Reset the widget — it's a bulk action tool, not a persistent indicator
        ui->fileListStarRating->setRating(std::nullopt);
    });

    // Rating filter mode button — popup menu to choose Less Than / Exactly / More Than
    {
        auto *menu = new QMenu(ui->ratingFilterModeButton);
        auto *actLess    = menu->addAction(tr("Less Than"));
        auto *actExactly = menu->addAction(tr("Exactly"));
        auto *actMore    = menu->addAction(tr("More Than"));

        auto applyMode = [this, actLess, actExactly, actMore](QAction *chosen,
                                                               FilterProxyModel::RatingFilterMode mode) {
            ui->ratingFilterModeButton->setText(chosen->text());
            core->setRatingFilterMode(mode);
            updateFileCountLabel();
        };

        connect(actLess,    &QAction::triggered, this, [=]{ applyMode(actLess,    FilterProxyModel::LessOrEqual);    });
        connect(actExactly, &QAction::triggered, this, [=]{ applyMode(actExactly, FilterProxyModel::Exactly);        });
        connect(actMore,    &QAction::triggered, this, [=]{ applyMode(actMore,    FilterProxyModel::GreaterOrEqual); });

        ui->ratingFilterModeButton->setMenu(menu);
        ui->ratingFilterModeButton->setMaximumWidth(90);
        // background-color and text-align are set via the QSS token @NAV_BLUE;
    }

    // Filter star rating — apply a rating filter to the proxy model
    connect(ui->filterStarRating, &StarRatingWidget::ratingChanged,
            this, [this](std::optional<int> rating) {
        core->setRatingFilter(rating);
        updateFileCountLabel();
    });

    // Star rating widget — update the selected file's rating when the user clicks a star
    connect(ui->previewStarRating, &StarRatingWidget::ratingChanged,
            this, [this](std::optional<int> rating) {
        auto *selModel = ui->fileListView->selectionModel();
        if (!selModel || selModel->selectedIndexes().isEmpty()) return;
        QModelIndex src = core->getItemModelProxy()->mapToSource(
            selModel->selectedIndexes().first());
        TaggedFile *tf = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
        if (tf)
            core->setFileRating(tf, rating);
    });


    // Welcome widget — shown in the file-list area before any folder is loaded
    {
        welcome_widget_ = new QWidget(ui->verticalLayoutWidget);
        auto* outerLayout = new QVBoxLayout(welcome_widget_);
        outerLayout->setAlignment(Qt::AlignTop);
        outerLayout->setContentsMargins(40, 40, 40, 40);

        auto* inner = new QWidget;
        auto* il = new QVBoxLayout(inner);
        il->setSpacing(6);

        auto makeSectionLabel = [&](const QString &text) {
            auto* lbl = new QLabel(text);
            QFont f = lbl->font();
            f.setBold(true);
            lbl->setFont(f);
            return lbl;
        };

        // Title
        auto* title = new QLabel("compendia");
        QFont tf = title->font();
        tf.setPointSize(tf.pointSize() + 8);
        tf.setBold(true);
        title->setFont(tf);
        il->addWidget(title);

        auto* subtitle = new QLabel("Get a handle on your photos and videos!");
        il->addWidget(subtitle);
        il->addSpacing(12);

        // Get started
        il->addWidget(makeSectionLabel("Get started"));

        auto* openLink = new QLabel("<a href='open'>Open a media folder...</a>");
        openLink->setTextFormat(Qt::RichText);
        connect(openLink, &QLabel::linkActivated, this, [this](const QString &){ setRootFolder(); });
        il->addWidget(openLink);

        {
            QSettings qs(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
            QString lastPath = qs.value("folder/lastPath").toString();
            if (!lastPath.isEmpty()) {
                auto* recentLink = new QLabel(
                    QString("Reopen: <a href='reopen'>%1</a>").arg(lastPath.toHtmlEscaped()));
                recentLink->setTextFormat(Qt::RichText);
                recentLink->setWordWrap(true);
                connect(recentLink, &QLabel::linkActivated, this,
                        [this, lastPath](const QString &){ loadFolder(lastPath); });
                il->addWidget(recentLink);
            }
        }

        il->addSpacing(12);

        // How it works
        il->addWidget(makeSectionLabel("Tips"));

        const QStringList tips = {
            "Make groups of tags in the library (left). Drag and drop tags onto files to apply them.",
            "Filter your view by tag, name, folder, or date using the filter panel (top).",
            "Right-click files and choose Isolate Selection to focus on a subset.",
            "Use Autos > Face Detection to find and tag faces automatically.",
            "Tags you apply are saved alongside your images and video files.",
        };
        for (const QString &tip : tips) {
            auto* lbl = new QLabel(QString("%1  %2").arg(QChar(0x2022)).arg(tip));
            lbl->setWordWrap(true);
            il->addWidget(lbl);
        }

        outerLayout->addWidget(inner);
        ui->fileListContainer->insertWidget(0, welcome_widget_);
        ui->fileListView->hide();
    }

    // File list right-click context menu for isolation actions (only on actual items)
    ui->fileListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->fileListView, &QListView::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QModelIndex clickedProxy = ui->fileListView->indexAt(pos);
        if (!clickedProxy.isValid())
            return;
        QMenu menu(this);
        menu.addAction(ui->actionIsolateSelection);

        // Resolve the clicked item's folder so we can offer folder isolation.
        QModelIndex clickedSrc = core->getItemModelProxy()->mapToSource(clickedProxy);
        TaggedFile* clickedTf = core->getItemModel()->data(clickedSrc, Qt::UserRole + 1).value<TaggedFile*>();
        if (clickedTf) {
            QAction* isolateFolderAction = menu.addAction(QIcon(":/resources/isolate-folder.svg"), tr("Isolate this Folder"));
            const QString targetFolder = clickedTf->filePath;
            connect(isolateFolderAction, &QAction::triggered, this, [this, targetFolder]() {
                applyFolderIsolation(targetFolder);
            });

            QAction* clearAllIsolationAction = menu.addAction(
                ui->actionClearIsolation->icon(), tr("Clear Isolation"));
            clearAllIsolationAction->setEnabled(
                ui->actionClearIsolation->isEnabled() || ui->actionClearFolderIsolation->isEnabled());
            connect(clearAllIsolationAction, &QAction::triggered, this, [this]() {
                clearSelectionIsolation();
                clearFolderIsolation();
            });

            menu.addSeparator();

            QAction* drillAction = menu.addAction(QIcon(":/resources/drill-into-folder.svg"), tr("Drill to this Folder"));
            const QString drillTarget = QDir::cleanPath(clickedTf->filePath);
            connect(drillAction, &QAction::triggered, this, [this, drillTarget]() {
                drillToFolder(drillTarget);
            });

            menu.addAction(ui->actionDrillUp);

            menu.addSeparator();

            // Collect seed files (selected files with a valid pHash)
            QSet<TaggedFile*> seeds;
            const QModelIndexList selectedProxyIdxs = ui->fileListView->selectionModel()->selectedIndexes();
            for (const QModelIndex &pi : selectedProxyIdxs) {
                QModelIndex si = core->getItemModelProxy()->mapToSource(pi);
                TaggedFile* tf = core->getItemModel()->data(si, Qt::UserRole + 1).value<TaggedFile*>();
                if (tf && tf->pHash() != 0)
                    seeds.insert(tf);
            }

            QAction* findSimilarAction = menu.addAction(tr("Find Similar Images"));
            findSimilarAction->setEnabled(!seeds.isEmpty());
            connect(findSimilarAction, &QAction::triggered, this, [this, seeds]() {
                const QSet<TaggedFile*> similar = core->findSimilarTo(seeds, Compendia::SimilarImageThreshold);

                if (similar.size() == seeds.size()) {
                    QMessageBox::information(this, tr("Find Similar Images"),
                        tr("No additional similar images were found for the selected file(s)."));
                    return;
                }

                // Check which matches would be hidden by the current non-isolation filters.
                QSet<TaggedFile*> hidden;
                FilterProxyModel* proxy = core->getItemModelProxy();
                for (TaggedFile* tf : similar) {
                    if (!proxy->passesNonIsolationFilters(tf))
                        hidden.insert(tf);
                }

                if (!hidden.isEmpty()) {
                    QMessageBox dlg(this);
                    dlg.setWindowTitle(tr("Find Similar Images"));
                    dlg.setText(tr("%n similar image(s) found, but %1 would be hidden by the current filters. "
                                   "What would you like to do?",
                                   "", similar.size()).arg(hidden.size()));
                    QPushButton* okBtn = dlg.addButton(tr("Show Available"), QMessageBox::AcceptRole);
                    QPushButton* clearBtn = dlg.addButton(tr("Clear Filters"), QMessageBox::ResetRole);
                    dlg.addButton(QMessageBox::Cancel);
                    dlg.exec();

                    QAbstractButton* clicked = dlg.clickedButton();
                    if (clicked != okBtn && clicked != clearBtn)
                        return;
                    if (clicked == clearBtn)
                        on_actionClearAllFilters_triggered();
                }

                core->setIsolationSet(similar);
                ui->actionClearIsolation->setEnabled(true);
                updateFileCountLabel();
            });

            const QString fullPath = clickedTf->filePath + "/" + clickedTf->fileName;
            QAction* showInFileMgrAction = menu.addAction(tr("Open Folder in Filesystem"));
            connect(showInFileMgrAction, &QAction::triggered, this, [fullPath]() {
#ifdef Q_OS_WIN
                QProcess::startDetached("explorer.exe",
                    {"/select,", QDir::toNativeSeparators(fullPath)});
#elif defined(Q_OS_MACOS)
                QProcess::startDetached("open", {"-R", fullPath});
#else
                QDesktopServices::openUrl(
                    QUrl::fromLocalFile(QFileInfo(fullPath).absolutePath()));
#endif
            });
        }
        menu.exec(ui->fileListView->viewport()->mapToGlobal(pos));
    });

    // Isolation toolbar buttons — bind to the existing actions so enabled state is automatic.
    ui->actionIsolateSelection->setIcon(QIcon(":/resources/isolate-selection.svg"));
    ui->actionClearIsolation->setIcon(QIcon(":/resources/clear-isolation.svg"));
    ui->isolateSelectionButton->setDefaultAction(ui->actionIsolateSelection);
    ui->clearIsolationButton->setDefaultAction(ui->actionClearIsolation);
    ui->actionIsolateFolderSelection->setIcon(QIcon(":/resources/isolate-folder.svg"));
    ui->actionClearFolderIsolation->setIcon(QIcon(":/resources/clear-isolation-folder.svg"));
    ui->isolateFolderButton->setDefaultAction(ui->actionIsolateFolderSelection);
    ui->clearFolderIsolationButton->setDefaultAction(ui->actionClearFolderIsolation);
    ui->actionDrillToFolder->setIcon(QIcon(":/resources/drill-into-folder.svg"));
    ui->actionDrillUp->setIcon(QIcon(":/resources/drill-up-folder.svg"));
    ui->actionDrillToFolder->setEnabled(false);
    ui->actionDrillUp->setEnabled(false);

    // Default pane sizes
    resize(1400, 900);

    ui->windowBodySplitter->setSizes({350,650,400});
    ui->navSplitter->setSizes({100,200,600});
    ui->fileListSplitter->setSizes({700,300});
    ui->previewSplitter->setSizes({800,200});

    // Settings so multithreading does not take over all cores
    int idealThreads = QThread::idealThreadCount();
    if (idealThreads <= 4){
        QThreadPool::globalInstance()->setMaxThreadCount(idealThreads);
    } else {
        QThreadPool::globalInstance()->setMaxThreadCount(idealThreads - 2);
    }

    // Debounce timer for post-rect-adjust face cache warming.
    // Interval is tunable via Compendia::RectWarmupDelayMs in constants.h.
    rectWarmupTimer_ = new QTimer(this);
    rectWarmupTimer_->setSingleShot(true);
    rectWarmupTimer_->setInterval(Compendia::RectWarmupDelayMs);
    connect(rectWarmupTimer_, &QTimer::timeout, this, [this]() {
        if (warmupPendingFile_ && ensureFaceRecognizerLoaded())
            face_recognizer_->scheduleRectAdjustWarmup(warmupPendingFile_);
        warmupPendingFile_ = nullptr;
    });

    // Install multi-line filename delegate on the file list view
    ui->fileListView->setItemDelegate(new FileNameDelegate(core, ui->fileListView));

    // on_iconZoomSlider_valueChanged is auto-connected after setupUi sets the slider
    // value, so the initial grid height is never updated from the .ui default.
    // Apply it explicitly here to match the slider's starting position.
    on_iconZoomSlider_valueChanged(ui->iconZoomSlider->value());

    // Tag filter mode button — popup menu to choose ANY (OR) or ALL (AND)
    {
        auto *menu = new QMenu(ui->tagFilterModeButton);
        auto *actAny = menu->addAction(tr("ANY"));
        auto *actAll = menu->addAction(tr("ALL"));

        auto applyMode = [this](QAction *chosen, FilterProxyModel::TagFilterMode mode) {
            ui->tagFilterModeButton->setText(chosen->text());
            core->getItemModelProxy()->setTagFilterMode(mode);
        };

        connect(actAny, &QAction::triggered, this, [=]{ applyMode(actAny, FilterProxyModel::AnyTag);  });
        connect(actAll, &QAction::triggered, this, [=]{ applyMode(actAll, FilterProxyModel::AllTags); });

        ui->tagFilterModeButton->setMenu(menu);
        ui->tagFilterModeButton->setMaximumWidth(60);
        // background-color and text-align are set via the QSS token @NAV_BLUE;

        // Ensure the proxy starts in AnyTag mode (matching the button's default label).
        core->getItemModelProxy()->setTagFilterMode(FilterProxyModel::AnyTag);
    }
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
    QLineEdit *le = qobject_cast<QLineEdit*>(sender());
    loadFolder(le->text());
}

/*! \brief Validates that \p folder is a non-empty, existing, readable directory.
 *
 * \param folder The path to validate.
 * \return \c true if the path is safe to pass to core; \c false otherwise.
 */
bool MainWindow::validateFolder(const QString &folder)
{
    if (folder.trimmed().isEmpty())
        return false;  // silent — nothing typed, or file-picker was cancelled

    QFileInfo fi(folder);

    if (!fi.exists()) {
        QMessageBox::warning(this, "Compendia",
            QString("The folder \"%1\" does not exist.").arg(folder));
        return false;
    }

    if (!fi.isDir()) {
        QMessageBox::warning(this, "Compendia",
            QString("\"%1\" is not a folder.").arg(folder));
        return false;
    }

    if (!fi.isReadable()) {
        QMessageBox::warning(this, "Compendia",
            QString("The folder \"%1\" cannot be read. Check that you have permission to access it.").arg(folder));
        return false;
    }

    return true;
}

/*! \brief Shows the pre-release warning dialog.
 *
 * The OK button is disabled until the user checks the acknowledgement checkbox.
 * Sets preReleaseWarningAccepted_ to \c true on success so the dialog is only
 * shown once per session.
 *
 * \return \c true if the user accepted; \c false if they cancelled.
 */
bool MainWindow::confirmPreRelease()
{
    QMessageBox mb(this);
    mb.setWindowTitle("Pre-release Software");
    mb.setText("<p>Thank you for testing! You are trying a pre-release version of <i>compendia</i>. "
               "This software may behave in unexpected ways, and has only had limited testing.</p>");
    mb.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    mb.setDefaultButton(QMessageBox::Cancel);

    QPushButton* okButton = qobject_cast<QPushButton*>(mb.button(QMessageBox::Ok));
    okButton->setEnabled(false);

    QCheckBox* cb = new QCheckBox("I like to live dangerously.");
    mb.setCheckBox(cb);

    QObject::connect(cb, &QCheckBox::stateChanged, okButton, [okButton](int state) {
        okButton->setEnabled(state == Qt::Checked);
    });

    bool accepted = (mb.exec() == QMessageBox::Ok);
    if (accepted)
        preReleaseWarningAccepted_ = true;
    return accepted;
}

/*! \brief Checks for a compendiacache folder and prompts the user if none exists.
 *
 * \param folder The root folder the user is about to load.
 * \return \c true if loading should proceed; \c false if the user cancelled.
 */
bool MainWindow::confirmCacheFolder(const QString &folder)
{
    if (QDir(folder).exists(Compendia::CacheFolderName))
        return true;

    QSettings s(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia");
    if (s.value("warnings/skipCacheFolderWarning", false).toBool())
        return true;

    QMessageBox mb(this);
    mb.setWindowTitle("Compendia");
    mb.setText("<p>For performance with large libraries of images, compendia <b>makes files</b>.</p> "
               "<p>A cache folder will appear alongside the files in each of your project folders.</p>"
               "<p>When you save, a metadata file (sometimes called a <i>sidecar</i>) will be created "
               "alongside each of your images</p>"
               "<p><b>OK to make all these files?<b></p>");
    mb.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    mb.setDefaultButton(QMessageBox::Ok);

    QCheckBox* cb = new QCheckBox("Don't ask me again");
    mb.setCheckBox(cb);

    int result = mb.exec();
    if (result == QMessageBox::Ok && cb->isChecked())
        s.setValue("warnings/skipCacheFolderWarning", true);

    return result == QMessageBox::Ok;
}

/*! \brief Opens a folder-picker dialog and loads the selected folder into core. */
void MainWindow::setRootFolder()
{
    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");
    QString folder = QFileDialog::getExistingDirectory(this, "Open Folder", le->text());
    loadFolder(folder);
}

/*! \brief Validates \p folder, confirms cache creation if needed, then performs a full
 *         load: clears existing state, loads files into core, and refreshes all UI areas.
 *
 * \param folder Absolute path to the media folder to load.
 */
void MainWindow::loadFolder(const QString &folder, bool skipCacheConfirm)
{
    if (!validateFolder(folder))
        return;
    if (Compendia::ShowPreReleaseWarning && !preReleaseWarningAccepted_ && !skipCacheConfirm)
        if (!confirmPreRelease())
            return;
    if (!skipCacheConfirm && !confirmCacheFolder(folder))
        return;

    if (!skipCacheConfirm) {
        drillCeilingPath_.clear();
        updateDrillUpEnabled();
    }

    core->cancelIconGeneration();

    QLineEdit* le = ui->fileListFiltersContainer->findChild<QLineEdit*>("mediaFolderLineEdit");
    QListView* lv = ui->fileListView;

    lv->setModel(nullptr);
    ui->navFilterContainer->clear();
    ui->fileListTagAssignmentContainer->clear();
    ui->navLibraryContainer->clear();
    TagFamily::restartColorSequence();

    core->setRootDirectory(folder);

    le->setText(folder);
    le->clearFocus();
    QSettings(QSettings::IniFormat, QSettings::UserScope, "compendia", "compendia")
        .setValue("folder/lastPath", folder);

    welcome_widget_->hide();
    ui->fileListView->show();
    ui->actionClearIsolation->setEnabled(false);
    ui->actionUnreadableFiles->setEnabled(false);
    ui->actionUnreadableFiles->setIcon(QIcon());
    lv->setModel(core->getItemModelProxy());

    // Viewport-priority icon loading: keep CompendiaCore informed of what is visible.
    core->setListView(lv);
    core->updateWantedPaths();  // compute immediately for the initial view

    connect(lv->verticalScrollBar(), &QScrollBar::valueChanged,
            core, &CompendiaCore::scheduleWantedUpdate);
    connect(core->getItemModelProxy(), &QAbstractItemModel::modelReset,
            core, &CompendiaCore::scheduleWantedUpdate);
    connect(core->getItemModelProxy(), &QAbstractItemModel::rowsInserted,
            core, [this](){ core->scheduleWantedUpdate(); });

    connectFileCountLabel();
    connect(lv->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileSelectionChanged);
    lv->show();

    // Activate welcome hints now; scanFinished will dismiss them if tags are found.
    ui->navLibraryContainer->activateWelcome();
    ui->navFilterContainer->activateWelcome();
    ui->fileListTagAssignmentContainer->activateWelcome();

    sortLibraryOnNextRefresh_ = true;
    refreshNavTagLibrary();
    refreshTagAssignmentArea();
    clearPreview();
    ui->filterStarRating->setEnabled(true);
    ui->previewStarRating->setEnabled(false);
    ui->fileListStarRating->setEnabled(true);
    // Icon generation progress bar and folder stats are started from the
    // scanFinished signal handler once the file count is known.
}

/*! \brief Rebuilds the tag-library navigation area from the current library tags. */
void MainWindow::refreshNavTagLibrary(){

    // Update the display of the tag library to show all the tag families and tags

    QSet<Tag*>* libTags = core->getLibraryTags();
    ui->navLibraryContainer->refresh(libTags);

    // On the first non-empty refresh after a folder load, sort once so the
    // starting view is alphabetical even though auto-sort is disabled.
    if (sortLibraryOnNextRefresh_ && !libTags->isEmpty()) {
        sortTagLibrary();
        sortLibraryOnNextRefresh_ = false;
    }

    // Dismiss all welcome hints the moment the first tag exists, regardless of
    // which code path created it (tag dialog, face detection, drag-drop, etc.).
    if (!libTags->isEmpty()) {
        ui->navLibraryContainer->dismissWelcome();
        ui->navFilterContainer->dismissWelcome();
        ui->fileListTagAssignmentContainer->dismissWelcome();
    }

    // Enable tag-drag-drop on library family widgets and wire reassignment signal.
    const auto tfws = ui->navLibraryContainer->findChildren<TagFamilyWidget*>();
    for (TagFamilyWidget *tfw : tfws) {
        tfw->setAcceptDrops(true);
        connect(tfw, &TagFamilyWidget::tagRefamilyRequested,
                this, &MainWindow::onTagRefamilyRequested, Qt::UniqueConnection);
    }
}

/*! \brief Rebuilds the tag-assignment area from tags on the currently filtered files. */
void MainWindow::refreshTagAssignmentArea(){

    // Update the display of the tag assignment area to show all the tag families and tags that are
    // associated to files in the file list

    QSet<Tag*>* assignedTags = core->getAssignedTags_FilteredFiles();
    ui->fileListTagAssignmentContainer->refresh(assignedTags);
    refreshPreviewTagsLabel();

}

/*! \brief Alphabetizes tag families and tags in the library panel.
 *
 * Shared by the Sort button and Edit > Sort Library menu action.
 */
void MainWindow::sortTagLibrary()
{
    ui->navLibraryContainer->sort();
}

/*! \brief Rebuilds the tag-region overlays in the preview when a tag is renamed.
 *
 * Re-reads the selected file's tag rects (now carrying the updated name) and
 * pushes them to the preview container so every overlay label reflects the new name.
 *
 * \param tag The Tag whose name was changed.
 */
void MainWindow::onTagNameChanged(Tag* tag)
{
    // Promote face embeddings for the renamed tag: copy from descriptor cache →
    // known-face cache so Phase 1 of the next sweep finds hits immediately.
    if (tag && face_recognizer_) {
        QVector<PromotionEntry> entries;
        QStandardItemModel* m = core->getItemModel();
        for (int r = 0; r < m->rowCount(); ++r) {
            TaggedFile* tf = m->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
            if (!tf || !tf->tags()->contains(tag))
                continue;
            auto rect = tf->tagRect(tag);
            if (!rect.has_value())
                continue;
            entries.append({tf->filePath + "/" + tf->fileName,
                            tag->tagFamily->getName(),
                            tag->getName(),
                            rect.value()});
        }
        if (!entries.isEmpty())
            QtConcurrent::run([entries]() {
                FaceRecognizer::promoteDescriptorEmbeddings(entries);
            });
    }

    QItemSelectionModel* selModel = ui->fileListView->selectionModel();
    if (!selModel)
        return;
    QModelIndexList sel = selModel->selectedIndexes();
    if (sel.isEmpty())
        return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
    TaggedFile* tf  = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf)
        return;

    refreshPreviewTagRects(tf);
}

/*! \brief Prompts for confirmation then moves \a tag to \a newFamily via core->refamilyTag(). */
void MainWindow::onTagRefamilyRequested(Tag* tag, TagFamily* newFamily)
{
    if (!tag || !newFamily || tag->tagFamily == newFamily)
        return;

    const QString tagName       = tag->getName();
    const QString oldFamilyName = tag->tagFamily->getName();
    const QString newFamilyName = newFamily->getName();

    Tag* collision = core->getTag(newFamilyName, tagName);

    QString msg = tr("Move tag \"%1\" from \"%2\" to \"%3\"?")
                      .arg(tagName, oldFamilyName, newFamilyName);
    if (collision)
        msg += tr("\n\nA tag named \"%1\" already exists in \"%2\". The two tags will be merged.")
                   .arg(tagName, newFamilyName);

    if (QMessageBox::question(this, tr("Move Tag"), msg) != QMessageBox::Yes)
        return;

    core->refamilyTag(tag, newFamily);

    // Update known-face cache with the new family name (no-op if tag was merged away).
    if (!collision)
        onTagNameChanged(tag);
}

/*! \brief Refreshes all tag-related containers after a merge changes the library. */
void MainWindow::onTagLibraryChanged()
{
    refreshNavTagLibrary();
    refreshTagAssignmentArea();
    refreshTagFilterArea();
    onTagNameChanged(nullptr);  // refresh preview tag-region overlays
}

/*! \brief Refreshes per-file preview UI elements not covered by onTagLibraryChanged(). */
void MainWindow::onSnapshotRestored()
{
    QItemSelectionModel* sm = ui->fileListView->selectionModel();
    if (!sm || !sm->currentIndex().isValid()) return;
    QModelIndex src = core->getItemModelProxy()->mapToSource(sm->currentIndex());
    TaggedFile* tf  = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;
    ui->previewStarRating->setRating(tf->rating());
}

/*! \brief Isolates the currently selected files so only they pass the filter.
 *
 * Collects every selected file's TaggedFile pointer and passes them to the core
 * as the new isolation set.  Does nothing when the selection is empty.
 */
void MainWindow::on_actionIsolateSelection_triggered()
{
    QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
    if (sel.isEmpty())
        return;

    QSet<TaggedFile*> files;
    for (const QModelIndex &pi : sel) {
        QModelIndex src = core->getItemModelProxy()->mapToSource(pi);
        TaggedFile* tf = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
        if (tf)
            files.insert(tf);
    }
    core->setIsolationSet(files);
    ui->actionClearIsolation->setEnabled(true);
    updateFileCountLabel();
    refreshTagAssignmentArea();
}

/*! \brief Clears the selection isolation set. */
void MainWindow::on_actionClearIsolation_triggered()
{
    clearSelectionIsolation();
}

/*! \brief Sets the folder filter to the path of the selected file, narrowing the view to that subtree. */
void MainWindow::on_actionIsolateFolderSelection_triggered()
{
    QItemSelectionModel* sm = ui->fileListView->selectionModel();
    if (!sm || !sm->currentIndex().isValid())
        return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sm->currentIndex());
    TaggedFile* current = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!current)
        return;

    applyFolderIsolation(current->filePath);
}

/*! \brief Clears the folder filter set by folder isolation. */
void MainWindow::on_actionClearFolderIsolation_triggered()
{
    clearFolderIsolation();
}

/*! \brief Resets all filter controls to their default empty state. */
void MainWindow::on_actionClearAllFilters_triggered()
{
    // Tag filters
    core->clearAllTagFilters();
    refreshTagFilterArea();

    // Name and folder text fields (signals propagate to core automatically)
    ui->fileNameFilterLineEdit->clear();
    clearFolderIsolation();

    // Date filter
    ui->dateEdit->clear();

    // Rating filter — setRating() is display-only and does not emit ratingChanged(),
    // so clear the proxy directly as well.
    ui->filterStarRating->setRating(std::nullopt);
    core->setRatingFilter(std::nullopt);
    core->setRatingFilterMode(FilterProxyModel::Exactly);
    ui->ratingFilterModeButton->setText(tr("Exactly"));

    // Tag filter mode — reset to ANY
    core->getItemModelProxy()->setTagFilterMode(FilterProxyModel::AnyTag);
    ui->tagFilterModeButton->setText(tr("ANY"));

    // Isolation sets
    clearSelectionIsolation();

    // Untagged-only filter
    ui->actionUntaggedImages->setChecked(false);
    core->setUntaggedOnly(false);

    refreshTagAssignmentArea();
}

/*! \brief Isolates files that could not be opened during icon generation. */
void MainWindow::on_actionUnreadableFiles_triggered()
{
    core->isolateUnreadableFiles();
    ui->actionClearIsolation->setEnabled(true);
    updateFileCountLabel();
    refreshTagAssignmentArea();
}

/*! \brief Toggles the untagged-only filter to show only files with no tags. */
void MainWindow::on_actionUntaggedImages_triggered()
{
    bool active = ui->actionUntaggedImages->isChecked();
    core->setUntaggedOnly(active);
    updateFileCountLabel();
    refreshTagAssignmentArea();
}

bool MainWindow::isAtDrillCeiling() const
{
    return drillCeilingPath_.isEmpty()
        || QDir(core->rootDirectory()) == QDir(drillCeilingPath_);
}

void MainWindow::updateDrillUpEnabled()
{
    ui->actionDrillUp->setEnabled(!isAtDrillCeiling());
}

/*! \brief Shows a save/discard/cancel dialog when there are unsaved changes.
 *
 * \return true if navigation should proceed (changes saved or discarded),
 *         false if the user chose Cancel.
 */
bool MainWindow::confirmProceedWithUnsavedChanges()
{
    if (!core->hasUnsavedChanges())
        return true;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText("There are unsaved changes.");
    msgBox.setInformativeText("Would you like to save before navigating?");

    QPushButton *saveBtn    = msgBox.addButton("Save and Continue", QMessageBox::AcceptRole);
    QPushButton *discardBtn = msgBox.addButton("Discard Changes",   QMessageBox::DestructiveRole);
    /*QPushButton *cancelBtn =*/ msgBox.addButton("Cancel",         QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == saveBtn) {
        core->writeFileMetadata();
        return true;
    }
    return msgBox.clickedButton() == discardBtn;
}

/*! \brief Rebuilds the tag-rect overlay list for \p tf and pushes it to the preview container. */
void MainWindow::refreshPreviewTagRects(TaggedFile* tf, bool forceVisible)
{
    QList<TagRectDescriptor> tagRects;
    for (Tag* t : *tf->tags()) {
        auto r = tf->tagRect(t);
        if (r.has_value())
            tagRects.append({r.value(), t->tagFamily->getColor(), t->getName()});
    }
    ui->previewContainer->setTagRects(tagRects);
    ui->previewContainer->setTagRectsVisible(
        forceVisible || ui->showTaggedRegionsCheckbox->isChecked());
}

/*! \brief Starts the Save progress bar and writes metadata for all dirty files. */
void MainWindow::saveAll()
{
    progress_->startProcess(MultiProgressBar::Process::Save,
                            0, core->getItemModel()->rowCount(), "Saving");
    core->writeFileMetadata();
}

/*! \brief Sets the folder filter to \p folderPath and enables both clear-isolation actions. */
void MainWindow::applyFolderIsolation(const QString& folderPath)
{
    ui->folderFilterLineEdit->setText(folderPath);
    ui->actionClearFolderIsolation->setEnabled(true);
    ui->actionClearIsolation->setEnabled(true);
}

/*! \brief Clears the selection isolation set and disables its clear action. */
void MainWindow::clearSelectionIsolation()
{
    core->clearIsolationSet();
    ui->actionClearIsolation->setEnabled(false);
    updateFileCountLabel();
}

/*! \brief Clears the folder filter and disables its clear action. */
void MainWindow::clearFolderIsolation()
{
    ui->folderFilterLineEdit->setText("");
    ui->actionClearFolderIsolation->setEnabled(false);
}

/*! \brief Guards dirty state, sets drill ceiling if needed, and loads \p targetPath as the new root. */
void MainWindow::drillToFolder(const QString& targetPath)
{
    if (targetPath == QDir::cleanPath(core->rootDirectory()))
        return;
    if (!confirmProceedWithUnsavedChanges())
        return;
    if (drillCeilingPath_.isEmpty())
        drillCeilingPath_ = QDir::cleanPath(core->rootDirectory());
    loadFolder(targetPath, /*skipCacheConfirm=*/true);
    updateDrillUpEnabled();
}

/*! \brief Drills into the folder of the selected file, reloading with it as the new root. */
void MainWindow::on_actionDrillToFolder_triggered()
{
    QItemSelectionModel* sm = ui->fileListView->selectionModel();
    if (!sm || !sm->currentIndex().isValid())
        return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sm->currentIndex());
    TaggedFile* current = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!current)
        return;

    drillToFolder(QDir::cleanPath(current->filePath));
}

/*! \brief Navigates up one folder level, stopping at the drill ceiling. */
void MainWindow::on_actionDrillUp_triggered()
{
    if (isAtDrillCeiling())
        return;

    if (!confirmProceedWithUnsavedChanges())
        return;

    QDir dir(core->rootDirectory());
    dir.cdUp();
    QString parent = QDir::cleanPath(dir.absolutePath());

    // Clamp to ceiling in case the parent would exceed it.
    const QString ceiling = QDir::cleanPath(drillCeilingPath_);
    if (!parent.startsWith(ceiling))
        parent = ceiling;

    loadFolder(parent, /*skipCacheConfirm=*/true);

    if (QDir::cleanPath(core->rootDirectory()) == ceiling)
        drillCeilingPath_.clear();

    updateDrillUpEnabled();
}

/*! \brief Tags each group of similar files with "Set NN" in the "Similarity Sets" family.
 *
 * Creates or reuses tags named "Set 01", "Set 02", … (zero-padded to at least 2 digits,
 * widening automatically for 100+ groups) in the "Similarity Sets" tag family, then applies
 * the appropriate tag to every file in each group.  Refreshes the tag library and assignment
 * panels when done.
 *
 * \param groups List of file groups as returned by findSimilarImages() or groupBySimilarity().
 */
void MainWindow::applySimilaritySetTags(const QList<QList<TaggedFile*>> &groups)
{
    const int digits = qMax(2, QString::number(groups.size()).length());
    for (int i = 0; i < groups.size(); ++i) {
        const QString tagName =
            QStringLiteral("Set %1").arg(i + 1, digits, 10, QLatin1Char('0'));
        Tag* tag = core->addLibraryTag(QStringLiteral("Similarity Sets"), tagName);
        for (TaggedFile* tf : groups[i])
            tf->addTag(tag);
    }
    refreshNavTagLibrary();
    refreshTagAssignmentArea();
}

/*! \brief Finds images similar to the current selection by perceptual hash and isolates them.
 *
 * Collects selected files that carry a valid pHash as seeds, then calls
 * findSimilarTo() to locate near-duplicates across the whole project.  If any
 * results would be hidden by the current filters the user is offered a chance
 * to clear the filters or proceed with what is visible.
 */
void MainWindow::on_actionFind_Similar_In_Selection_triggered()
{
    QSet<TaggedFile*> seeds;
    const QModelIndexList selectedProxyIdxs = ui->fileListView->selectionModel()->selectedIndexes();
    for (const QModelIndex &pi : selectedProxyIdxs) {
        QModelIndex si = core->getItemModelProxy()->mapToSource(pi);
        TaggedFile* tf = core->getItemModel()->data(si, Qt::UserRole + 1).value<TaggedFile*>();
        if (tf && tf->pHash() != 0)
            seeds.insert(tf);
    }

    if (seeds.isEmpty()) return;

    const QSet<TaggedFile*> similar = core->findSimilarTo(seeds, Compendia::SimilarImageThreshold);

    if (similar.size() == seeds.size()) {
        QMessageBox::information(this, tr("Find Similar Images"),
            tr("No additional similar images were found for the selected file(s)."));
        return;
    }

    const QList<QList<TaggedFile*>> groups =
        core->groupBySimilarity(similar, Compendia::SimilarImageThreshold);

    QMessageBox ask(this);
    ask.setWindowTitle(tr("Find Similar Images"));
    ask.setText(tr("%n similar image(s) found across %1 group(s). "
                   "Would you like to tag the similar images in sets, or just display them?", "", similar.size()).arg(groups.size()));
    QPushButton* tagBtn  = ask.addButton(tr("Tag Images in Sets"), QMessageBox::YesRole);
    QPushButton* showBtn = ask.addButton(tr("Just Show Me"),    QMessageBox::NoRole);
    ask.addButton(QMessageBox::Cancel);
    ask.exec();
    if (ask.clickedButton() != tagBtn && ask.clickedButton() != showBtn)
        return;
    if (ask.clickedButton() == tagBtn)
        applySimilaritySetTags(groups);

    // Check which matches would be hidden by the current non-isolation filters.
    QSet<TaggedFile*> hidden;
    FilterProxyModel* proxy = core->getItemModelProxy();
    for (TaggedFile* tf : similar) {
        if (!proxy->passesNonIsolationFilters(tf))
            hidden.insert(tf);
    }

    if (!hidden.isEmpty()) {
        QMessageBox dlg(this);
        dlg.setWindowTitle(tr("Find Similar Images"));
        dlg.setText(tr("%n similar image(s) found, but %1 would be hidden by the current filters. "
                       "What would you like to do?",
                       "", similar.size()).arg(hidden.size()));
        QPushButton* okBtn  = dlg.addButton(tr("Show Available"), QMessageBox::AcceptRole);
        QPushButton* clearBtn = dlg.addButton(tr("Clear Filters"), QMessageBox::ResetRole);
        dlg.addButton(QMessageBox::Cancel);
        dlg.exec();

        QAbstractButton* clicked = dlg.clickedButton();
        if (clicked != okBtn && clicked != clearBtn)
            return;
        if (clicked == clearBtn)
            on_actionClearAllFilters_triggered();
    }

    core->setIsolationSet(similar);
    ui->actionClearIsolation->setEnabled(true);
    updateFileCountLabel();
}

/*! \brief Finds all near-duplicate image groups by perceptual hash and isolates them.
 *
 * Collects every file that belongs to a group of two or more near-duplicates and
 * passes them to the core as the isolation set.  Shows a status-bar summary on
 * completion, or an information dialog when no near-duplicates are found.
 */
void MainWindow::on_actionFind_Similar_Images_triggered()
{
    if (!core->containsFiles()) return;

    auto groups = core->findSimilarImages(Compendia::SimilarImageThreshold);

    if (groups.isEmpty()) {
        QMessageBox::information(this, "Find Similar Images",
                                 "No near-duplicate images were found.");
        return;
    }

    QSet<TaggedFile*> similar;
    for (const auto &g : groups)
        for (TaggedFile *tf : g)
            similar.insert(tf);

    QMessageBox ask(this);
    ask.setWindowTitle(tr("Find Similar Images"));
    ask.setText(tr("%n similar image(s) found across %1 group(s). "
                   "Would you like to tag each set?", "", similar.size()).arg(groups.size()));
    QPushButton* tagBtn  = ask.addButton(tr("Tag and Show"), QMessageBox::YesRole);
    QPushButton* showBtn = ask.addButton(tr("Show Only"),    QMessageBox::NoRole);
    ask.addButton(QMessageBox::Cancel);
    ask.exec();
    if (ask.clickedButton() != tagBtn && ask.clickedButton() != showBtn)
        return;
    if (ask.clickedButton() == tagBtn)
        applySimilaritySetTags(groups);

    core->setIsolationSet(similar);
    ui->actionClearIsolation->setEnabled(true);
    updateFileCountLabel();

    QString msg = QString("%1 similar image(s) found across %2 group(s).")
                      .arg(similar.size())
                      .arg(groups.size());
    statusBar()->showMessage(msg, 5000);
}

/*! \brief Rebuilds the tag-filter area from the currently active filter tags. */
void MainWindow::refreshTagFilterArea(){

    // Update the display of the tag filter area to show all the tag families and tags that are
    // currently set as filters

    QSet<Tag*>* filterTags = core->getFilterTags();
    ui->navFilterContainer->refresh(filterTags);

}

/*! \brief Rebuilds the previewFileTagsValue label from the currently selected file's tags. */
void MainWindow::refreshPreviewTagsLabel()
{
    QItemSelectionModel* selModel = ui->fileListView->selectionModel();
    if (!selModel || selModel->selectedIndexes().isEmpty()) {
        ui->previewFileTagsValue->setText("-");
        return;
    }

    QModelIndex src = core->getItemModelProxy()->mapToSource(
        selModel->selectedIndexes().first());
    TaggedFile* tf = core->getItemModel()->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) {
        ui->previewFileTagsValue->setText("-");
        return;
    }

    QMap<QString, QList<QString>> dict;
    for (Tag* t : *tf->tags()) {
        QString famName = t->tagFamily->getName();
        if (!dict.contains(famName))
            dict.insert(famName, QList<QString>({t->getName()}));
        else
            dict[famName].append(t->getName());
    }

    QString tagText;
    for (auto [key, value] : dict.asKeyValueRange()) {
        tagText += key + ": " + value.join(", ") + " ";
    }
    ui->previewFileTagsValue->setText(tagText.isEmpty() ? "-" : tagText.trimmed());
}

/*! \brief Updates the preview and property panel when the file-list selection changes.
 *
 * \param selected   The newly selected model indexes.
 * \param deselected The previously selected model indexes that are now deselected.
 */
void MainWindow::onFileSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {

    // Warm up known-face embeddings for the file being left.
    if (!deselected.isEmpty()) {
        QModelIndex di = deselected.indexes().first();
        QModelIndex si = core->getItemModelProxy()->mapToSource(di);
        TaggedFile* prev = core->getItemModel()->data(si, Qt::UserRole + 1).value<TaggedFile*>();

        // Cancel the pending debounce timer and immediately trigger a full warmup
        // (descriptor cache + known-face cache) for the departing file, so that
        // navigation away always completes cache warming regardless of the timer state.
        rectWarmupTimer_->stop();
        warmupPendingFile_ = nullptr;

        if (face_recognizer_)
            face_recognizer_->scheduleRectAdjustWarmup(prev);
    }

    // Use currentIndex() rather than selected.indexes().first().
    // `selected` is only the delta (newly added items), which is wrong for
    // shift-click (gives the first of the added range, not the clicked item)
    // and is empty when the navigation target was already in the selection
    // (causing ClearAndSelect to net-zero for that item and blank the preview).
    QItemSelectionModel* sm = ui->fileListView->selectionModel();
    const QModelIndex proxyIndex = sm ? sm->currentIndex() : QModelIndex();

    const bool hasSelection = sm && !sm->selectedIndexes().isEmpty();
    ui->actionIsolateSelection->setEnabled(hasSelection);
    ui->actionIsolateFolderSelection->setEnabled(proxyIndex.isValid());
    ui->actionDrillToFolder->setEnabled(proxyIndex.isValid());

    bool hasSeedInSelection = false;
    if (sm) {
        for (const QModelIndex &pi : sm->selectedIndexes()) {
            QModelIndex si = core->getItemModelProxy()->mapToSource(pi);
            TaggedFile* tf = core->getItemModel()->data(si, Qt::UserRole + 1).value<TaggedFile*>();
            if (tf && tf->pHash() != 0) { hasSeedInSelection = true; break; }
        }
    }
    ui->actionFind_Similar_In_Selection->setEnabled(hasSeedInSelection);

    if (!proxyIndex.isValid()) {
        ui->previewContainer->clear();
        ui->previewFileNameValue->setText("-");
        ui->previewFileLocationValue->setText("-");
        ui->previewImageCapturedValue->setText("-");
        ui->previewFileCreatedValue->setText("-");
        ui->previewFileModifiedValue->setText("-");
        ui->previewFileNameTimestampValue->setText("-");
        ui->previewFileTagsValue->setText("-");
        ui->previewExifValue->setText("-");
        ui->previewStarRating->setRating(std::nullopt);
        ui->previewStarRating->setEnabled(false);
    }
    else {

        // Map proxy index used by the view to source index in the model
        QModelIndex sourceIndex = core->getItemModelProxy()->mapToSource(proxyIndex);

        // Get the absolute path to the selected file
        QVariant selectedImage = core->getItemModel()->data(sourceIndex, Qt::UserRole + 1);
        TaggedFile* itemAsTaggedFile = selectedImage.value<TaggedFile*>();

        ui->previewContainer->preview(itemAsTaggedFile->filePath + "/" + itemAsTaggedFile->fileName);

        // Build tag rect overlays and push to the preview container
        refreshPreviewTagRects(itemAsTaggedFile);

        // Properties area
        QLocale locale = QLocale::system();

        ui->previewFileNameValue->setText(itemAsTaggedFile->fileName);
        ui->previewFileLocationValue->setText(itemAsTaggedFile->filePath);
        if (ui->previewImageCapturedValue != nullptr)
            ui->previewImageCapturedValue->setText(locale.toString(itemAsTaggedFile->imageCaptureDateTime, QLocale::ShortFormat));
        ui->previewFileCreatedValue->setText(locale.toString(itemAsTaggedFile->fileCreationDateTime, QLocale::ShortFormat));
        ui->previewFileModifiedValue->setText(locale.toString(itemAsTaggedFile->fileModificationDateTime, QLocale::ShortFormat));
        ui->previewFileNameTimestampValue->setText(
            itemAsTaggedFile->fileNameInferredDate.isValid()
                ? locale.toString(itemAsTaggedFile->fileNameInferredDate, QLocale::ShortFormat)
                : "-");

        refreshPreviewTagsLabel();
        ui->previewStarRating->setEnabled(true);
        ui->previewStarRating->setRating(itemAsTaggedFile->rating());

        QString exifText;
        const QMap<QString, QString> exifMap = itemAsTaggedFile->exifMap();
        for (auto [key, value] : exifMap.asKeyValueRange()) {
            exifText += key + ": " + value + "  ";
        }
        ui->previewExifValue->setText(exifText);

    }
}

/*! \brief Handles a tag drop on the preview; stores the region and refreshes overlays.
 *  \param family         Tag family name.
 *  \param tagName        Tag name.
 *  \param normalizedRect Normalized (0-1) bounding rect for the region.
 */
void MainWindow::onTagDroppedOnPreview(const QString &family,
                                       const QString &tagName,
                                       const QRectF  &normalizedRect)
{
    QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
    TaggedFile* tf  = core->getItemModel()
                          ->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;

    Tag* tag = core->getTag(family, tagName);
    if (!tag) return;

    core->checkpoint(tr("Tag region"));

    if (tf->tags()->contains(tag))
        tf->setTagRect(tag, normalizedRect);
    else
        tf->addTag(tag, normalizedRect);

    // Start (or restart) the debounce timer.  The user typically adjusts the
    // rect immediately after dropping, so we wait for idle before warming the
    // face caches — onTagRectResized will keep restarting the timer until they stop.
    warmupPendingFile_ = tf;
    rectWarmupTimer_->start();

    // Rebuild overlay list and push to preview
    refreshPreviewTagRects(tf);

    refreshTagAssignmentArea();
}

/*! \brief Handles a tag drop onto an existing tag region; replaces the old tag with the dropped one.
 *
 * The region rect is preserved; only the tag assignment changes.
 *
 *  \param family       Tag family name of the dropped tag.
 *  \param tagName      Tag name of the dropped tag.
 *  \param existingRect Normalized rect of the region being replaced.
 */
void MainWindow::onTagDroppedOnExistingRegion(const QString &family,
                                               const QString &tagName,
                                               const QRectF  &existingRect)
{
    QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
    TaggedFile* tf  = core->getItemModel()
                          ->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;

    Tag* newTag = core->getTag(family, tagName);
    if (!newTag) return;

    core->checkpoint(tr("Tag region"));

    // Find the tag currently assigned to the hit region
    Tag* oldTag = nullptr;
    for (Tag* t : *tf->tags()) {
        auto r = tf->tagRect(t);
        if (r.has_value() && r.value() == existingRect) {
            oldTag = t;
            break;
        }
    }

    if (oldTag == newTag) return;  // dropped tag onto its own region — no-op

    if (oldTag)
        tf->removeTag(oldTag);

    if (tf->tags()->contains(newTag))
        tf->setTagRect(newTag, existingRect);
    else
        tf->addTag(newTag, existingRect);

    // The rect is already final here (existing region, not adjusted), so start
    // the debounce timer to warm the face caches under the new tag name.
    warmupPendingFile_ = tf;
    rectWarmupTimer_->start();

    // Rebuild overlay list and push to preview
    refreshPreviewTagRects(tf);

    refreshTagAssignmentArea();
}

/*! \brief Persists a resized tag region back to the TaggedFile.
 *
 * Identifies the affected tag by matching \p oldNorm against stored tag rects, then
 * calls setTagRect() with \p newNorm.  No overlay rebuild is needed because the
 * TagRectItem already displays the new rect live.
 *
 *  \param oldNorm Normalized rect before the resize (used to identify the tag).
 *  \param newNorm Normalized rect after the resize.
 */
void MainWindow::onTagRectResized(const QRectF &oldNorm, const QRectF &newNorm)
{
    QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
    TaggedFile* tf  = core->getItemModel()
                          ->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;

    for (Tag* tag : *tf->tags()) {
        auto r = tf->tagRect(tag);
        if (r.has_value() && r.value() == oldNorm) {
            tf->setTagRect(tag, newNorm);
            ui->previewContainer->updateTagRectDescriptor(oldNorm, newNorm);
            break;
        }
    }

    // Restart the debounce timer on every mouse-release so the cache is warmed
    // only after the user has finished adjusting.
    warmupPendingFile_ = tf;
    rectWarmupTimer_->start();
}

/*! \brief Removes the tag region at \p normalizedRect from the selected file.
 *
 * Finds the tag whose stored rect matches \p normalizedRect, removes it from
 * the TaggedFile, rebuilds the preview overlays, and refreshes the assignment area.
 *
 * \param normalizedRect Normalized (0–1) rect identifying the tag region to delete.
 */
void MainWindow::onTagRectDeleteRequested(const QRectF &normalizedRect)
{
    QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
    TaggedFile* tf  = core->getItemModel()
                          ->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;

    for (Tag* tag : *tf->tags()) {
        auto r = tf->tagRect(tag);
        if (r.has_value() && r.value() == normalizedRect) {
            tf->removeTag(tag);
            break;
        }
    }

    // Rebuild overlay
    refreshPreviewTagRects(tf);

    refreshTagAssignmentArea();
}

/*! \brief Launches a "Find this person" search for the tag region at \p normalizedRect.
 *
 * Identifies the tag whose stored rect matches \p normalizedRect on the currently
 * selected file, prompts for scope, then runs a targeted background sweep that
 * tags matching faces across the chosen files without generating auto-face tags.
 *
 * \param normalizedRect Normalized (0–1) rect identifying the query face region.
 */
void MainWindow::onTagRectFindPersonRequested(const QRectF &normalizedRect)
{
    // Resolve the selected file and the tag that owns this rect.
    QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
    TaggedFile* tf  = core->getItemModel()
                          ->data(src, Qt::UserRole + 1).value<TaggedFile*>();
    if (!tf) return;

    Tag* queryTag = nullptr;
    for (Tag* tag : *tf->tags()) {
        auto r = tf->tagRect(tag);
        if (r.has_value() && r.value() == normalizedRect) {
            queryTag = tag;
            break;
        }
    }
    if (!queryTag) return;

    if (!ensureFaceRecognizerLoaded()) return;

    if (face_recognizer_->isSweepRunning()) {
        QMessageBox::information(this, tr("Face Search"),
            tr("A face recognition sweep is already running. Please wait for it to finish or cancel it first."));
        return;
    }

    QStandardItemModel* model = core->getItemModel();
    const int rowCount = model->rowCount();

    // Collect all TaggedFiles (main thread — safe).
    QVector<TaggedFile*> allFiles;
    allFiles.reserve(rowCount);
    for (int r = 0; r < rowCount; ++r) {
        TaggedFile* f = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (f) allFiles.append(f);
    }

    // ----- Scope selection -----
    FilterProxyModel* proxy = core->getItemModelProxy();
    const bool isFiltered   = proxy->rowCount() < model->rowCount();
    QModelIndexList selIndexes = ui->fileListView->selectionModel()->selectedIndexes();
    const bool hasSelection = !selIndexes.isEmpty();

    QVector<TaggedFile*> targetFiles;

    if (!isFiltered && !hasSelection) {
        targetFiles = allFiles;
    } else {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Find This Person"));
        msgBox.setText(tr("Search for \"%1\" in which files?").arg(queryTag->getName()));

        QPushButton *selectedBtn = hasSelection ? msgBox.addButton(tr("Selected Files"), QMessageBox::AcceptRole) : nullptr;
        QPushButton *visibleBtn  = isFiltered   ? msgBox.addButton(tr("Visible Files"),  QMessageBox::AcceptRole) : nullptr;
        QPushButton *allBtn      = msgBox.addButton(tr("All Files"), QMessageBox::AcceptRole);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();
        QAbstractButton* clicked = msgBox.clickedButton();

        if (clicked == allBtn) {
            targetFiles = allFiles;
        } else if (visibleBtn && clicked == visibleBtn) {
            for (int r = 0; r < proxy->rowCount(); ++r) {
                QModelIndex s = proxy->mapToSource(proxy->index(r, 0));
                TaggedFile* f = model->data(s, Qt::UserRole + 1).value<TaggedFile*>();
                if (f) targetFiles.append(f);
            }
        } else if (selectedBtn && clicked == selectedBtn) {
            for (const QModelIndex &pi : selIndexes) {
                QModelIndex s = proxy->mapToSource(pi);
                TaggedFile* f = model->data(s, Qt::UserRole + 1).value<TaggedFile*>();
                if (f) targetFiles.append(f);
            }
        } else {
            return; // Cancel
        }
    }

    // Build phase1Input with just the source file and the single query region.
    const QString sourceImagePath = tf->filePath + "/" + tf->fileName;
    Phase1FileInput queryInput;
    queryInput.imagePath = sourceImagePath;
    queryInput.tagRegions.append({ queryTag->tagFamily->getName(), queryTag->getName(), normalizedRect });

    // Build phase3 paths (image files only).
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    QStringList phase3Paths;
    phase3Paths.reserve(targetFiles.size());
    for (TaggedFile* f : targetFiles) {
        if (!videoExts.contains(QFileInfo(f->fileName).suffix().toLower()))
            phase3Paths.append(f->filePath + "/" + f->fileName);
    }

    face_recognizer_->startBackgroundSweep({queryInput}, phase3Paths, 0, /*suppressAutoFaces=*/true);
}

/*! \brief Refreshes the preview pane to reflect the current viewport size. */
void MainWindow::freshenPreview(){
    ui->previewContainer->freshen();
}

/*! \brief Clears the preview pane of any displayed image. */
void MainWindow::clearPreview(){
    ui->previewContainer->clear();
}

/*! \brief Moves the file list selection forward or backward by \p delta rows.
 *
 * Clamps at the first and last visible row. Clears any multi-selection before
 * moving, using the current (focused) item as the starting point.
 *
 * \param delta +1 for next file, -1 for previous.
 */
void MainWindow::navigatePreview(int delta)
{
    FilterProxyModel* proxy = core->getItemModelProxy();
    const int rowCount = proxy->rowCount();
    if (rowCount == 0) return;

    QItemSelectionModel* selModel = ui->fileListView->selectionModel();
    if (!selModel) return;

    const QModelIndex current = selModel->currentIndex();
    int newRow;
    if (!current.isValid()) {
        newRow = delta > 0 ? 0 : rowCount - 1;
    } else {
        newRow = qBound(0, current.row() + delta, rowCount - 1);
        if (newRow == current.row()) return;  // already at boundary
    }

    const QModelIndex newIndex = proxy->index(newRow, 0);
    selModel->setCurrentIndex(newIndex, QItemSelectionModel::ClearAndSelect);
    ui->fileListView->scrollTo(newIndex);
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
    if (core)
        core->scheduleWantedUpdate();
    QMainWindow::resizeEvent(event);
}

/*! \brief Slot for the Save button; writes metadata for all dirty files. */
void MainWindow::on_saveButton_clicked(){
    saveAll();
}

/*! \brief Slot for File → Save All; writes metadata for all dirty files. */
void MainWindow::on_actionSave_All_triggered(){
    saveAll();
}

/*! \brief Slot for File → Save Visible; writes metadata only for files passing the current filter. */
void MainWindow::on_actionSave_Visible_triggered(){
    progress_->startProcess(MultiProgressBar::Process::Save,
                            0, core->getItemModelProxy()->rowCount(), "Saving");
    core->writeVisibleFileMetadata();
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
    const int affected = core->countVisibleFilesWithTag(tag);
    if (affected >= Compendia::BulkUntagWarningThreshold) {
        const QString label = tag->tagFamily->getName() + ":" + tag->getName();
        const auto result = QMessageBox::warning(
            this, "Remove tag",
            QString("This will remove the tag %1 from %2 files.").arg(label).arg(affected),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (result != QMessageBox::Ok)
            return;
    }
    core->unapplyTag(tag);
    refreshTagAssignmentArea();
    onTagNameChanged(nullptr);  // refresh preview tag-region overlays if the current image was affected
}

/*! \brief Removes a tag from the active filter set when a filter-remove is requested.
 *
 * \param tag The Tag to remove from the filter.
 */
void MainWindow::on_navFilterContainer_tagDeleteRequested(Tag* tag){
    core->removeTagFilter(tag);
    refreshTagFilterArea();
    refreshTagAssignmentArea();
}

/*! \brief Deletes a tag from the library when a delete is requested from the library panel.
 *
 * If no files carry the tag it is removed silently; otherwise the user is asked to confirm.
 * \param tag The Tag whose deletion was requested.
 */
void MainWindow::onNavLibraryContainerTagDeleteRequested(Tag* tag){
    const int affected = core->countAllFilesWithTag(tag);
    if (affected > 0) {
        const QString label = tag->tagFamily->getName() + ":" + tag->getName();
        QMessageBox mb(this);
        mb.setIcon(QMessageBox::Warning);
        mb.setWindowTitle("Delete tag");
        mb.setTextFormat(Qt::RichText);
        mb.setText(QString("This will remove the tag %1 from <b>%2</b> files in the folder. Proceed?")
                       .arg(label).arg(affected));
        mb.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        mb.setDefaultButton(QMessageBox::Cancel);
        if (mb.exec() != QMessageBox::Ok)
            return;
    }
    core->deleteTagFromLibrary(tag);
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

/*! \brief Adjusts the file-list icon and grid size when the zoom slider moves.
 *
 * \param value The new slider position (0–100).
 */
void MainWindow::on_iconZoomSlider_valueChanged(int value)
{
    int iconSize = static_cast<int>(50 + value * 1.5);
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
    int shown = core->getItemModelProxy()->rowCount();
    if (core->isIsolated()) {
        int isolated = core->isolationSetSize();
        QString text = (shown == isolated)
            ? QString("%1 Files (explicit selection set)").arg(shown)
            : QString("%1 of %2 Files (explicit selection set)").arg(shown).arg(isolated);
        ui->fileCountLabel->setText(text);
    } else {
        int total = core->getItemModel()->rowCount();
        if (shown == total)
            ui->fileCountLabel->setText(QString("%1 Files").arg(total));
        else
            ui->fileCountLabel->setText(QString("%1 of %2 Files").arg(shown).arg(total));
    }
}

/*! \brief Updates the persistent folder-stats label in the status bar.
 *
 * Shows the number of images, videos, folders, and total file size for the
 * currently loaded folder. Files inside .compendia_cache directories are excluded.
 * Clears the label when no files are loaded.
 */
void MainWindow::updateFolderStatsLabel()
{
    if (!core->containsFiles()) {
        folderStatsLabel_->clear();
        return;
    }
    const auto stats = core->getFolderStats();
    const QString sizeStr = QLocale().formattedDataSize(stats.totalBytes);
    folderStatsLabel_->setText(
        QString("%1 image(s), %2 video(s) in %3 folder(s)  |  %4")
            .arg(stats.imageCount)
            .arg(stats.videoCount)
            .arg(stats.folderCount)
            .arg(sizeStr));
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

/*! \brief Advances the icon-generation progress bar when a thumbnail is ready. */
void MainWindow::onIconUpdated(){
    progress_->increment(MultiProgressBar::Process::IconGeneration);
    int v = progress_->value(MultiProgressBar::Process::IconGeneration);
    int m = progress_->max(MultiProgressBar::Process::IconGeneration);
    progress_->setLabel(MultiProgressBar::Process::IconGeneration,
                        QString("Caching icons… %L1 / %L2").arg(v).arg(m));
}

/*! \brief Advances the save progress bar when a metadata file is written. */
void MainWindow::onMetadataSaved(){
    progress_->increment(MultiProgressBar::Process::Save);
}

/*! \brief Lazily constructs face_recognizer_ and loads the DNN model files.
 *
 * Resolves the models/ directory from the application's executable directory,
 * shows a QMessageBox with download instructions if the model files are absent.
 *
 * \return \c true if the recognizer is ready; \c false if model loading failed.
 */
bool MainWindow::ensureFaceRecognizerLoaded()
{
    if (!face_recognizer_) {
        face_recognizer_ = new FaceRecognizer(this);
        connect(face_recognizer_, &FaceRecognizer::embeddingWarmedUp, this, [this]() {
            progress_->increment(MultiProgressBar::Process::EmbeddingWarmup);
        }, Qt::QueuedConnection);
        connect(face_recognizer_, &FaceRecognizer::warmupScheduled, this, [this](int misses) {
            progress_->startProcess(MultiProgressBar::Process::EmbeddingWarmup,
                                    0, misses, "Warming face cache");
        }, Qt::QueuedConnection);
        connect(face_recognizer_, &FaceRecognizer::phase1Started, this, [this](int total) {
            progress_->startProcess(MultiProgressBar::Process::EmbeddingWarmup,
                                    0, total, "Processing Known Faces");
        }, Qt::QueuedConnection);
        connect(face_recognizer_, &FaceRecognizer::phase1Progress, this, [this](int done, int total) {
            progress_->setLabel(MultiProgressBar::Process::EmbeddingWarmup,
                                QString("Processing Known Faces %1/%2").arg(done).arg(total));
            progress_->increment(MultiProgressBar::Process::EmbeddingWarmup);
        }, Qt::QueuedConnection);
        connect(face_recognizer_, &FaceRecognizer::sweepStarted, this, [this](int total) {
            progress_->startProcess(MultiProgressBar::Process::FaceDetection,
                                    0, total, "Detecting New Faces");
            ui->actionFind_Faces->setEnabled(true);
            ui->actionFind_Faces->setText(tr("Cancel Face Sweep"));
        }, Qt::QueuedConnection);
        connect(face_recognizer_, &FaceRecognizer::sweepProgress, this, [this](int done, int total) {
            progress_->setLabel(MultiProgressBar::Process::FaceDetection,
                                QString("Detecting New Faces %1/%2").arg(done).arg(total));
        }, Qt::QueuedConnection);
        connect(face_recognizer_, &FaceRecognizer::sweepFileResult, this,
            [this](const QString &path, const QVector<FaceTagAssignment> &assignments) {
                // Find the TaggedFile* by path and apply assignments on the main thread.
                QStandardItemModel* m = core->getItemModel();
                for (int r = 0; r < m->rowCount(); ++r) {
                    TaggedFile* tf = m->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
                    if (!tf)
                        continue;
                    const QString fp = tf->filePath + "/" + tf->fileName;
                    if (fp != path)
                        continue;
                    for (const FaceTagAssignment &a : assignments) {
                        Tag* tag = core->addLibraryTag(a.family, a.name);
                        if (tag)
                            tf->addTag(tag, a.rect);
                    }
                    break;
                }
                progress_->increment(MultiProgressBar::Process::FaceDetection);
            });
        auto onSweepDone = [this]() {
            progress_->finishProcess(MultiProgressBar::Process::FaceDetection);
            ui->actionFind_Faces->setEnabled(true);
            ui->actionFind_Faces->setText(tr("Find Faces"));
            refreshNavTagLibrary();
            refreshTagAssignmentArea();
            ui->showTaggedRegionsCheckbox->setChecked(true);
            QModelIndexList sel = ui->fileListView->selectionModel()->selectedIndexes();
            if (!sel.isEmpty()) {
                QModelIndex src = core->getItemModelProxy()->mapToSource(sel.first());
                TaggedFile* tf  = core->getItemModel()
                                      ->data(src, Qt::UserRole + 1).value<TaggedFile*>();
                if (tf)
                    refreshPreviewTagRects(tf, /*forceVisible=*/true);
            }
        };
        connect(face_recognizer_, &FaceRecognizer::sweepFinished, this, onSweepDone);
        connect(face_recognizer_, &FaceRecognizer::sweepCancelled, this, [this]() {
            progress_->finishProcess(MultiProgressBar::Process::FaceDetection);
            ui->actionFind_Faces->setEnabled(true);
            ui->actionFind_Faces->setText(tr("Find Faces"));
        });
    }

    if (face_recognizer_->modelsLoaded())
        return true;

    QString modelsDir = QCoreApplication::applicationDirPath() + "/models";
    if (face_recognizer_->loadModels(modelsDir))
        return true;

    QMessageBox::warning(this, "Face Recognition Models Not Found",
        "Face recognition requires two model files in the \"models\" folder "
        "next to the application executable:\n\n"
        "  shape_predictor_5_face_landmarks.dat  (~9.5 MB)\n"
        "  dlib_face_recognition_resnet_model_v1.dat  (~21 MB)\n\n"
        "Download them from http://dlib.net/files/ and place them in:\n"
        + modelsDir);
    return false;
}

/*! \brief Cross-image face recognition sweep across all loaded image files.
 *
 * Four phases:
 *   1. Seed known-person embeddings from user-labeled tag regions.
 *   2. Clear all previous auto-detected face tags from every file.
 *   3. Scan every image file: load or compute face embeddings, match against
 *      known faces (threshold 0.6), assign matched tags or create new auto tags.
 *   4. Refresh the tag library and assignment UI areas.
 */
void MainWindow::on_actionFind_Faces_triggered()
{
    // If a sweep is running, this action acts as Cancel.
    if (face_recognizer_ && face_recognizer_->isSweepRunning()) {
        face_recognizer_->cancelSweep();
        return;
    }

    if (!ensureFaceRecognizerLoaded())
        return;

    QStandardItemModel* model = core->getItemModel();
    const int rowCount = model->rowCount();

    // Collect all TaggedFiles up front (main thread — safe).
    QVector<TaggedFile*> allFiles;
    allFiles.reserve(rowCount);
    for (int r = 0; r < rowCount; ++r) {
        TaggedFile* tf = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (tf)
            allFiles.append(tf);
    }

    // ----- Scope selection -----
    FilterProxyModel* proxy = core->getItemModelProxy();
    const bool isFiltered   = proxy->rowCount() < model->rowCount();
    QModelIndexList selIndexes = ui->fileListView->selectionModel()->selectedIndexes();
    const bool hasSelection = !selIndexes.isEmpty();

    QVector<TaggedFile*> targetFiles;

    if (!isFiltered && !hasSelection) {
        targetFiles = allFiles;
    } else {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Face Detection"));
        msgBox.setText(tr("Would you like to process all files, or a selection?"));

        QPushButton *selectedBtn = hasSelection ? msgBox.addButton(tr("Selected Files"), QMessageBox::AcceptRole) : nullptr;
        QPushButton *visibleBtn  = isFiltered   ? msgBox.addButton(tr("Visible Files"),  QMessageBox::AcceptRole) : nullptr;
        QPushButton *allBtn      = msgBox.addButton(tr("All Files"),      QMessageBox::AcceptRole);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();
        QAbstractButton* clicked = msgBox.clickedButton();

        if (clicked == allBtn) {
            targetFiles = allFiles;
        } else if (visibleBtn && clicked == visibleBtn) {
            for (int r = 0; r < proxy->rowCount(); ++r) {
                QModelIndex src = proxy->mapToSource(proxy->index(r, 0));
                TaggedFile* tf = model->data(src, Qt::UserRole + 1).value<TaggedFile*>();
                if (tf) targetFiles.append(tf);
            }
        } else if (selectedBtn && clicked == selectedBtn) {
            for (const QModelIndex &pi : selIndexes) {
                QModelIndex src = proxy->mapToSource(pi);
                TaggedFile* tf = model->data(src, Qt::UserRole + 1).value<TaggedFile*>();
                if (tf) targetFiles.append(tf);
            }
        } else {
            return;  // Cancel
        }
    }

    // ----- Phase 2: clear auto tags on the main thread (fast, safe) -----
    for (TaggedFile* tf : targetFiles) {
        QSet<Tag*> toRemove;
        for (Tag* t : *tf->tags()) {
            if (t->getName().startsWith(Compendia::AutoFaceTagPrefix))
                toRemove.insert(t);
        }
        for (Tag* t : toRemove)
            tf->removeTag(t);
    }

    // ----- Extract Phase1FileInput from allFiles (main thread access to TaggedFile*) -----
    QVector<Phase1FileInput> phase1Input;
    phase1Input.reserve(allFiles.size());
    for (TaggedFile* tf : allFiles) {
        Phase1FileInput fi;
        fi.imagePath = tf->filePath + "/" + tf->fileName;
        for (Tag* tag : *tf->tags()) {
            if (tag->getName().startsWith(Compendia::AutoFaceTagPrefix))
                continue;
            auto r = tf->tagRect(tag);
            if (!r.has_value())
                continue;
            fi.tagRegions.append({ tag->tagFamily->getName(), tag->getName(), r.value() });
        }
        if (!fi.tagRegions.isEmpty())
            phase1Input.append(fi);
    }

    // ----- Extract phase3 paths (image files only) -----
    static const QStringList videoExts = {"mp4","mov","avi","mkv","wmv","webm","m4v"};
    QStringList phase3Paths;
    phase3Paths.reserve(targetFiles.size());
    for (TaggedFile* tf : targetFiles) {
        if (!videoExts.contains(QFileInfo(tf->fileName).suffix().toLower()))
            phase3Paths.append(tf->filePath + "/" + tf->fileName);
    }

    face_recognizer_->startBackgroundSweep(phase1Input, phase3Paths);
}

/*! \brief Opens the Face Recognition Settings dialog and applies any changes. */
void MainWindow::on_actionFace_Recognition_Settings_triggered()
{
    if (!ensureFaceRecognizerLoaded())
        return;

    FaceRecognitionSettingsDialog dlg(
        face_recognizer_->detectionThreshold(),
        face_recognizer_->matchThreshold(),
        this);

    if (dlg.exec() == QDialog::Accepted) {
        face_recognizer_->setDetectionThreshold(dlg.detectionThreshold());
        face_recognizer_->setMatchThreshold(dlg.matchThreshold());
    }
}

/*! \brief Removes all auto-detected face tags from every file and from the tag library after user confirmation. */
void MainWindow::on_actionRemove_Auto_Detected_Faces_triggered()
{
    int ret = QMessageBox::question(
        this,
        tr("Remove Auto Detected Faces"),
        tr("Remove all auto-detected face tags from every file?\n\nThis cannot be undone."),
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ret != QMessageBox::Ok)
        return;

    if (core->removeAutoDetectedFaceTags() == 0)
        return;

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

/*! \brief Slot — forwards creation-date filter changes to the core.
 *
 * A date equal to the widget's minimum is treated as "no filter" (null QDate).
 * \param date The newly selected date.
 */
void MainWindow::on_dateEdit_dateChanged(const QDate &date)
{
    core->setCreationDateFilter(date);  // invalid QDate = no filter
}

/*! \brief Tags every file with its capture/creation year in the "Year" tag family.
 *
 * For each file the effective date is the EXIF capture datetime when present,
 * falling back to the filesystem creation datetime.  Files with no resolvable
 * date are skipped.  The tag name is the four-digit year as a string.
 */
void MainWindow::on_actionAuto_Tag_Year_triggered()
{
    if (!core->containsFiles()) return;

    QStandardItemModel *model = core->getItemModel();
    for (int r = 0; r < model->rowCount(); ++r) {
        TaggedFile *tf = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;

        const auto hasYearTag = [](Tag *t){ return t->tagFamily->getName() == u"Year"; };
        if (std::any_of(tf->tags()->cbegin(), tf->tags()->cend(), hasYearTag)) continue;

        QDate date = tf->effectiveDate();
        if (!date.isValid()) continue;

        Tag *tag = core->addLibraryTag(QStringLiteral("Year"), QString::number(date.year()));
        tf->addTag(tag);
    }

    refreshNavTagLibrary();
    refreshTagAssignmentArea();
}

/*! \brief Tags every file with its capture/creation month name in the "Month" tag family.
 *
 * For each file the effective date is the EXIF capture datetime when present,
 * falling back to the filesystem creation datetime.  Files with no resolvable
 * date are skipped.  The tag name is the full English month name.
 */
void MainWindow::on_actionAuto_Tag_Month_triggered()
{
    if (!core->containsFiles()) return;

    QStandardItemModel *model = core->getItemModel();
    const QLocale english(QLocale::English);
    for (int r = 0; r < model->rowCount(); ++r) {
        TaggedFile *tf = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;

        const auto hasMonthTag = [](Tag *t){ return t->tagFamily->getName() == u"Month"; };
        if (std::any_of(tf->tags()->cbegin(), tf->tags()->cend(), hasMonthTag)) continue;

        QDate date = tf->effectiveDate();
        if (!date.isValid()) continue;

        QString monthName = english.monthName(date.month(), QLocale::LongFormat);
        Tag *tag = core->addLibraryTag(QStringLiteral("Month"), monthName);
        tf->addTag(tag);
    }

    refreshNavTagLibrary();
    refreshTagAssignmentArea();
}

/*! \brief Slot for Autos → Grab Video Frames; begins a frame-grab batch for all video files. */
void MainWindow::on_actionGrab_Video_Frame_triggered()
{
    if (frameGrabber_) {
        QMessageBox::information(this, tr("Frame Grab In Progress"),
            tr("A frame-grab operation is already running. Please wait for it to finish."));
        return;
    }

    if (!core->containsFiles()) {
        QMessageBox::information(this, tr("No Files Loaded"),
            tr("Open a folder first."));
        return;
    }

    static const QStringList videoExts = {"mp4", "mov", "avi", "mkv", "wmv", "webm", "m4v"};

    QStringList paths;
    QStandardItemModel *model = core->getItemModel();
    for (int r = 0; r < model->rowCount(); ++r) {
        TaggedFile *tf = model->item(r)->data(Qt::UserRole + 1).value<TaggedFile*>();
        if (!tf) continue;
        QString full = tf->filePath + "/" + tf->fileName;
        if (videoExts.contains(QFileInfo(full).suffix().toLower()))
            paths.append(full);
    }

    if (paths.isEmpty()) {
        QMessageBox::information(this, tr("No Video Files"),
            tr("No video files were found in the loaded folder."));
        return;
    }

    frameGrabber_ = new FrameGrabber(this);
    connect(frameGrabber_, &FrameGrabber::frameGrabbed,  this, &MainWindow::onVideoFrameGrabbed);
    connect(frameGrabber_, &FrameGrabber::frameFailed,   this, &MainWindow::onVideoFrameFailed);
    connect(frameGrabber_, &FrameGrabber::progress,      this, &MainWindow::onVideoGrabProgress);
    connect(frameGrabber_, &FrameGrabber::finished,      this, &MainWindow::onVideoGrabFinished);

    progress_->startProcess(MultiProgressBar::Process::VideoGrab,
                            0, paths.size(), "Grabbing video frames");

    frameGrabber_->grab(paths);
}

/*! \brief Receives a successfully captured frame and updates the in-memory icon for the file.
 *
 * Also applies any container metadata to the TaggedFile so the preview panel can display it.
 *
 * \param path  Absolute path to the video file.
 * \param frame Captured frame (scaled to at most 400×400 px).
 * \param meta  Container metadata read from the video.
 */
void MainWindow::onVideoFrameGrabbed(const QString &path, const QImage &frame,
                                     const QMap<QString, QString> &meta)
{
    QVector<QImage> images;
    for (int size : IconGenerator::kIconSizes)
        images.append(frame.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    core->updateFileIcons(path, images);
    if (!meta.isEmpty())
        core->applyVideoMetadata(path, meta);
}

/*! \brief Logs a failed frame-capture attempt.
 *
 * \param path   Absolute path to the video file.
 * \param reason Human-readable description of the failure.
 * \param meta   Container metadata (may be partially populated).
 */
void MainWindow::onVideoFrameFailed(const QString &path, const QString &reason,
                                    const QMap<QString, QString> &meta)
{
    qWarning() << "[FrameGrabber] failed:" << path << "-" << reason;
    if (!meta.isEmpty())
        core->applyVideoMetadata(path, meta);
}

/*! \brief Advances the video-grab progress bar as each file completes.
 *
 * \param done  Number of files processed so far.
 * \param total Total number of files in the batch.
 */
void MainWindow::onVideoGrabProgress(int done, int total)
{
    Q_UNUSED(done)
    Q_UNUSED(total)
    progress_->increment(MultiProgressBar::Process::VideoGrab);
}

/*! \brief Shows the grab summary in the status bar and cleans up after the batch finishes.
 *
 * \param success Number of files from which a frame was captured.
 * \param fail    Number of files for which capture failed.
 */
void MainWindow::onVideoGrabFinished(int success, int fail)
{
    ui->statusBar->showMessage(
        tr("Video frame grab: %1 succeeded, %2 failed.").arg(success).arg(fail), 8000);
    progress_->finishProcess(MultiProgressBar::Process::VideoGrab);
    frameGrabber_->deleteLater();
    frameGrabber_ = nullptr;
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
    core->cancelIconGeneration();

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

void MainWindow::on_actionDocumentation_triggered()
{
    QDesktopServices::openUrl(QUrl("https://compendia.gitbook.io/compendia"));
}

void MainWindow::on_actionAbout_triggered()
{
    auto *dlg = new AboutDialog(this);
    dlg->show();
}

/*! \brief Slot for File → Export; copies all visible files to a user-chosen folder.
 *
 * Collects every file currently passing the proxy filter, asks the user for a
 * destination folder, warns if that folder is inside the open project tree
 * (which would create duplicates), then copies each media file (no sidecars,
 * no cache files).
 */
void MainWindow::on_actionExport_triggered()
{
    FilterProxyModel *proxy = core->getItemModelProxy();
    const int count = proxy->rowCount();
    if (count == 0) {
        QMessageBox::information(this, tr("Export"), tr("There are no visible files to export."));
        return;
    }

    // Ask the user for a destination folder.
    const QString destPath = QFileDialog::getExistingDirectory(
        this,
        tr("Export Files — Choose Destination Folder"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (destPath.isEmpty())
        return;

    // Warn if the destination is inside (or equal to) the project root.
    const QString cleanRoot = QDir::cleanPath(core->rootDirectory());
    const QString cleanDest = QDir::cleanPath(destPath);
    if (!cleanRoot.isEmpty() &&
        (cleanDest == cleanRoot || cleanDest.startsWith(cleanRoot + QDir::separator()))) {
        QMessageBox mb(QMessageBox::Warning,
                       tr("Export"),
                       tr("The selected destination folder is inside the open project folder.\n\n"
                          "Exporting here would create duplicate files within the project. "
                          "Are you sure you want to continue?"),
                       QMessageBox::Ok | QMessageBox::Cancel,
                       this);
        if (mb.exec() != QMessageBox::Ok)
            return;
    }

    // Collect source paths from the proxy model.
    QStringList sourcePaths;
    sourcePaths.reserve(count);
    for (int row = 0; row < count; ++row) {
        QModelIndex proxyIdx = proxy->index(row, 0);
        QModelIndex srcIdx   = proxy->mapToSource(proxyIdx);
        auto *item = core->getItemModel()->itemFromIndex(srcIdx);
        if (!item)
            continue;
        auto *tf = qvariant_cast<TaggedFile*>(item->data(Qt::UserRole + 1));
        if (!tf || tf->filePath.isEmpty() || tf->fileName.isEmpty())
            continue;
        sourcePaths.append(tf->filePath + "/" + tf->fileName);
    }

    if (sourcePaths.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("No files were found to export."));
        return;
    }

    // Filter out cache files and build the final list of (src, dest) pairs.
    const QString cacheDirName = QStringLiteral(".compendia_cache");
    struct CopyJob { QString src; QString dest; QString fileName; };
    QList<CopyJob> jobs;
    jobs.reserve(sourcePaths.size());
    for (const QString &src : std::as_const(sourcePaths)) {
        if (src.contains(cacheDirName))
            continue;
        const QString fileName = QFileInfo(src).fileName();
        jobs.append({src, QDir(destPath).filePath(fileName), fileName});
    }

    // Check for pre-existing files at the destination.
    const int existingCount = static_cast<int>(
        std::count_if(jobs.cbegin(), jobs.cend(),
                      [](const CopyJob &j){ return QFile::exists(j.dest); }));

    bool overwrite = false;
    if (existingCount > 0) {
        QMessageBox mb(QMessageBox::Warning, tr("Export"),
                       tr("%n of these files already exist in the destination folder. "
                          "What would you like to do?", "", existingCount),
                       QMessageBox::NoButton, this);
        QPushButton *overwriteBtn = mb.addButton(tr("Overwrite Existing Files"),  QMessageBox::AcceptRole);
        QPushButton *skipBtn      = mb.addButton(tr("Skip Existing Files"),       QMessageBox::AcceptRole);
        mb.addButton(tr("Cancel Export"), QMessageBox::RejectRole);
        mb.exec();

        if (mb.clickedButton() == overwriteBtn)
            overwrite = true;
        else if (mb.clickedButton() == skipBtn)
            overwrite = false;
        else
            return; // Cancel
    }

    // Copy files.
    int copied  = 0;
    int skipped = 0;
    QStringList errors;

    for (const CopyJob &job : std::as_const(jobs)) {
        if (QFile::exists(job.dest)) {
            if (!overwrite) {
                ++skipped;
                continue;
            }
            QFile::remove(job.dest);
        }

        if (!QFile::copy(job.src, job.dest))
            errors.append(job.fileName);
        else
            ++copied;
    }

    // Report results (rich text so the folder link is clickable).
    const QString folderUrl = QUrl::fromLocalFile(destPath).toString();
    QString summary = tr("Export complete.<br><br>Copied: %1 file(s)").arg(copied);
    if (skipped > 0)
        summary += tr("<br>Skipped (already exist): %1 file(s)").arg(skipped);
    if (!errors.isEmpty())
        summary += tr("<br>Failed: %1 file(s)").arg(errors.size());
    summary += tr("<br><br><a href=\"%1\">Open Folder</a>").arg(folderUrl);

    QMessageBox mb(QMessageBox::Information, tr("Export"), summary, QMessageBox::Ok, this);
    mb.setTextFormat(Qt::RichText);
    mb.setTextInteractionFlags(Qt::TextBrowserInteraction);
    mb.exec();
}

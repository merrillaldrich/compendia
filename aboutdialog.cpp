#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "version.h"
#include <QPushButton>
#include <QPainter>
#include <QSvgRenderer>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
    resize(900, 675);
    setMinimumSize(600, 450);

    // Make the scroll area and its interior transparent so the background
    // image shows through.  QLabel's autoFillBackground is false by default,
    // but QScrollArea and its viewport paint an opaque background unless told
    // otherwise.
    ui->scrollArea->setAutoFillBackground(false);
    ui->scrollArea->viewport()->setAutoFillBackground(false);
    ui->scrollAreaWidgetContents->setAutoFillBackground(false);

    ui->versionLabel->setText(tr("Version %1").arg(APP_VERSION_STRING));
    if (QPushButton *ok = ui->buttonBox->button(QDialogButtonBox::Ok))
        ok->setIcon(QIcon());
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);
    static QSvgRenderer renderer(QStringLiteral(":/resources/about_background.svg"));
    QPainter p(this);
    // Render at the SVG's native design size (900x675), pinned to the
    // top-left corner.  When the window is smaller the image crops from the
    // right and bottom edges; when larger the image simply doesn't fill the
    // extra space.
    renderer.render(&p, QRectF(0, 0, 900, 675));
}

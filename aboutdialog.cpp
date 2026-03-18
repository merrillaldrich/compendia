#include "aboutdialog.h"
#include "version.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Luminism"));
    setMinimumWidth(420);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    auto *content = new QLabel(this);
    content->setTextFormat(Qt::RichText);
    content->setWordWrap(true);
    content->setOpenExternalLinks(false);
    content->setText(
        "<h2>Luminism</h2>"
        "<p>Version " APP_VERSION_STRING "</p>"
        "<p>A desktop application for managing and tagging media files.</p>"
        "<hr>"
        "<p><b>Third-party dependencies</b></p>"
        "<ul>"
        "<li><b>Qt 6</b> &mdash; UI framework (Widgets, Core, Concurrent, "
        "Multimedia, MultimediaWidgets)<br>"
        "Licensed under the GNU Lesser General Public License v3.</li>"
        "<li><b>dlib</b> &mdash; Face detection and recognition<br>"
        "Licensed under the Boost Software License 1.0.</li>"
        "<li><b>libexif</b> &mdash; EXIF metadata extraction<br>"
        "Licensed under the GNU Lesser General Public License v2.1.</li>"
        "<li><b>libheif</b> &mdash; HEIF/HEIC image decoding<br>"
        "Licensed under the GNU Lesser General Public License v3.</li>"
        "</ul>"
        "<hr>"
        "<p><b>License</b></p>"
        "<p>This program is free software: you can redistribute it and/or "
        "modify it under the terms of the GNU General Public License as "
        "published by the Free Software Foundation, version&nbsp;3.</p>"
        "<hr>"
        "<p>Copyright &copy; Merrill Aldrich. All rights reserved.</p>"
    );

    layout->addWidget(content);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttons);
}

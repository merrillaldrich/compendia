// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "autogen/environment.h"

#include "luminismcore.h"

int main(int argc, char *argv[])
{
    set_qt_environment();
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    LuminismCore core = LuminismCore();

    QList<TagSet> tsl = QList<TagSet>();

    TagSet ts = TagSet("People", "Susan");
    tsl.append(ts);

    TagSet ts2 = TagSet("People", "Bill");
    tsl.append(ts2);

    core.tfc->addFile("/home/foo/bar/", "myfile.jpg", tsl);
    //core.tfc->renameFamily("People","Persons");

    engine.rootContext()->setContextProperty("core", &core);

    const QUrl url(mainQmlFile);
    QObject::connect(
                &engine, &QQmlApplicationEngine::objectCreated, &app,
                [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.addImportPath(":/");
    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

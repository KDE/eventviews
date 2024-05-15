/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwindow.h"

#include <KAboutData>

#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

int main(int argc, char **argv)
{
    KAboutData about(QStringLiteral("viewerapp"),
                     i18n("ViewerApp"),
                     QStringLiteral("0.1"),
                     i18n("A test app for embedding calendarviews"),
                     KAboutLicense::GPL,
                     i18n("Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net"));
    about.addAuthor(i18nc("@info:credit", "Kevin Krammer"), QString(), QStringLiteral("krake@kdab.com"));

    QCommandLineParser parser;
    QApplication app(argc, argv);
    parser.addVersionOption();
    parser.addHelpOption();
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("+[viewname]"), i18n("Optional list of view names to instantiate")));

    QStringList viewNames;
    for (int i = 0; i < parser.positionalArguments().count(); ++i) {
        viewNames << parser.positionalArguments().at(i).toLower();
    }

    MainWindow *widget = new MainWindow(viewNames);

    widget->show();

    return app.exec();
}

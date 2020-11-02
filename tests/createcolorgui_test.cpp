/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "createcolorgui_test.h"
#include "prefs.h"

#include <KLocalizedString>
#include <QVBoxLayout>
#include <QListWidget>
#include <QApplication>
#include <KAboutData>
#include <QCommandLineParser>
#include "calendarview_debug.h"

CreateColorGui_test::CreateColorGui_test(QWidget *parent)
    : QWidget(parent)
{
    auto *vbox = new QVBoxLayout(this);
    mListWidget = new QListWidget;
    vbox->addWidget(mListWidget);
    createListWidgetItem();
}

CreateColorGui_test::~CreateColorGui_test()
{
}

void CreateColorGui_test::createListWidgetItem()
{
    EventViews::Prefs prefs;
    mListWidget->clear();
    for (int i = 0; i < 100; ++i) {
        auto *item = new QListWidgetItem;
        QColor color = prefs.resourceColor(QString::number(i));
        item->setBackground(color);
        mListWidget->addItem(item);
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("CreateColorGui_test"), i18n(
                             "CreateColorGui_test"), QStringLiteral("1.0"));
    aboutData.setShortDescription(i18n("Test creating color"));
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    auto *createColor = new CreateColorGui_test;
    createColor->resize(800, 600);
    createColor->show();

    app.exec();
    return 0;
}

/*
  Copyright (c) 2014-2020 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
    QVBoxLayout *vbox = new QVBoxLayout(this);
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
        QListWidgetItem *item = new QListWidgetItem;
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

    CreateColorGui_test *createColor = new CreateColorGui_test;
    createColor->resize(800, 600);
    createColor->show();

    app.exec();
    return 0;
}

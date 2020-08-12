/*
  SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#ifndef CREATECOLORGUI_TEST_H
#define CREATECOLORGUI_TEST_H

#include <QWidget>
class QListWidget;
class CreateColorGui_test : public QWidget
{
    Q_OBJECT
public:
    explicit CreateColorGui_test(QWidget *parent = nullptr);
    ~CreateColorGui_test();
private:
    void createListWidgetItem();
    QListWidget *mListWidget = nullptr;
};

#endif // CREATECOLORGUI_TEST_H

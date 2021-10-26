/*
  SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QWidget>
class QListWidget;
class CreateColorGui_test : public QWidget
{
    Q_OBJECT
public:
    explicit CreateColorGui_test(QWidget *parent = nullptr);
    ~CreateColorGui_test() override;

private:
    void createListWidgetItem();
    QListWidget *mListWidget = nullptr;
};


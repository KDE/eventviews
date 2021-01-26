/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <Akonadi/Calendar/ETMCalendar>
#include <QMainWindow>

namespace Akonadi
{
class IncidenceChanger;
}

namespace EventViews
{
class Prefs;
typedef QSharedPointer<Prefs> PrefsPtr;
}

class QAction;
class Settings;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const QStringList &viewNames);

    ~MainWindow();

private:
    const QStringList mViewNames;

    Ui_MainWindow mUi;

    Akonadi::ETMCalendar::Ptr mCalendar;
    Akonadi::IncidenceChanger *mIncidenceChanger = nullptr;
    Settings *mSettings = nullptr;
    EventViews::PrefsPtr *mViewPreferences = nullptr;

private:
    void addView(const QString &viewName);

private Q_SLOTS:
    void delayedInit();
    void addViewTriggered(QAction *action);
};

#endif

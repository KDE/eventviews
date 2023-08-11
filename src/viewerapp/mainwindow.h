/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ui_mainwindow.h"

#include <Akonadi/CollectionCalendar>
#include <Akonadi/ETMCalendar>

#include <QMainWindow>
#include <QVector>

namespace Akonadi
{
class IncidenceChanger;
class Collection;
}

namespace EventViews
{
class EventView;
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

private Q_SLOTS:
    void collectionSelected(const Akonadi::Collection &col);
    void collectionDeselected(const Akonadi::Collection &col);

private:
    const QStringList mViewNames;

    Ui_MainWindow mUi;

    Akonadi::ETMCalendar::Ptr mCalendar;
    Akonadi::IncidenceChanger *mIncidenceChanger = nullptr;
    Settings *mSettings = nullptr;
    EventViews::PrefsPtr *mViewPreferences = nullptr;
    QVector<EventViews::EventView *> mEventViews;
    QVector<Akonadi::CollectionCalendar::Ptr> mCalendars;

private:
    void addView(const QString &viewName);

private Q_SLOTS:
    void delayedInit();
    void addViewTriggered(QAction *action);

private:
};

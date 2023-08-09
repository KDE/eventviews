/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"

#include <Akonadi/CollectionCalendar>

#include <memory>

namespace KHolidays
{
class HolidayRegion;
}

class KCheckableProxyModel;

#include <QDateTime>

namespace EventViews
{
class EventView;

class EventViewPrivate
{
public: /// Methods
    EventViewPrivate(EventView *qq);
    ~EventViewPrivate();

    /**
      This is called when the new event dialog is shown. It sends
      all events in mTypeAheadEvents to the receiver.
     */
    void finishTypeAhead();

public: // virtual functions
    void setUpModels();

private:
    EventView *const q;

public: /// Members
    Akonadi::EntityTreeModel *etm = nullptr;
    std::unique_ptr<CalendarSupport::CollectionSelection> customCollectionSelection;
    KCheckableProxyModel *collectionSelectionModel = nullptr;

    QByteArray identifier;
    QDateTime startDateTime;
    QDateTime endDateTime;
    QDateTime actualStartDateTime;
    QDateTime actualEndDateTime;

    /* When we receive a QEvent with a key_Return release
     * we will only show a new event dialog if we previously received a
     * key_Return press, otherwise a new event dialog appears when
     * you hit return in some yes/no dialog */
    bool mReturnPressed = false;
    bool mDateRangeSelectionEnabled = true;
    bool mTypeAhead = false;
    QObject *mTypeAheadReceiver = nullptr;
    QList<QEvent *> mTypeAheadEvents;
    static CalendarSupport::CollectionSelection *sGlobalCollectionSelection;
    QVector<Akonadi::CollectionCalendar::Ptr> mCalendars;

    std::vector<std::unique_ptr<KHolidays::HolidayRegion>> mHolidayRegions;
    PrefsPtr mPrefs;
    KCalPrefsPtr mKCalPrefs;

    Akonadi::IncidenceChanger *mChanger = nullptr;
    EventView::Changes mChanges = EventView::DatesChanged;
};
} // EventViews

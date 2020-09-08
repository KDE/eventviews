/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#ifndef EVENTVIEWS_EVENTVIEW_P_H
#define EVENTVIEWS_EVENTVIEW_P_H

#include "eventview.h"

namespace KHolidays {
class HolidayRegion;
}

class KCheckableProxyModel;

#include <QDateTime>

namespace EventViews {
class EventViewPrivate
{
public: /// Methods
    EventViewPrivate();
    ~EventViewPrivate();

    /**
      This is called when the new event dialog is shown. It sends
      all events in mTypeAheadEvents to the receiver.
     */
    void finishTypeAhead();

public: // virtual functions
    void setUpModels();

public: /// Members
    Akonadi::ETMCalendar::Ptr calendar;
    CalendarSupport::CollectionSelection *customCollectionSelection = nullptr;
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

    QList<KHolidays::HolidayRegion *> mHolidayRegions;
    PrefsPtr mPrefs;
    KCalPrefsPtr mKCalPrefs;

    Akonadi::IncidenceChanger *mChanger = nullptr;
    EventView::Changes mChanges;
    Akonadi::Collection::Id mCollectionId;
};
} // EventViews

#endif

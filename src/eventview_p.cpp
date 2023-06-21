/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "eventview_p.h"
#include "calendarview_debug.h"
#include "eventview.h"
#include "prefs.h"

#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/KCalPrefs>

#include <KHolidays/HolidayRegion>

#include <KCheckableProxyModel>

#include <QApplication>

#include <ranges>

using namespace EventViews;

EventViewPrivate::EventViewPrivate(EventView *qq)
    : q(qq)
    , mPrefs(QSharedPointer<Prefs>::create())
    , mKCalPrefs(QSharedPointer<CalendarSupport::KCalPrefs>::create())
{
}

EventViewPrivate::~EventViewPrivate() = default;

void EventViewPrivate::finishTypeAhead()
{
    if (mTypeAheadReceiver) {
        for (QEvent *e : std::as_const(mTypeAheadEvents)) {
            QApplication::sendEvent(mTypeAheadReceiver, e);
        }
    }
    qDeleteAll(mTypeAheadEvents);
    mTypeAheadEvents.clear();
    mTypeAhead = false;
}

void EventViewPrivate::setUpModels()
{
    q->collectionSelection()->disconnect(q);

    customCollectionSelection.reset();
    if (collectionSelectionModel) {
        customCollectionSelection = std::make_unique<CalendarSupport::CollectionSelection>(collectionSelectionModel->selectionModel());
    }
}

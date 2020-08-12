/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "eventview_p.h"
#include "prefs.h"

#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/KCalPrefs>

#include <KCheckableProxyModel>

#include <QApplication>

using namespace EventViews;

EventViewPrivate::EventViewPrivate()
    : calendar(nullptr)
    , customCollectionSelection(nullptr)
    , collectionSelectionModel(nullptr)
    , mReturnPressed(false)
    , mDateRangeSelectionEnabled(true)
    , mTypeAhead(false)
    , mTypeAheadReceiver(nullptr)
    , mPrefs(new Prefs())
    , mKCalPrefs(new CalendarSupport::KCalPrefs())
    , mChanger(nullptr)
    , mChanges(EventView::DatesChanged)
    , mCollectionId(-1)
{
}

EventViewPrivate::~EventViewPrivate()
{
    delete collectionSelectionModel;
}

void EventViewPrivate::finishTypeAhead()
{
    if (mTypeAheadReceiver) {
        for (QEvent *e : qAsConst(mTypeAheadEvents)) {
            QApplication::sendEvent(mTypeAheadReceiver, e);
        }
    }
    qDeleteAll(mTypeAheadEvents);
    mTypeAheadEvents.clear();
    mTypeAhead = false;
}

void EventViewPrivate::setUpModels()
{
    delete customCollectionSelection;
    customCollectionSelection = nullptr;
    if (collectionSelectionModel) {
        customCollectionSelection = new CalendarSupport::CollectionSelection(
            collectionSelectionModel->selectionModel());
    }
}

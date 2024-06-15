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
#include "prefs.h"

#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/KCalPrefs>

#include <Akonadi/EntityTreeModel>

#include <KHolidays/HolidayRegion>

#include <KCheckableProxyModel>

#include <QAbstractProxyModel>
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
    if (auto cs = q->collectionSelection())
        cs->disconnect(q);

    customCollectionSelection.reset();
    if (collectionSelectionModel) {
        customCollectionSelection = std::make_unique<CalendarSupport::CollectionSelection>(collectionSelectionModel->selectionModel());
    }
}

void EventViewPrivate::setEtm(QAbstractItemModel *model)
{
    while (model) {
        if (const auto *proxy = qobject_cast<QAbstractProxyModel *>(model); proxy != nullptr) {
            model = proxy->sourceModel();
        } else if (auto *etm = qobject_cast<Akonadi::EntityTreeModel *>(model); etm != nullptr) {
            this->etm = etm;
            break;
        } else {
            model = nullptr;
        }
    }

    Q_ASSERT_X(this->etm != nullptr, "EventView", "Model is not ETM, ETM-derived or a proxy chain on top of an ETM or an ETM-derived model");
}

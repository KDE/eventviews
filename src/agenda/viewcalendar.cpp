/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#include "viewcalendar.h"
#include "agendaview.h"
#include "calendarview_debug.h"
#include "helper.h"

#include <Akonadi/CalendarUtils>
#include <Akonadi/EntityTreeModel>

#include <QAbstractProxyModel>
#include <QSharedPointer>

#include <ranges>

using namespace EventViews;

ViewCalendar::~ViewCalendar() = default;

MultiViewCalendar::~MultiViewCalendar() = default;

KCalendarCore::Calendar::Ptr MultiViewCalendar::getCalendar() const
{
    return {};
}

KCalendarCore::Incidence::List MultiViewCalendar::incidences() const
{
    KCalendarCore::Incidence::List list;
    for (const ViewCalendar::Ptr &cal : std::as_const(mSubCalendars)) {
        if (cal->getCalendar()) {
            list += cal->getCalendar()->incidences();
        }
    }
    return list;
}

int MultiViewCalendar::calendarCount() const
{
    return mSubCalendars.size();
}

Akonadi::CollectionCalendar::Ptr MultiViewCalendar::calendarForCollection(const Akonadi::Collection &collection) const
{
    return calendarForCollection(collection.id());
}

Akonadi::CollectionCalendar::Ptr MultiViewCalendar::calendarForCollection(Akonadi::Collection::Id collectionId) const
{
    const auto cal = std::find_if(mSubCalendars.begin(), mSubCalendars.end(), [collectionId](const auto &calendar) {
        if (const auto &akonadiCal = qSharedPointerDynamicCast<AkonadiViewCalendar>(calendar); akonadiCal) {
            return akonadiCal->mCalendar->collection().id() == collectionId;
        }
        return false;
    });
    if (cal == mSubCalendars.end()) {
        return {};
    }

    return qSharedPointerCast<AkonadiViewCalendar>(*cal)->mCalendar;
}

ViewCalendar::Ptr MultiViewCalendar::findCalendar(const KCalendarCore::Incidence::Ptr &incidence) const
{
    for (const ViewCalendar::Ptr &cal : std::as_const(mSubCalendars)) {
        if (cal->isValid(incidence)) {
            return cal;
        }
    }
    return {};
}

ViewCalendar::Ptr MultiViewCalendar::findCalendar(const QString &incidenceIdentifier) const
{
    for (const ViewCalendar::Ptr &cal : std::as_const(mSubCalendars)) {
        if (cal->isValid(incidenceIdentifier)) {
            return cal;
        }
    }
    return {};
}

void MultiViewCalendar::addCalendar(const ViewCalendar::Ptr &calendar)
{
    if (!mSubCalendars.contains(calendar)) {
        mSubCalendars.append(calendar);
    }
}

void MultiViewCalendar::removeCalendar(const ViewCalendar::Ptr &calendar)
{
    mSubCalendars.removeOne(calendar);
}

QString MultiViewCalendar::displayName(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr const cal = findCalendar(incidence);
    if (cal) {
        return cal->displayName(incidence);
    }
    return {};
}

QString MultiViewCalendar::iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr const cal = findCalendar(incidence);
    if (cal) {
        return cal->iconForIncidence(incidence);
    }
    return {};
}

bool MultiViewCalendar::isValid(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr const cal = findCalendar(incidence);
    return !cal.isNull();
}

bool MultiViewCalendar::isValid(const QString &incidenceIdentifier) const
{
    ViewCalendar::Ptr const cal = findCalendar(incidenceIdentifier);
    return !cal.isNull();
}

QColor MultiViewCalendar::resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr const cal = findCalendar(incidence);
    if (cal) {
        return cal->resourceColor(incidence);
    }
    return {};
}

Akonadi::Item MultiViewCalendar::item(const KCalendarCore::Incidence::Ptr &incidence) const
{
    for (auto &cal : mSubCalendars) {
        const auto &akonadiCal = qSharedPointerDynamicCast<AkonadiViewCalendar>(cal);
        if (!akonadiCal) {
            continue;
        }
        if (const auto &aItem = akonadiCal->item(incidence); aItem.isValid()) {
            return aItem;
        }
    }
    return {};
}

AkonadiViewCalendar::~AkonadiViewCalendar() = default;

bool AkonadiViewCalendar::isValid(const KCalendarCore::Incidence::Ptr &incidence) const
{
    if (!mCalendar) {
        return false;
    }

    if (item(incidence).isValid()) {
        return true;
    }
    return false;
}

bool AkonadiViewCalendar::isValid(const QString &incidenceIdentifier) const
{
    if (!mCalendar) {
        return false;
    }

    return !mCalendar->incidence(incidenceIdentifier).isNull();
}

Akonadi::Item AkonadiViewCalendar::item(const KCalendarCore::Incidence::Ptr &incidence) const
{
    if (!mCalendar || !incidence) {
        return {};
    }
    bool ok = false;
    Akonadi::Item::Id id = incidence->customProperty("VOLATILE", "AKONADI-ID").toLongLong(&ok);

    if (id == -1 || !ok) {
        id = mCalendar->item(incidence).id();

        if (id == -1) {
            // Ok, we really don't know the ID, give up.
            qCWarning(CALENDARVIEW_LOG) << "Item is invalid. uid = " << incidence->instanceIdentifier();
            return {};
        }
        return mCalendar->item(incidence->instanceIdentifier());
    }
    return mCalendar->item(id);
}

QString AkonadiViewCalendar::displayName(const KCalendarCore::Incidence::Ptr &incidence) const
{
    auto *model = mAgendaView->model();
    Akonadi::EntityTreeModel *etm = nullptr;
    while (model) {
        if (const auto *proxy = qobject_cast<QAbstractProxyModel *>(model); proxy != nullptr) {
            model = proxy->sourceModel();
        } else if (etm = qobject_cast<Akonadi::EntityTreeModel *>(model); etm != nullptr) {
            break;
        } else {
            model = nullptr;
        }
    }
    if (etm) {
        return Akonadi::CalendarUtils::displayName(etm, item(incidence).parentCollection());
    }

    return {};
}

QColor AkonadiViewCalendar::resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const
{
    Q_UNUSED(incidence);
    return EventViews::resourceColor(mCalendar->collection(), mAgendaView->preferences());
}

QString AkonadiViewCalendar::iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const
{
    return mAgendaView->iconForItem(item(incidence));
}

KCalendarCore::Calendar::Ptr AkonadiViewCalendar::getCalendar() const
{
    return mCalendar;
}

/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#include "viewcalendar.h"
#include "agendaview.h"
#include "helper.h"
#include "calendarview_debug.h"

#include <CalendarSupport/Utils>

using namespace EventViews;

ViewCalendar::~ViewCalendar()
{
}

MultiViewCalendar::~MultiViewCalendar()
{
}

KCalendarCore::Calendar::Ptr MultiViewCalendar::getCalendar() const
{
    return KCalendarCore::Calendar::Ptr();
}

KCalendarCore::Incidence::List MultiViewCalendar::incidences() const
{
    KCalendarCore::Incidence::List list;
    for (const ViewCalendar::Ptr &cal : qAsConst(mSubCalendars)) {
        if (cal->getCalendar()) {
            list += cal->getCalendar()->incidences();
        }
    }
    return list;
}

int MultiViewCalendar::calendars() const
{
    return mSubCalendars.size();
}

ViewCalendar::Ptr MultiViewCalendar::findCalendar(const KCalendarCore::Incidence::Ptr &incidence) const
{
    for (const ViewCalendar::Ptr &cal : qAsConst(mSubCalendars)) {
        if (cal->isValid(incidence)) {
            return cal;
        }
    }
    return ViewCalendar::Ptr();
}

ViewCalendar::Ptr MultiViewCalendar::findCalendar(const QString &incidenceIdentifier) const
{
    for (const ViewCalendar::Ptr &cal : qAsConst(mSubCalendars)) {
        if (cal->isValid(incidenceIdentifier)) {
            return cal;
        }
    }
    return ViewCalendar::Ptr();
}

void MultiViewCalendar::addCalendar(const ViewCalendar::Ptr &calendar)
{
    if (!mSubCalendars.contains(calendar)) {
        mSubCalendars.append(calendar);
    }
}

void MultiViewCalendar::setETMCalendar(const Akonadi::ETMCalendar::Ptr &calendar)
{
    if (!mETMCalendar) {
        mETMCalendar = AkonadiViewCalendar::Ptr(new AkonadiViewCalendar);
        mETMCalendar->mAgendaView = mAgendaView;
    }
    mETMCalendar->mCalendar = calendar;
    addCalendar(mETMCalendar);
}

QString MultiViewCalendar::displayName(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr cal = findCalendar(incidence);
    if (cal) {
        return cal->displayName(incidence);
    }
    return QString();
}

QString MultiViewCalendar::iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr cal = findCalendar(incidence);
    if (cal) {
        return cal->iconForIncidence(incidence);
    }
    return QString();
}

bool MultiViewCalendar::isValid(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr cal = findCalendar(incidence);
    return cal;
}

bool MultiViewCalendar::isValid(const QString &incidenceIdentifier) const
{
    ViewCalendar::Ptr cal = findCalendar(incidenceIdentifier);
    return cal;
}

QColor MultiViewCalendar::resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const
{
    ViewCalendar::Ptr cal = findCalendar(incidence);
    if (cal) {
        return cal->resourceColor(incidence);
    }
    return {};
}

Akonadi::Item MultiViewCalendar::item(const KCalendarCore::Incidence::Ptr &incidence) const
{
    if (mETMCalendar->isValid(incidence)) {
        return mETMCalendar->item(incidence);
    }

    return Akonadi::Item();
}

AkonadiViewCalendar::~AkonadiViewCalendar()
{
}

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
        return Akonadi::Item();
    }
    bool ok = false;
    Akonadi::Item::Id id = incidence->customProperty("VOLATILE", "AKONADI-ID").toLongLong(&ok);

    if (id == -1 || !ok) {
        id = mCalendar->item(incidence).id();

        if (id == -1) {
            // Ok, we really don't know the ID, give up.
            qCWarning(CALENDARVIEW_LOG) << "Item is invalid. uid = "
                                        << incidence->instanceIdentifier();
            return Akonadi::Item();
        }
        return mCalendar->item(incidence->instanceIdentifier());
    }
    return mCalendar->item(id);
}

QString AkonadiViewCalendar::displayName(const KCalendarCore::Incidence::Ptr &incidence) const
{
    return CalendarSupport::displayName(mCalendar.data(), item(incidence).parentCollection());
}

QColor AkonadiViewCalendar::resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const
{
    return EventViews::resourceColor(item(incidence), mAgendaView->preferences());
}

QString AkonadiViewCalendar::iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const
{
    return mAgendaView->iconForItem(item(incidence));
}

KCalendarCore::Calendar::Ptr AkonadiViewCalendar::getCalendar() const
{
    return mCalendar;
}

/*
  SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "timelineview_p.h"
#include "timelineitem.h"

#include <KGantt/KGanttGraphicsView>

#include <Akonadi/Calendar/ETMCalendar>
#include <Akonadi/Calendar/IncidenceChanger>
#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/Utils>
#include <KCalendarCore/OccurrenceIterator>

#include "calendarview_debug.h"

#include <QStandardItemModel>

using namespace KCalendarCore;
using namespace EventViews;

TimelineViewPrivate::TimelineViewPrivate(TimelineView *parent)
    : q(parent)
{
}

TimelineViewPrivate::~TimelineViewPrivate()
{
}

void TimelineViewPrivate::itemSelected(const QModelIndex &index)
{
    auto tlitem = dynamic_cast<TimelineSubItem *>(static_cast<QStandardItemModel *>(mGantt->model())->item(index.row(), index.column()));
    if (tlitem) {
        Q_EMIT q->incidenceSelected(tlitem->incidence(), tlitem->originalStart().date());
    }
}

void TimelineViewPrivate::itemDoubleClicked(const QModelIndex &index)
{
    auto tlitem = dynamic_cast<TimelineSubItem *>(static_cast<QStandardItemModel *>(mGantt->model())->item(index.row(), index.column()));
    if (tlitem) {
        Q_EMIT q->editIncidenceSignal(tlitem->incidence());
    }
}

void TimelineViewPrivate::contextMenuRequested(QPoint point)
{
    QPersistentModelIndex index = mGantt->indexAt(point);
    // mHintDate = QDateTime( mGantt->getDateTimeForCoordX( QCursor::pos().x(), true ) );
    auto tlitem = dynamic_cast<TimelineSubItem *>(static_cast<QStandardItemModel *>(mGantt->model())->item(index.row(), index.column()));
    if (!tlitem) {
        Q_EMIT q->showNewEventPopupSignal();
        mSelectedItemList = Akonadi::Item::List();
    } else {
        Q_EMIT q->showIncidencePopupSignal(tlitem->incidence(), CalendarSupport::incidence(tlitem->incidence())->dtStart().date());

        mSelectedItemList << tlitem->incidence();
    }
}

// slot
void TimelineViewPrivate::newEventWithHint(const QDateTime &dt)
{
    mHintDate = dt;
    Q_EMIT q->newEventSignal(dt);
}

TimelineItem *TimelineViewPrivate::calendarItemForIncidence(const Akonadi::Item &incidence)
{
    Akonadi::ETMCalendar::Ptr calres = q->calendar();
    TimelineItem *item = nullptr;
    if (!calres) {
        item = mCalendarItemMap.value(-1);
    } else {
        item = mCalendarItemMap.value(incidence.parentCollection().id());
    }
    return item;
}

void TimelineViewPrivate::insertIncidence(const Akonadi::Item &aitem, QDate day)
{
    const Incidence::Ptr incidence = CalendarSupport::incidence(aitem);
    // qCDebug(CALENDARVIEW_LOG) << "Item " << aitem.id() << " parentcollection: " << aitem.parentCollection().id();
    TimelineItem *item = calendarItemForIncidence(aitem);
    if (!item) {
        qCWarning(CALENDARVIEW_LOG) << "Help! Something is really wrong here!";
        return;
    }

    if (incidence->recurs()) {
        KCalendarCore::OccurrenceIterator occurIter(*(q->calendar()), incidence, QDateTime(day, QTime(0, 0, 0)), QDateTime(day, QTime(23, 59, 59)));
        while (occurIter.hasNext()) {
            occurIter.next();
            const Akonadi::Item akonadiItem = q->calendar()->item(occurIter.incidence());
            const QDateTime startOfOccurrence = occurIter.occurrenceStartDate();
            const QDateTime endOfOccurrence = occurIter.incidence()->endDateForStart(startOfOccurrence);
            item->insertIncidence(akonadiItem, startOfOccurrence.toLocalTime(), endOfOccurrence.toLocalTime());
        }
    } else {
        if (incidence->dtStart().date() == day || incidence->dtStart().date() < mStartDate) {
            item->insertIncidence(aitem);
        }
    }
}

void TimelineViewPrivate::insertIncidence(const Akonadi::Item &incidence)
{
    const Event::Ptr event = CalendarSupport::event(incidence);
    if (!event) {
        return;
    }

    if (event->recurs()) {
        insertIncidence(incidence, QDate());
    }

    for (QDate day = mStartDate; day <= mEndDate; day = day.addDays(1)) {
        const KCalendarCore::Event::List events =
            q->calendar()->events(day, QTimeZone::systemTimeZone(), KCalendarCore::EventSortStartDate, KCalendarCore::SortDirectionAscending);
        if (events.contains(event)) {
            // PENDING(AKONADI_PORT) check if correct. also check the original if,
            // was inside the for loop (unnecessarily)
            for (const KCalendarCore::Event::Ptr &i : events) {
                Akonadi::Item item = q->calendar()->item(i);
                insertIncidence(item, day);
            }
        }
    }
}

void TimelineViewPrivate::removeIncidence(const Akonadi::Item &incidence)
{
    TimelineItem *item = calendarItemForIncidence(incidence);
    if (item) {
        item->removeIncidence(incidence);
    } else {
#if 0 // AKONADI_PORT_DISABLED
      // try harder, the incidence might already be removed from the resource
        typedef QMap<QString, KOrg::TimelineItem *> M2_t;
        typedef QMap<KCalendarCore::ResourceCalendar *, M2_t> M1_t;
        for (M1_t::ConstIterator it1 = d->mCalendarItemMap.constBegin();
             it1 != mCalendarItemMap.constEnd(); ++it1) {
            for (M2_t::ConstIterator it2 = it1.value().constBegin();
                 it2 != it1.value().constEnd(); ++it2) {
                if (it2.value()) {
                    it2.value()->removeIncidence(incidence);
                }
            }
        }
#endif
    }
}

void TimelineViewPrivate::itemChanged(QStandardItem *item)
{
    auto tlit = dynamic_cast<TimelineSubItem *>(item);
    if (!tlit) {
        return;
    }

    const Akonadi::Item i = tlit->incidence();
    const Incidence::Ptr inc = CalendarSupport::incidence(i);

    QDateTime newStart(tlit->startTime());
    if (inc->allDay()) {
        newStart = QDateTime(newStart.date().startOfDay());
    }

    int delta = tlit->originalStart().secsTo(newStart);
    inc->setDtStart(inc->dtStart().addSecs(delta));
    int duration = tlit->startTime().secsTo(tlit->endTime());
    int allDayOffset = 0;
    if (inc->allDay()) {
        int secsPerDay = 60 * 60 * 24;
        duration /= secsPerDay;
        duration *= secsPerDay;
        allDayOffset = secsPerDay;
        duration -= allDayOffset;
        if (duration < 0) {
            duration = 0;
        }
    }
    inc->setDuration(duration);
    TimelineItem *parent = tlit->parent();
    parent->moveItems(i, tlit->originalStart().secsTo(newStart), duration + allDayOffset);
}

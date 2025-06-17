/*
  SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "timelineview_p.h"
#include "timelineitem.h"

#include <KGanttGraphicsView>

#include <Akonadi/CalendarUtils>
#include <Akonadi/IncidenceChanger>
#include <CalendarSupport/CollectionSelection>
#include <KCalendarCore/OccurrenceIterator>

#include "calendarview_debug.h"

#include <QStandardItemModel>

using namespace KCalendarCore;
using namespace EventViews;

TimelineViewPrivate::TimelineViewPrivate(TimelineView *parent)
    : q(parent)
{
}

TimelineViewPrivate::~TimelineViewPrivate() = default;

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
    const QPersistentModelIndex index = mGantt->indexAt(point);
    // mHintDate = QDateTime( mGantt->getDateTimeForCoordX( QCursor::pos().x(), true ) );
    auto tlitem = dynamic_cast<TimelineSubItem *>(static_cast<QStandardItemModel *>(mGantt->model())->item(index.row(), index.column()));
    if (!tlitem) {
        Q_EMIT q->showNewEventPopupSignal();
        mSelectedItemList = Akonadi::Item::List();
    } else {
        if (const auto calendar = tlitem->parent()->calendar(); calendar) {
            Q_EMIT q->showIncidencePopupSignal(calendar, tlitem->incidence(), Akonadi::CalendarUtils::incidence(tlitem->incidence())->dtStart().date());
        }

        mSelectedItemList << tlitem->incidence();
    }
}

// slot
void TimelineViewPrivate::newEventWithHint(const QDateTime &dt)
{
    mHintDate = dt;
    Q_EMIT q->newEventSignal(dt);
}

TimelineItem *TimelineViewPrivate::calendarItemForIncidence(const Akonadi::Item &item) const
{
    auto timelineItem = mCalendarItemMap.constFind(item.parentCollection().id());
    if (timelineItem == mCalendarItemMap.cend()) {
        return mCalendarItemMap.value(-1);
    }

    return *timelineItem;
}

void TimelineViewPrivate::insertIncidence(const Akonadi::CollectionCalendar::Ptr &calendar, const Akonadi::Item &item, QDate day)
{
    const Incidence::Ptr incidence = Akonadi::CalendarUtils::incidence(item);
    TimelineItem *timelineItem = calendarItemForIncidence(item);
    if (!timelineItem) {
        qCWarning(CALENDARVIEW_LOG) << "Help! Something is really wrong here!";
        return;
    }

    if (incidence->recurs()) {
        KCalendarCore::OccurrenceIterator occurIter(*calendar, incidence, QDateTime(day, QTime(0, 0, 0)), QDateTime(day, QTime(23, 59, 59)));
        while (occurIter.hasNext()) {
            occurIter.next();
            const Akonadi::Item akonadiItem = calendar->item(occurIter.incidence());
            const QDateTime startOfOccurrence = occurIter.occurrenceStartDate();
            const QDateTime endOfOccurrence = occurIter.incidence()->endDateForStart(startOfOccurrence);
            timelineItem->insertIncidence(akonadiItem, startOfOccurrence.toLocalTime(), endOfOccurrence.toLocalTime());
        }
    } else {
        if (incidence->dtStart().date() == day || incidence->dtStart().date() < mStartDate) {
            timelineItem->insertIncidence(item);
        }
    }
}

void TimelineViewPrivate::insertIncidence(const Akonadi::CollectionCalendar::Ptr &calendar, const Akonadi::Item &item)
{
    const Event::Ptr event = Akonadi::CalendarUtils::event(item);
    if (!event) {
        return;
    }

    if (event->recurs()) {
        insertIncidence(calendar, item, QDate());
    }

    for (QDate day = mStartDate; day <= mEndDate; day = day.addDays(1)) {
        const auto events = calendar->events(day, QTimeZone::systemTimeZone(), KCalendarCore::EventSortStartDate, KCalendarCore::SortDirectionAscending);
        if (events.contains(event)) {
            // PENDING(AKONADI_PORT) check if correct. also check the original if,
            // was inside the for loop (unnecessarily)
            for (const KCalendarCore::Event::Ptr &i : events) {
                const Akonadi::Item item = calendar->item(i);
                insertIncidence(calendar, item, day);
            }
        }
    }
}

void TimelineViewPrivate::removeIncidence(const Akonadi::Item &item)
{
    TimelineItem *timelineItem = calendarItemForIncidence(item);
    if (timelineItem) {
        timelineItem->removeIncidence(item);
    } else {
#ifdef AKONADI_PORT_DISABLED
        // try harder, the incidence might already be removed from the resource
        typedef QMap<QString, KOrg::TimelineItem *> M2_t;
        typedef QMap<KCalendarCore::ResourceCalendar *, M2_t> M1_t;
        for (M1_t::ConstIterator it1 = d->mCalendarItemMap.constBegin(); it1 != mCalendarItemMap.constEnd(); ++it1) {
            for (M2_t::ConstIterator it2 = it1.value().constBegin(); it2 != it1.value().constEnd(); ++it2) {
                if (it2.value()) {
                    it2.value()->removeIncidence(item);
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
    const Incidence::Ptr inc = Akonadi::CalendarUtils::incidence(i);

    QDateTime newStart(tlit->startTime());
    if (inc->allDay()) {
        newStart = QDateTime(newStart.date().startOfDay());
    }

    const int delta = tlit->originalStart().secsTo(newStart);
    inc->setDtStart(inc->dtStart().addSecs(delta));
    int duration = tlit->startTime().secsTo(tlit->endTime());
    int allDayOffset = 0;
    if (inc->allDay()) {
        const int secsPerDay = 60 * 60 * 24;
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

#include "moc_timelineview_p.cpp"

/*
  Copyright (c) 2007 Volker Krause <vkrause@kde.org>
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Andras Mantia <andras@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "timelineitem.h"

#ifdef KDIAGRAM_SUPPORT
#include <KGantt/KGanttGlobal>
#else
#include <KDGantt2/KDGanttGlobal>
#endif
#include <CalendarSupport/KCalPrefs>

#include <CalendarSupport/Utils>

#include <KCalCore/Incidence>
#include <KCalUtils/IncidenceFormatter>

using namespace KCalCore;
using namespace KCalUtils;
using namespace EventViews;

TimelineItem::TimelineItem(const Akonadi::ETMCalendar::Ptr &calendar, uint index,
                           QStandardItemModel *model, QObject *parent)
    : QObject(parent), mCalendar(calendar), mModel(model), mIndex(index)
{
    mModel->removeRow(mIndex);
    QStandardItem *dummyItem = new QStandardItem;
#ifdef KDIAGRAM_SUPPORT
    dummyItem->setData(KGantt::TypeTask, KGantt::ItemTypeRole);
#else
    dummyItem->setData(KDGantt::TypeTask, KDGantt::ItemTypeRole);
#endif
    mModel->insertRow(mIndex, dummyItem);
}

void TimelineItem::insertIncidence(const Akonadi::Item &aitem,
                                   const KDateTime &_start, const KDateTime &_end)
{
    const Incidence::Ptr incidence = CalendarSupport::incidence(aitem);
    KDateTime start(_start);
    KDateTime end(_end);
    if (!start.isValid()) {
        start = incidence->dtStart().toTimeSpec(CalendarSupport::KCalPrefs::instance()->timeSpec());
    }
    if (!end.isValid()) {
        end = incidence->dateTime(Incidence::RoleEnd).toTimeSpec(
                  CalendarSupport::KCalPrefs::instance()->timeSpec());
    }
    if (incidence->allDay()) {
        end = end.addDays(1);
    }

    typedef QList<QStandardItem *> ItemList;
    ItemList list = mItemMap.value(aitem.id());
    for (ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        if (KDateTime(static_cast<TimelineSubItem *>(*it)->startTime()) == start &&
                KDateTime(static_cast<TimelineSubItem *>(*it)->endTime()) == end) {
            return;
        }
    }

    TimelineSubItem *item = new TimelineSubItem(mCalendar, aitem, this);

    item->setStartTime(start.dateTime());
    item->setOriginalStart(start);
    item->setEndTime(end.dateTime());
    item->setData(mColor, Qt::DecorationRole);

    list = mModel->takeRow(mIndex);

    mItemMap[aitem.id()].append(item);

    list.append(mItemMap[aitem.id()]);

    mModel->insertRow(mIndex, list);
}

void TimelineItem::removeIncidence(const Akonadi::Item &incidence)
{
    qDeleteAll(mItemMap.value(incidence.id()));
    mItemMap.remove(incidence.id());
}

void TimelineItem::moveItems(const Akonadi::Item &incidence, int delta, int duration)
{
    typedef QList<QStandardItem *> ItemList;
    ItemList list = mItemMap.value(incidence.id());
    for (ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        QDateTime start = static_cast<TimelineSubItem *>(*it)->originalStart().dateTime();
        start = start.addSecs(delta);
        static_cast<TimelineSubItem *>(*it)->setStartTime(start);
        static_cast<TimelineSubItem *>(*it)->setOriginalStart(KDateTime(start));
        static_cast<TimelineSubItem *>(*it)->setEndTime(start.addSecs(duration));
    }
}

void TimelineItem::setColor(const QColor &color)
{
    mColor = color;
}

TimelineSubItem::TimelineSubItem(const Akonadi::ETMCalendar::Ptr &calendar,
                                 const Akonadi::Item &incidence, TimelineItem *parent)
    : QStandardItem(), mCalendar(calendar), mIncidence(incidence),
      mParent(parent), mToolTipNeedsUpdate(true)
{
#ifdef KDIAGRAM_SUPPORT
    setData(KGantt::TypeTask, KGantt::ItemTypeRole);
#else
    setData(KDGantt::TypeTask, KDGantt::ItemTypeRole);
#endif
    if (!CalendarSupport::incidence(incidence)->isReadOnly()) {
        setFlags(Qt::ItemIsSelectable);
    }
}

TimelineSubItem::~TimelineSubItem()
{
}

void TimelineSubItem::setStartTime(const QDateTime &dt)
{
#ifdef KDIAGRAM_SUPPORT
    setData(dt, KGantt::StartTimeRole);
#else
    setData(dt, KDGantt::StartTimeRole);
#endif
}

QDateTime TimelineSubItem::startTime() const
{
#ifdef KDIAGRAM_SUPPORT
    return data(KGantt::StartTimeRole).toDateTime();
#else
    return data(KDGantt::StartTimeRole).toDateTime();
#endif
}

void TimelineSubItem::setEndTime(const QDateTime &dt)
{
#ifdef KDIAGRAM_SUPPORT
    setData(dt, KGantt::EndTimeRole);
#else
    setData(dt, KDGantt::EndTimeRole);
#endif
}

QDateTime TimelineSubItem::endTime() const
{
#ifdef KDIAGRAM_SUPPORT
    return data(KGantt::EndTimeRole).toDateTime();
#else
    return data(KDGantt::EndTimeRole).toDateTime();
#endif
}

void TimelineSubItem::updateToolTip()
{
    if (!mToolTipNeedsUpdate) {
        return;
    }

    mToolTipNeedsUpdate = false;

    setData(IncidenceFormatter::toolTipStr(
                CalendarSupport::displayName(mCalendar.data(), mIncidence.parentCollection()),
                CalendarSupport::incidence(mIncidence), originalStart().date(),
                true, CalendarSupport::KCalPrefs::instance()->timeSpec()), Qt::ToolTipRole);
}

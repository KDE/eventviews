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

#include <KGantt/KGanttGlobal>

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
    dummyItem->setData(KGantt::TypeTask, KGantt::ItemTypeRole);
    mModel->insertRow(mIndex, dummyItem);
}

void TimelineItem::insertIncidence(const Akonadi::Item &aitem,
                                   const QDateTime &_start, const QDateTime &_end)
{
    const Incidence::Ptr incidence = CalendarSupport::incidence(aitem);
    QDateTime start(_start);
    QDateTime end(_end);
    if (!start.isValid()) {
        start = incidence->dtStart().toLocalZone().dateTime();
    }
    if (!end.isValid()) {
        end = incidence->dateTime(Incidence::RoleEnd).toLocalZone().dateTime();
    }
    if (incidence->allDay()) {
        end = end.addDays(1);
    }

    typedef QList<QStandardItem *> ItemList;
    ItemList list = mItemMap.value(aitem.id());
    for (ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        if (static_cast<TimelineSubItem *>(*it)->startTime() == start &&
                static_cast<TimelineSubItem *>(*it)->endTime() == end) {
            return;
        }
    }

    TimelineSubItem *item = new TimelineSubItem(mCalendar, aitem, this);

    item->setStartTime(start);
    item->setOriginalStart(start);
    item->setEndTime(end);
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
    const ItemList::ConstIterator end(list.constEnd());
    for (ItemList::ConstIterator it = list.constBegin(); it != end; ++it) {
        QDateTime start = static_cast<TimelineSubItem *>(*it)->originalStart();
        start = start.addSecs(delta);
        static_cast<TimelineSubItem *>(*it)->setStartTime(start);
        static_cast<TimelineSubItem *>(*it)->setOriginalStart(start);
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
    setData(KGantt::TypeTask, KGantt::ItemTypeRole);
    if (!CalendarSupport::incidence(incidence)->isReadOnly()) {
        setFlags(Qt::ItemIsSelectable);
    }
}

TimelineSubItem::~TimelineSubItem()
{
}

void TimelineSubItem::setStartTime(const QDateTime &dt)
{
    setData(dt, KGantt::StartTimeRole);
}

QDateTime TimelineSubItem::startTime() const
{
    return data(KGantt::StartTimeRole).toDateTime();
}

void TimelineSubItem::setEndTime(const QDateTime &dt)
{
    setData(dt, KGantt::EndTimeRole);
}

QDateTime TimelineSubItem::endTime() const
{
    return data(KGantt::EndTimeRole).toDateTime();
}

void TimelineSubItem::updateToolTip()
{
    if (!mToolTipNeedsUpdate) {
        return;
    }

    mToolTipNeedsUpdate = false;

    setData(IncidenceFormatter::toolTipStr(
                CalendarSupport::displayName(mCalendar.data(), mIncidence.parentCollection()),
                CalendarSupport::incidence(mIncidence), originalStart().date(), true), Qt::ToolTipRole);
}

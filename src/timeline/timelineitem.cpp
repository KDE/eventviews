/*
  SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "timelineitem.h"

#include <Akonadi/CalendarUtils>

#include <KGanttGlobal>

#include <KCalUtils/IncidenceFormatter>

using namespace KCalendarCore;
using namespace KCalUtils;
using namespace EventViews;

TimelineItem::TimelineItem(const Akonadi::CollectionCalendar::Ptr &calendar, uint index, QStandardItemModel *model, QObject *parent)
    : QObject(parent)
    , mCalendar(calendar)
    , mModel(model)
    , mIndex(index)
{
    mModel->removeRow(mIndex);
    auto dummyItem = new QStandardItem;
    dummyItem->setData(KGantt::TypeTask, KGantt::ItemTypeRole);
    mModel->insertRow(mIndex, dummyItem);
}

TimelineItem::~TimelineItem()
{
    mModel->removeRow(mIndex);
}

void TimelineItem::insertIncidence(const Akonadi::Item &aitem, const QDateTime &_start, const QDateTime &_end)
{
    const Incidence::Ptr incidence = Akonadi::CalendarUtils::incidence(aitem);
    QDateTime start(_start);
    QDateTime end(_end);
    if (!start.isValid()) {
        start = incidence->dtStart().toLocalTime();
    }
    if (!end.isValid()) {
        end = incidence->dateTime(Incidence::RoleEnd).toLocalTime();
    }
    if (incidence->allDay()) {
        end = end.addDays(1);
    }

    using ItemList = QList<QStandardItem *>;
    ItemList list = mItemMap.value(aitem.id());
    for (ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        if (static_cast<TimelineSubItem *>(*it)->startTime() == start && static_cast<TimelineSubItem *>(*it)->endTime() == end) {
            return;
        }
    }

    auto item = new TimelineSubItem(aitem, this);

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
    using ItemList = QList<QStandardItem *>;
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

Akonadi::CollectionCalendar::Ptr TimelineItem::calendar() const
{
    return mCalendar;
}

TimelineSubItem::TimelineSubItem(const Akonadi::Item &incidence, TimelineItem *parent)
    : mIncidence(incidence)
    , mParent(parent)
    , mToolTipNeedsUpdate(true)
{
    setData(KGantt::TypeTask, KGantt::ItemTypeRole);
    if (!Akonadi::CalendarUtils::incidence(incidence)->isReadOnly()) {
        setFlags(Qt::ItemIsSelectable);
    }
}

TimelineSubItem::~TimelineSubItem() = default;

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

    const auto name = Akonadi::CalendarUtils::displayName(mParent->calendar()->model(), mIncidence.parentCollection());
    setData(IncidenceFormatter::toolTipStr(name, Akonadi::CalendarUtils::incidence(mIncidence), originalStart().date(), true), Qt::ToolTipRole);
}

#include "moc_timelineitem.cpp"

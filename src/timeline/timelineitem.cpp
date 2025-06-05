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

void TimelineItem::insertIncidence(const Akonadi::Item &item, const QDateTime &start, const QDateTime &end)
{
    const Incidence::Ptr incidence = Akonadi::CalendarUtils::incidence(item);
    QDateTime dtStart(start);
    QDateTime dtEnd(end);
    if (!dtStart.isValid()) {
        dtStart = incidence->dtStart().toLocalTime();
    }
    if (!dtEnd.isValid()) {
        dtEnd = incidence->dateTime(Incidence::RoleEnd).toLocalTime();
    }
    if (incidence->allDay()) {
        dtEnd = dtEnd.addDays(1);
    }

    using ItemList = QList<QStandardItem *>;
    ItemList list = mItemMap.value(item.id());
    for (ItemList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        if (static_cast<TimelineSubItem *>(*it)->startTime() == dtStart && static_cast<TimelineSubItem *>(*it)->endTime() == dtEnd) {
            return;
        }
    }

    auto subItem = new TimelineSubItem(item, this);

    subItem->setStartTime(dtStart);
    subItem->setOriginalStart(dtStart);
    subItem->setEndTime(dtEnd);
    subItem->setData(mColor, Qt::DecorationRole);

    list = mModel->takeRow(mIndex);

    mItemMap[item.id()].append(subItem);

    list.append(mItemMap[item.id()]);

    mModel->insertRow(mIndex, list);
}

void TimelineItem::removeIncidence(const Akonadi::Item &item)
{
    qDeleteAll(mItemMap.value(item.id()));
    mItemMap.remove(item.id());
}

void TimelineItem::moveItems(const Akonadi::Item &item, int delta, int duration)
{
    using ItemList = QList<QStandardItem *>;
    const ItemList list = mItemMap.value(item.id());
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

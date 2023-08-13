/*
  SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KCalendarCore/Incidence>

#include <Akonadi/CollectionCalendar>
#include <Akonadi/Item>

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QStandardItemModel>

namespace EventViews
{
class TimelineSubItem;

class TimelineItem : public QObject
{
    Q_OBJECT
public:
    TimelineItem(const Akonadi::CollectionCalendar::Ptr &calendar, uint index, QStandardItemModel *model, QObject *parent);
    ~TimelineItem() override;

    void insertIncidence(const Akonadi::Item &incidence, const QDateTime &start = QDateTime(), const QDateTime &end = QDateTime());
    void removeIncidence(const Akonadi::Item &incidence);

    void moveItems(const Akonadi::Item &incidence, int delta, int duration);

    void setColor(const QColor &color);

    Q_REQUIRED_RESULT Akonadi::CollectionCalendar::Ptr calendar() const;

private:
    Akonadi::CollectionCalendar::Ptr mCalendar;
    QMap<Akonadi::Item::Id, QList<QStandardItem *>> mItemMap;
    QStandardItemModel *const mModel;
    QColor mColor;
    const uint mIndex;
};

class TimelineSubItem : public QStandardItem
{
public:
    TimelineSubItem(const Akonadi::Item &incidence, TimelineItem *parent);
    ~TimelineSubItem() override;

    Q_REQUIRED_RESULT Akonadi::Item incidence() const
    {
        return mIncidence;
    }

    Q_REQUIRED_RESULT QDateTime originalStart() const
    {
        return mStart;
    }

    void setOriginalStart(const QDateTime &dt)
    {
        mStart = dt;
    }

    void setStartTime(const QDateTime &dt);
    Q_REQUIRED_RESULT QDateTime startTime() const;

    void setEndTime(const QDateTime &dt);
    Q_REQUIRED_RESULT QDateTime endTime() const;

    Q_REQUIRED_RESULT TimelineItem *parent() const
    {
        return mParent;
    }

    void updateToolTip();

private:
    Akonadi::Item mIncidence;
    QDateTime mStart;
    TimelineItem *const mParent = nullptr;
    bool mToolTipNeedsUpdate;
};
}

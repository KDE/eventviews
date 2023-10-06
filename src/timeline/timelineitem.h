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

    [[nodiscard]] Akonadi::CollectionCalendar::Ptr calendar() const;

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

    [[nodiscard]] Akonadi::Item incidence() const
    {
        return mIncidence;
    }

    [[nodiscard]] QDateTime originalStart() const
    {
        return mStart;
    }

    void setOriginalStart(const QDateTime &dt)
    {
        mStart = dt;
    }

    void setStartTime(const QDateTime &dt);
    [[nodiscard]] QDateTime startTime() const;

    void setEndTime(const QDateTime &dt);
    [[nodiscard]] QDateTime endTime() const;

    [[nodiscard]] TimelineItem *parent() const
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

/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "timelineview.h"

#include <AkonadiCore/Item>
#include <Collection>

#include <QMap>
#include <QModelIndex>
#include <QObject>

class QStandardItem;
class QTreeWidget;

namespace KGantt
{
class GraphicsView;
}

namespace EventViews
{
class TimelineItem;
class RowController;

class TimelineView::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(TimelineView *parent = nullptr);
    ~Private();

    TimelineItem *calendarItemForIncidence(const Akonadi::Item &incidence);
    void insertIncidence(const Akonadi::Item &incidence);
    void insertIncidence(const Akonadi::Item &incidence, const QDate &day);
    void removeIncidence(const Akonadi::Item &incidence);

public Q_SLOTS:
    // void overscale( KDGantt::View::Scale scale );
    void itemSelected(const QModelIndex &index);
    void itemDoubleClicked(const QModelIndex &index);
    void itemChanged(QStandardItem *item);
    void contextMenuRequested(const QPoint &point);
    void newEventWithHint(const QDateTime &);

public:
    Akonadi::Item::List mSelectedItemList;
    KGantt::GraphicsView *mGantt = nullptr;
    QTreeWidget *mLeftView = nullptr;
    RowController *mRowController = nullptr;
    QMap<Akonadi::Collection::Id, TimelineItem *> mCalendarItemMap;
    QDate mStartDate, mEndDate;
    QDateTime mHintDate;

private:
    TimelineView *const q;
};
} // namespace EventViews


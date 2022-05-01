/*
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>
  SPDX-FileCopyrightText: 2012 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "todomodel.h"

#include <Akonadi/ETMCalendar>
#include <Akonadi/Item>

#include <QModelIndex>
#include <QString>

namespace Akonadi
{
class IncidenceChanger;
}

class TodoModelPrivate : public QObject
{
    Q_OBJECT
public:
    TodoModelPrivate(const EventViews::PrefsPtr &preferences, TodoModel *qq);

    // TODO: O(N) complexity, see if the profiler complains about this
    Akonadi::Item findItemByUid(const QString &uid, const QModelIndex &parent) const;

public:
    Akonadi::ETMCalendar::Ptr m_calendar;
    Akonadi::IncidenceChanger *m_changer = nullptr;
    EventViews::PrefsPtr m_preferences;

public Q_SLOTS:
    void onDataChanged(const QModelIndex &begin, const QModelIndex &end);

private:
    TodoModel *const q;
};

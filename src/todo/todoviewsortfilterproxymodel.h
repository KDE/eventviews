/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "prefs.h"

#include <QSortFilterProxyModel>
#include <QStringList>

class TodoViewSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TodoViewSortFilterProxyModel(const EventViews::PrefsPtr &prefs, QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    Q_REQUIRED_RESULT const QStringList &categories() const
    {
        return mCategories;
    }

    Q_REQUIRED_RESULT const QStringList &priorities() const
    {
        return mPriorities;
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

public Q_SLOTS:
    void setCategoryFilter(const QStringList &categories);
    void setPriorityFilter(const QStringList &priorities);

private:
    int compareStartDates(const QModelIndex &left, const QModelIndex &right) const;
    int compareDueDates(const QModelIndex &left, const QModelIndex &right) const;
    int comparePriorities(const QModelIndex &left, const QModelIndex &right) const;
    int compareCompletion(const QModelIndex &left, const QModelIndex &right) const;
    QStringList mCategories;
    QStringList mPriorities;
    Qt::SortOrder mSortOrder = Qt::AscendingOrder;
    EventViews::PrefsPtr mPreferences;
};


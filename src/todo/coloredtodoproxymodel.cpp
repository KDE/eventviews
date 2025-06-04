/*
  SPDX-FileCopyrightText: 2023 Kevin Ottens <ervin@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "coloredtodoproxymodel.h"

#include <Akonadi/TodoModel>

#include <KCalUtils/IncidenceFormatter>

class ColoredTodoProxyModelPrivate
{
public:
    explicit ColoredTodoProxyModelPrivate(const EventViews::PrefsPtr &preferences)
        : m_preferences(preferences)
    {
    }

    EventViews::PrefsPtr m_preferences;
};

static bool isDueToday(const KCalendarCore::Todo::Ptr &todo)
{
    return !todo->isCompleted() && todo->dtDue().date() == QDate::currentDate();
}

ColoredTodoProxyModel::ColoredTodoProxyModel(const EventViews::PrefsPtr &preferences, QObject *parent)
    : QIdentityProxyModel(parent)
    , d(new ColoredTodoProxyModelPrivate(preferences))
{
}

ColoredTodoProxyModel::~ColoredTodoProxyModel() = default;

QVariant ColoredTodoProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::BackgroundRole) {
        const auto todo = QIdentityProxyModel::data(index, Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
        if (!todo) {
            return {};
        }

        if (todo->isOverdue()) {
            return QVariant(QBrush(d->m_preferences->todoOverdueColor()));
        } else if (isDueToday(todo)) {
            return QVariant(QBrush(d->m_preferences->todoDueTodayColor()));
        } else {
            return {};
        }
    }

    if (role == Qt::ToolTipRole) {
        const auto todo = QIdentityProxyModel::data(index, Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
        if (!todo) {
            return {};
        }
        QString displayName;
        const Akonadi::Item item = data(index, Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        if (item.isValid()) {
            const Akonadi::Collection col = Akonadi::EntityTreeModel::updatedCollection(this, item.storageCollectionId());
            if (col.isValid()) {
                displayName = col.displayName();
            }
        }
        return KCalUtils::IncidenceFormatter::toolTipStr(displayName, todo, QDate::currentDate(), true);
    }

    return QIdentityProxyModel::data(index, role);
}

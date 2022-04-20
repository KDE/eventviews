/*
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>
  SPDX-FileCopyrightText: 2012 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "eventviews_export.h"
#include "prefs.h"

#include <Akonadi/ETMCalendar>
#include <Akonadi/IncidenceChanger>
#include <Akonadi/Item>

#include <Akonadi/EntityTreeModel>
#include <KCalendarCore/Todo>

#include <KExtraColumnsProxyModel>

#include <memory>

class QMimeData;

class TodoModelPrivate;

/** Expands an IncidenceTreeModel by additional columns for showing todos. */
class EVENTVIEWS_EXPORT TodoModel : public KExtraColumnsProxyModel
{
    Q_OBJECT

public:
    /** This enum defines all columns this model provides */
    enum {
        SummaryColumn = 0,
        RecurColumn,
        PriorityColumn,
        PercentColumn,
        StartDateColumn,
        DueDateColumn,
        CategoriesColumn,
        DescriptionColumn,
        CalendarColumn,
        CompletedDateColumn,
        ColumnCount // Just for iteration/column count purposes. Always keep at the end of enum.
    };

    /** This enum defines the user defined roles of the items in this model */
    enum {
        TodoRole = Akonadi::EntityTreeModel::UserRole + 1,
        TodoPtrRole,
        IsRichTextRole,
        SummaryRole,
        RecurRole,
        PriorityRole,
        PercentRole,
        StartDateRole,
        DueDateRole,
        CategoriesRole,
        DescriptionRole,
        CalendarRole,
    };

    explicit TodoModel(const EventViews::PrefsPtr &preferences, QObject *parent = nullptr);

    ~TodoModel() override;

    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override;
    QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role = Qt::DisplayRole) const override;

    Q_REQUIRED_RESULT bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation, int role) const override;

    void setCalendar(const Akonadi::ETMCalendar::Ptr &calendar);

    void setIncidenceChanger(Akonadi::IncidenceChanger *changer);

    Q_REQUIRED_RESULT QMimeData *mimeData(const QModelIndexList &indexes) const override;

    Q_REQUIRED_RESULT bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    Q_REQUIRED_RESULT QStringList mimeTypes() const override;

    Q_REQUIRED_RESULT Qt::DropActions supportedDropActions() const override;

    Q_REQUIRED_RESULT Qt::ItemFlags flags(const QModelIndex &index) const override;

    Q_REQUIRED_RESULT QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    /** Emitted when dropMimeData() rejected a drop
     *  on the same item or any of its children.
     */
    void dropOnSelfRejected();

private:
    friend class TodoModelPrivate;
    std::unique_ptr<TodoModelPrivate> const d;
};

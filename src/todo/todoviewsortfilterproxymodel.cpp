/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "todoviewsortfilterproxymodel.h"

#include <Akonadi/TodoModel>
#include <CalendarSupport/Utils>
#include <KCalendarCore/CalFilter>

#include <KLocalizedString>

TodoViewSortFilterProxyModel::TodoViewSortFilterProxyModel(const EventViews::PrefsPtr &prefs, QObject *parent)
    : QSortFilterProxyModel(parent)
    , mPreferences(prefs)
{
}

void TodoViewSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    mSortOrder = order;
    QSortFilterProxyModel::sort(column, order);
}

bool TodoViewSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    bool ret = QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

    if (ret && mCalFilter) {
        const auto incidence = sourceModel()
                                   ->index(source_row, Akonadi::TodoModel::SummaryColumn, source_parent)
                                   .data(Akonadi::TodoModel::TodoPtrRole)
                                   .value<KCalendarCore::Todo::Ptr>();
        if (!mCalFilter->filterIncidence(incidence)) {
            return false;
        }
    }

    bool returnValue = true;
    if (ret && !mPriorities.isEmpty()) {
        QString priorityValue = sourceModel()->index(source_row, Akonadi::TodoModel::PriorityColumn, source_parent).data(Qt::EditRole).toString();
        returnValue = mPriorities.contains(priorityValue);
    }
    if (ret && !mCategories.isEmpty()) {
        const QStringList categories = sourceModel()->index(source_row, Akonadi::TodoModel::CategoriesColumn, source_parent).data(Qt::EditRole).toStringList();

        for (const QString &category : categories) {
            if (mCategories.contains(category)) {
                return returnValue && true;
            }
        }
        ret = false;
    }

    // check if one of the children is accepted, and accept this node too if so
    QModelIndex cur = sourceModel()->index(source_row, Akonadi::TodoModel::SummaryColumn, source_parent);
    if (cur.isValid()) {
        for (int r = 0; r < cur.model()->rowCount(cur); ++r) {
            if (filterAcceptsRow(r, cur)) {
                return true;
            }
        }
    }

    return ret && returnValue;
}

bool TodoViewSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (mPreferences->sortCompletedTodosSeparately() && left.column() != Akonadi::TodoModel::PercentColumn) {
        QModelIndex cLeft = left.sibling(left.row(), Akonadi::TodoModel::PercentColumn);
        QModelIndex cRight = right.sibling(right.row(), Akonadi::TodoModel::PercentColumn);

        if (cRight.data(Qt::EditRole).toInt() == 100 && cLeft.data(Qt::EditRole).toInt() != 100) {
            return mSortOrder == Qt::AscendingOrder ? true : false;
        } else if (cRight.data(Qt::EditRole).toInt() != 100 && cLeft.data(Qt::EditRole).toInt() == 100) {
            return mSortOrder == Qt::AscendingOrder ? false : true;
        }
    }

    // To-dos without due date should appear last when sorting ascending,
    // so you can see the most urgent tasks first. (bug #174763)
    if (right.column() == Akonadi::TodoModel::DueDateColumn) {
        const int comparison = compareDueDates(left, right);

        if (comparison != 0) {
            return comparison == -1;
        } else {
            // Due dates are equal, but the user still expects sorting by importance
            // Fallback to the PriorityColumn
            QModelIndex leftPriorityIndex = left.sibling(left.row(), Akonadi::TodoModel::PriorityColumn);
            QModelIndex rightPriorityIndex = right.sibling(right.row(), Akonadi::TodoModel::PriorityColumn);
            const int fallbackComparison = comparePriorities(leftPriorityIndex, rightPriorityIndex);

            if (fallbackComparison != 0) {
                return fallbackComparison == 1;
            }
        }
    } else if (right.column() == Akonadi::TodoModel::StartDateColumn) {
        return compareStartDates(left, right) == -1;
    } else if (right.column() == Akonadi::TodoModel::CompletedDateColumn) {
        return compareCompletedDates(left, right) == -1;
    } else if (right.column() == Akonadi::TodoModel::PriorityColumn) {
        const int comparison = comparePriorities(left, right);

        if (comparison != 0) {
            return comparison == -1;
        } else {
            // Priorities are equal, but the user still expects sorting by importance
            // Fallback to the DueDateColumn
            QModelIndex leftDueDateIndex = left.sibling(left.row(), Akonadi::TodoModel::DueDateColumn);
            QModelIndex rightDueDateIndex = right.sibling(right.row(), Akonadi::TodoModel::DueDateColumn);
            const int fallbackComparison = compareDueDates(leftDueDateIndex, rightDueDateIndex);

            if (fallbackComparison != 0) {
                return fallbackComparison == 1;
            }
        }
    } else if (right.column() == Akonadi::TodoModel::PercentColumn) {
        const int comparison = compareCompletion(left, right);
        if (comparison != 0) {
            return comparison == -1;
        }
    }

    if (left.data() == right.data()) {
        // If both are equal, lets choose an order, otherwise Qt will display them randomly.
        // Fixes to-dos jumping around when you have calendar A selected, and then check/uncheck
        // a calendar B with no to-dos. No to-do is added/removed because calendar B is empty,
        // but you see the existing to-dos switching places.
        QModelIndex leftSummaryIndex = left.sibling(left.row(), Akonadi::TodoModel::SummaryColumn);
        QModelIndex rightSummaryIndex = right.sibling(right.row(), Akonadi::TodoModel::SummaryColumn);

        // This patch is not about fallingback to the SummaryColumn for sorting.
        // It's about avoiding jumping due to random reasons.
        // That's why we ignore the sort direction...
        return mSortOrder == Qt::AscendingOrder ? QSortFilterProxyModel::lessThan(leftSummaryIndex, rightSummaryIndex)
                                                : QSortFilterProxyModel::lessThan(rightSummaryIndex, leftSummaryIndex);

        // ...so, if you have 4 to-dos, all with CompletionColumn = "55%",
        // and click the header multiple times, nothing will happen because
        // it is already sorted by Completion.
    } else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

void TodoViewSortFilterProxyModel::setPriorityFilter(const QStringList &priorities)
{
    // preparing priority list for comparison
    mPriorities.clear();
    for (const QString &eachPriority : priorities) {
        if (eachPriority == i18nc("priority is unspecified", "unspecified")) {
            mPriorities.append(i18n("%1", 0));
        } else if (eachPriority == i18nc("highest priority", "%1 (highest)", 1)) {
            mPriorities.append(i18n("%1", 1));
        } else if (eachPriority == i18nc("medium priority", "%1 (medium)", 5)) {
            mPriorities.append(i18n("%1", 5));
        } else if (eachPriority == i18nc("lowest priority", "%1 (lowest)", 9)) {
            mPriorities.append(i18n("%1", 9));
        } else {
            mPriorities.append(eachPriority);
        }
    }
    invalidateFilter();
}

int TodoViewSortFilterProxyModel::compareStartDates(const QModelIndex &left, const QModelIndex &right) const
{
    Q_ASSERT(left.column() == Akonadi::TodoModel::StartDateColumn);
    Q_ASSERT(right.column() == Akonadi::TodoModel::StartDateColumn);

    // The start date column is a QString, so fetch the to-do.
    // We can't compare QStrings because it won't work if the format is MM/DD/YYYY
    const auto leftTodo = left.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
    const auto rightTodo = right.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();

    if (!leftTodo || !rightTodo) {
        return 0;
    }

    const bool leftIsEmpty = !leftTodo->hasStartDate();
    const bool rightIsEmpty = !rightTodo->hasStartDate();

    if (leftIsEmpty != rightIsEmpty) { // One of them doesn't have a start date
        // For sorting, no date is considered a very big date
        return rightIsEmpty ? -1 : 1;
    } else if (!leftIsEmpty) { // Both have start dates
        const auto leftDateTime = leftTodo->dtStart();
        const auto rightDateTime = rightTodo->dtStart();

        if (leftDateTime == rightDateTime) {
            return 0;
        } else {
            return leftDateTime < rightDateTime ? -1 : 1;
        }
    } else { // Neither has a start date
        return 0;
    }
}

int TodoViewSortFilterProxyModel::compareCompletedDates(const QModelIndex &left, const QModelIndex &right) const
{
    Q_ASSERT(left.column() == Akonadi::TodoModel::CompletedDateColumn);
    Q_ASSERT(right.column() == Akonadi::TodoModel::CompletedDateColumn);

    const auto leftTodo = left.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
    const auto rightTodo = right.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();

    if (!leftTodo || !rightTodo) {
        return 0;
    }

    const bool leftIsEmpty = !leftTodo->hasCompletedDate();
    const bool rightIsEmpty = !rightTodo->hasCompletedDate();

    if (leftIsEmpty != rightIsEmpty) { // One of them doesn't have a completed date.
        // For sorting, no date is considered a very big date.
        return rightIsEmpty ? -1 : 1;
    } else if (!leftIsEmpty) { // Both have completed dates.
        const auto leftDateTime = leftTodo->completed();
        const auto rightDateTime = rightTodo->completed();

        if (leftDateTime == rightDateTime) {
            return 0;
        } else {
            return leftDateTime < rightDateTime ? -1 : 1;
        }
    } else { // Neither has a completed date.
        return 0;
    }
}

void TodoViewSortFilterProxyModel::setCategoryFilter(const QStringList &categories)
{
    if (mCategories != categories) {
        mCategories = categories;
        invalidateFilter();
    }
}

void TodoViewSortFilterProxyModel::setCalFilter(KCalendarCore::CalFilter *filter)
{
    if (mCalFilter != filter) {
        mCalFilter = filter;
        invalidateFilter();
    }
}

/* -1 - less than
 *  0 - equal
 *  1 - bigger than
 */
int TodoViewSortFilterProxyModel::compareDueDates(const QModelIndex &left, const QModelIndex &right) const
{
    Q_ASSERT(left.column() == Akonadi::TodoModel::DueDateColumn);
    Q_ASSERT(right.column() == Akonadi::TodoModel::DueDateColumn);

    // The due date column is a QString, so fetch the to-do.
    // We can't compare QStrings because it won't work if the format is MM/DD/YYYY
    const auto leftTodo = left.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
    const auto rightTodo = right.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
    Q_ASSERT(leftTodo);
    Q_ASSERT(rightTodo);

    if (!leftTodo || !rightTodo) {
        return 0;
    }

    const bool leftIsEmpty = !leftTodo->hasDueDate();
    const bool rightIsEmpty = !rightTodo->hasDueDate();

    if (leftIsEmpty != rightIsEmpty) { // One of them doesn't have a due date
        // For sorting, no date is considered a very big date
        return rightIsEmpty ? -1 : 1;
    } else if (!leftIsEmpty) { // Both have due dates
        const auto leftDateTime = leftTodo->dtDue();
        const auto rightDateTime = rightTodo->dtDue();

        if (leftDateTime == rightDateTime) {
            return 0;
        } else {
            return leftDateTime < rightDateTime ? -1 : 1;
        }
    } else { // Neither has a due date
        return 0;
    }
}

/* -1 - less than
 *  0 - equal
 *  1 - bigger than
 */
int TodoViewSortFilterProxyModel::compareCompletion(const QModelIndex &left, const QModelIndex &right) const
{
    Q_ASSERT(left.column() == Akonadi::TodoModel::PercentColumn);
    Q_ASSERT(right.column() == Akonadi::TodoModel::PercentColumn);

    const int leftValue = sourceModel()->data(left).toInt();
    const int rightValue = sourceModel()->data(right).toInt();

    if (leftValue == 100 && rightValue == 100) {
        // Break ties with the completion date.
        const auto leftTodo = left.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
        const auto rightTodo = right.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
        Q_ASSERT(leftTodo);
        Q_ASSERT(rightTodo);
        if (!leftTodo || !rightTodo) {
            return 0;
        } else {
            return (leftTodo->completed() > rightTodo->completed()) ? -1 : 1;
        }
    } else {
        return (leftValue < rightValue) ? -1 : 1;
    }
}

/* -1 - less than
 *  0 - equal
 *  1 - bigger than
 * Sort in numeric order (1 < 9) rather than priority order (lowest 9 < highest 1).
 * There are arguments either way, but this is consistent with KCalendarCore.
 */
int TodoViewSortFilterProxyModel::comparePriorities(const QModelIndex &left, const QModelIndex &right) const
{
    Q_ASSERT(left.isValid());
    Q_ASSERT(right.isValid());

    const auto leftTodo = left.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
    const auto rightTodo = right.data(Akonadi::TodoModel::TodoPtrRole).value<KCalendarCore::Todo::Ptr>();
    Q_ASSERT(leftTodo);
    Q_ASSERT(rightTodo);
    if (!leftTodo || !rightTodo || leftTodo->priority() == rightTodo->priority()) {
        return 0;
    } else if (leftTodo->priority() < rightTodo->priority()) {
        return -1;
    } else {
        return 1;
    }
}

#include "moc_todoviewsortfilterproxymodel.cpp"

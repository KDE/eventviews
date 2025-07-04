/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
// clazy:excludeall=overloaded-signal

#pragma once

#include "eventview.h"
#include <Akonadi/IncidenceChanger>

#include <QPointer>
#include <akonadi/collectioncalendar.h>

class KJob;
class TodoCategoriesDelegate;
class TodoViewQuickAddLine;
class TodoViewQuickSearch;
class TodoViewSortFilterProxyModel;
class TodoViewView;

namespace Akonadi
{
class ETMViewStateSaver;
}

class QItemSelection;
class QMenu;
class QModelIndex;
class QToolButton;
class QTimer;
class KConfig;
class KDatePickerPopup;
namespace EventViews
{

class CalendarFilterModel;

/**
 * This class provides a view for Todo items.

 * @short View for Todo components.
 * @author Cornelius Schumacher <schumacher@kde.org>, Reinhold Kainhofer <reinhold@kainhofer.com>
 * @see EventView
 */
class EVENTVIEWS_EXPORT TodoView : public EventViews::EventView
{
    Q_OBJECT
    friend class ModelStack;

public:
    TodoView(const EventViews::PrefsPtr &prefs, bool sidebarView, QWidget *parent);
    ~TodoView() override;

    void setModel(QAbstractItemModel *model) override;
    void addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;
    void removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;

    [[nodiscard]] Akonadi::Item::List selectedIncidences() const override;
    [[nodiscard]] KCalendarCore::DateList selectedIncidenceDates() const override;
    [[nodiscard]] int currentDateCount() const override
    {
        return 0;
    }

    void setDocumentId(const QString &)
    {
    }

    void saveLayout(KConfig *config, const QString &group) const;

    void restoreLayout(KConfig *config, const QString &group, bool minimalDefaults);

    /** documentation in baseview.h */
    void getHighlightMode(bool &highlightEvents, bool &highlightTodos, bool &highlightJournals);

    [[nodiscard]] bool usesFullWindow();

    [[nodiscard]] bool supportsDateRangeSelection() const
    {
        return false;
    }

public Q_SLOTS:
    void setIncidenceChanger(Akonadi::IncidenceChanger *changer) override;
    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth = QDate()) override;
    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;
    void updateView() override;
    virtual void changeIncidenceDisplay(const Akonadi::Item &incidence, Akonadi::IncidenceChanger::ChangeType changeType);
    void updateConfig() override;
    void clearSelection() override;
    void expandIndex(const QModelIndex &index);
    void restoreViewState();
    void saveViewState();

    void createEvent();

protected Q_SLOTS:
    void resizeEvent(QResizeEvent *) override;
    void addQuickTodo(Qt::KeyboardModifiers modifier);

    void contextMenu(QPoint pos);

    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

    // slots used by popup-menus
    void showTodo();
    void editTodo();
    void deleteTodo();
    void newTodo();
    void newSubTodo();
    void copyTodoToDate(QDate date);
    void toggleTodoCompleted();
    void toggleTodoAlarm();

private Q_SLOTS:
    EVENTVIEWS_NO_EXPORT void scheduleResizeColumns();
    EVENTVIEWS_NO_EXPORT void resizeColumns();
    EVENTVIEWS_NO_EXPORT void itemDoubleClicked(const QModelIndex &index);
    EVENTVIEWS_NO_EXPORT void setNewDate(QDate date);
    EVENTVIEWS_NO_EXPORT void setStartDate(QDate date);
    EVENTVIEWS_NO_EXPORT void setNewPercentage(QAction *action);
    EVENTVIEWS_NO_EXPORT void setNewPriority(QAction *action);
    EVENTVIEWS_NO_EXPORT void changedCategories(QAction *action);
    EVENTVIEWS_NO_EXPORT void setFullView(bool fullView);

    EVENTVIEWS_NO_EXPORT void setFlatView(bool flatView, bool notifyOtherViews = true);

    EVENTVIEWS_NO_EXPORT void onRowsInserted(const QModelIndex &parent, int start, int end);
    EVENTVIEWS_NO_EXPORT void onTagsFetched(KJob *);

Q_SIGNALS:
    void purgeCompletedSignal();
    void unSubTodoSignal();
    void unAllSubTodoSignal();
    void configChanged();
    void fullViewChanged(bool enabled);
    void printPreviewTodo();
    void printTodo();

    void createEvent(const Akonadi::Item &item); // clazy:exclude=overloaded-signal

private:
    EVENTVIEWS_NO_EXPORT QMenu *createCategoryPopupMenu();
    EVENTVIEWS_NO_EXPORT QString stateSaverGroup() const;

    /** Creates a new todo with the given text as summary under the given parent */
    void addTodo(const QString &summary, const Akonadi::Item &parentItem, const QStringList &categories = QStringList());

    TodoViewView *mView = nullptr;
    TodoViewSortFilterProxyModel *mProxyModel = nullptr;
    TodoCategoriesDelegate *mCategoriesDelegate = nullptr;
    std::unique_ptr<CalendarFilterModel> mCalendarFilterModel;

    TodoViewQuickSearch *mQuickSearch = nullptr;
    TodoViewQuickAddLine *mQuickAdd = nullptr;
    QToolButton *mFullViewButton = nullptr;
    QToolButton *mFlatViewButton = nullptr;

    QMenu *mItemPopupMenu = nullptr;
    KDatePickerPopup *mCopyPopupMenu = nullptr;
    KDatePickerPopup *mMovePopupMenu = nullptr;
    QMenu *mPriorityPopupMenu = nullptr;
    QMenu *mPercentageCompletedPopupMenu = nullptr;
    QList<QAction *> mItemPopupMenuItemOnlyEntries;
    QList<QAction *> mItemPopupMenuReadWriteEntries;

    QAction *mMakeTodoIndependent = nullptr;
    QAction *mMakeSubtodosIndependent = nullptr;

    QPointer<Akonadi::ETMViewStateSaver> mTreeStateRestorer;

    QMap<QAction *, int> mPercentage;
    QMap<QAction *, int> mPriority;
    bool mSidebarView;
    bool mResizeColumnsScheduled;
    QTimer *mResizeColumnsTimer = nullptr;
};
}

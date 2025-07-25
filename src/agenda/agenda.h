/*
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

  Marcus Bains line.
  SPDX-FileCopyrightText: 2001 Ali Rahimi <ali@mit.edu>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "agendaitem.h"
#include "eventviews_export.h"
#include "viewcalendar.h"

#include <Akonadi/Item>

#include <KCalendarCore/Todo>

#include <QFrame>
#include <QScrollArea>

#include <memory>

namespace Akonadi
{
class IncidenceChanger;
}

namespace EventViews
{
class Agenda;
class AgendaItem;
class AgendaView;

class MarcusBainsPrivate;

class MarcusBains : public QFrame
{
    Q_OBJECT
public:
    explicit MarcusBains(EventView *eventView, Agenda *agenda = nullptr);
    void updateLocationRecalc(bool recalculate = false);
    ~MarcusBains() override;

public Q_SLOTS:
    void updateLocation();

private:
    std::unique_ptr<MarcusBainsPrivate> const d;
};

class AgendaPrivate;

class EVENTVIEWS_EXPORT Agenda : public QWidget
{
    Q_OBJECT
public:
    Agenda(AgendaView *agendaView, QScrollArea *scrollArea, int columns, int rows, int rowSize, bool isInteractive);

    Agenda(AgendaView *agendaView, QScrollArea *scrollArea, int columns, bool isInteractive);

    ~Agenda() override;

    [[nodiscard]] KCalendarCore::Incidence::Ptr selectedIncidence() const;
    [[nodiscard]] QDate selectedIncidenceDate() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QSize minimumSize() const;
    int minimumHeight() const;
    // QSizePolicy sizePolicy() const;
    [[nodiscard]] int contentsY() const
    {
        return -y();
    }

    [[nodiscard]] int contentsX() const
    {
        return x();
    }

    [[nodiscard]] QScrollBar *verticalScrollBar() const;

    [[nodiscard]] QScrollArea *scrollArea() const;

    [[nodiscard]] AgendaItem::List agendaItems(const QString &uid) const;

    /**
      Returns the uid of the last incidence that was selected. This
      persists across reloads and clear, so that if the same uid
      reappears, it can be reselected.
    */
    [[nodiscard]] QString lastSelectedItemUid() const;

    bool eventFilter(QObject *, QEvent *) override;

    void paintEvent(QPaintEvent *) override;

    [[nodiscard]] QPoint contentsToGrid(QPoint pos) const;
    [[nodiscard]] QPoint gridToContents(QPoint gpos) const;

    [[nodiscard]] int timeToY(QTime time) const;
    [[nodiscard]] QTime gyToTime(int y) const;

    [[nodiscard]] QList<int> minContentsY() const;
    [[nodiscard]] QList<int> maxContentsY() const;

    [[nodiscard]] int visibleContentsYMin() const;
    [[nodiscard]] int visibleContentsYMax() const;

    void setStartTime(QTime startHour);

    AgendaItem::QPtr insertItem(const KCalendarCore::Incidence::Ptr &incidence,
                                const QDateTime &recurrenceId,
                                int X,
                                int YTop,
                                int YBottom,
                                int itemPos,
                                int itemCount,
                                bool isSelected);

    AgendaItem::QPtr insertAllDayItem(const KCalendarCore::Incidence::Ptr &incidence, const QDateTime &recurrenceId, int XBegin, int XEnd, bool isSelected);

    void
    insertMultiItem(const KCalendarCore::Incidence::Ptr &event, const QDateTime &recurrenceId, int XBegin, int XEnd, int YTop, int YBottom, bool isSelected);

    /**
      Removes an event and all its multi-items from the agenda. This function
      removes the items from the view, but doesn't delete them immediately.
      Instead, they are queued in mItemsToDelete and later deleted by the
      slot deleteItemsToDelete() (called by QTimer::singleShot ).
      @param incidence The pointer to the incidence that should be removed.
    */
    void removeIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    void changeColumns(int columns);

    [[nodiscard]] int columns() const;
    [[nodiscard]] int rows() const;

    [[nodiscard]] double gridSpacingX() const;
    [[nodiscard]] double gridSpacingY() const;

    void clear();

    /** Update configuration from preference settings */
    void updateConfig();

    void checkScrollBoundaries();

    void setHolidayMask(QList<bool> *);

    void setDateList(const KCalendarCore::DateList &selectedDates);
    [[nodiscard]] KCalendarCore::DateList dateList() const;

    void setCalendar(const EventViews::MultiViewCalendar::Ptr &cal);

    void setIncidenceChanger(Akonadi::IncidenceChanger *changer);

public Q_SLOTS:
    void scrollUp();
    void scrollDown();

    void checkScrollBoundaries(int);

    /** Deselect selected items. This function does not Q_EMIT any signals. */
    void deselectItem();

    void clearSelection();

    /**
      Select item. If the argument is 0, the currently selected item gets
      deselected. This function emits the itemSelected(bool) signal to inform
      about selection/deselection of events.
    */
    void selectItem(const EventViews::AgendaItem::QPtr &);

    /**
      Selects the item associated with a given Akonadi Item id.
      Linear search, use carefully.
      @param id the item id of the item that should be selected. If no such
      item exists, the selection is not changed.
    */
    void selectIncidenceByUid(const QString &id);
    void selectItem(const Akonadi::Item &item);

    bool removeAgendaItem(const EventViews::AgendaItem::QPtr &item);
    void showAgendaItem(const EventViews::AgendaItem::QPtr &item);

Q_SIGNALS:
    void newEventSignal();
    void newTimeSpanSignal(const QPoint &, const QPoint &);
    void newStartSelectSignal();

    void showIncidenceSignal(const KCalendarCore::Incidence::Ptr &);
    void editIncidenceSignal(const KCalendarCore::Incidence::Ptr &);
    void deleteIncidenceSignal(const KCalendarCore::Incidence::Ptr &);
    void showIncidencePopupSignal(const KCalendarCore::Incidence::Ptr &, const QDate &);

    void showNewEventPopupSignal();

    void incidenceSelected(const KCalendarCore::Incidence::Ptr &, const QDate &);

    void lowerYChanged(int);
    void upperYChanged(int);

    void startDragSignal(const KCalendarCore::Incidence::Ptr &);
    void droppedIncidencesSignal(const KCalendarCore::Incidence::List &, const QPoint &gpos, bool allDay);
    void droppedUrlsSignal(const QList<QUrl> &, const QPoint &gpos, bool allDay);

    void enableAgendaUpdate(bool enable);
    void zoomView(const int delta, const QPoint &pos, const Qt::Orientation);

    void mousePosSignal(const QPoint &pos);
    void enterAgenda();
    void leaveAgenda();

    void gridSpacingYChanged(double);

private:
    enum MouseActionType {
        NOP,
        MOVE,
        SELECT,
        RESIZETOP,
        RESIZEBOTTOM,
        RESIZELEFT,
        RESIZERIGHT
    };

    AgendaItem::QPtr
    createAgendaItem(const KCalendarCore::Incidence::Ptr &incidence, int itemPos, int itemCount, const QDateTime &recurrenceId, bool isSelected);

protected:
    /**
      Draw the background grid of the agenda.
      @p cw grid width
      @p ch grid height
    */
    void drawContents(QPainter *p, int cx, int cy, int cw, int ch);

    int columnWidth(int column) const;
    void resizeEvent(QResizeEvent *) override;

    /** Handles mouse events. Called from eventFilter */
    virtual bool eventFilter_mouse(QObject *, QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    /** Handles mousewheel events. Called from eventFilter */
    virtual bool eventFilter_wheel(QObject *, QWheelEvent *);
#endif
    /** Handles key events. Called from eventFilter */
    virtual bool eventFilter_key(QObject *, QKeyEvent *);

    /** Handles drag and drop events. Called from eventFilter */
    virtual bool eventFilter_drag(QObject *, QDropEvent *);

    /** returns RESIZELEFT if pos is near the lower edge of the action item,
      RESIZERIGHT if pos is near the higher edge, and MOVE otherwise.
      If --reverse is used, RESIZELEFT still means resizing the beginning of
      the event, although that means moving to the right!
      horizontal is the same as mAllDayAgenda.
        @param horizontal Whether horizontal resizing is  possible
        @param pos The current mouse position
        @param item The affected item
    */
    MouseActionType isInResizeArea(bool horizontal, QPoint pos, const AgendaItem::QPtr &item);
    /** Return whether the cell specified by the grid point belongs to the current select
     */
    [[nodiscard]] bool ptInSelection(QPoint gpos) const;

    /** Start selecting time span. */
    void startSelectAction(QPoint viewportPos);

    /** Select time span. */
    void performSelectAction(QPoint viewportPos);

    /** Emd selecting time span. */
    void endSelectAction(const QPoint &viewportPos);

    /** Start moving/resizing agenda item */
    void startItemAction(const QPoint &viewportPos);

    /** Move/resize agenda item */
    void performItemAction(QPoint viewportPos);

    /** End moving/resizing agenda item */
    void endItemAction();

    /** Set cursor, when no item action is in progress */
    void setNoActionCursor(const AgendaItem::QPtr &moveItem, QPoint viewportPos);
    /** Sets the cursor according to the given action type.
        @param actionType The type of action for which the cursor should be set.
        @param acting If true, the corresponding action is running (e.g. the
        item is currently being moved by the user). If false the
        cursor should just indicate that the corresponding
        action is possible */
    void setActionCursor(int actionType, bool acting = false);

    /** calculate the width of the column subcells of the given item */
    double calcSubCellWidth(const AgendaItem::QPtr &item);
    /** Move and resize the given item to the correct position */
    void placeAgendaItem(const AgendaItem::QPtr &item, double subCellWidth);
    /** Place agenda item in agenda and adjust other cells if necessary */
    void placeSubCells(const AgendaItem::QPtr &placeItem);
    /** Place the agenda item at the correct position (ignoring conflicting items) */
    void adjustItemPosition(const AgendaItem::QPtr &item);

    /** Process the keyevent, including the ignored keyevents of eventwidgets.
     * Implements pgup/pgdn and cursor key navigation in the view.
     */
    void keyPressEvent(QKeyEvent *) override;

    void calculateWorkingHours();

    virtual void contentsMousePressEvent(QMouseEvent *);

protected Q_SLOTS:
    /** delete the items that are queued for deletion */
    void deleteItemsToDelete();
    /** Resizes all the child elements after the size of the agenda
        changed. This is needed because Qt seems to have a bug when
        the resizeEvent of one of the widgets in a splitter takes a
        lot of time / does a lot of resizes.... see bug 80114 */
    void resizeAllContents();

private:
    EVENTVIEWS_NO_EXPORT void init();
    EVENTVIEWS_NO_EXPORT void marcus_bains();

private:
    friend class AgendaPrivate;
    std::unique_ptr<AgendaPrivate> const d;
};

class AgendaScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    AgendaScrollArea(bool allDay, AgendaView *agendaView, bool isInteractive, QWidget *parent);
    ~AgendaScrollArea() override;

    Agenda *agenda() const;

private:
    Agenda *mAgenda = nullptr;
};
}

/*
  Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Author: Kevin Krammer, krake@kdab.com

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef EVENTVIEWS_AGENDA_H
#define EVENTVIEWS_AGENDA_H

#include "eventviews_export.h"
#include "agendaitem.h"
#include "viewcalendar.h"

#include <Item>

#include <KCalendarCore/Todo>

#include <QFrame>
#include <QScrollArea>

namespace Akonadi {
class IncidenceChanger;
}

namespace EventViews {
class Agenda;
class AgendaItem;
class AgendaView;

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
    class Private;
    Private *const d;
};

class EVENTVIEWS_EXPORT Agenda : public QWidget
{
    Q_OBJECT
public:
    Agenda(AgendaView *agendaView, QScrollArea *scrollArea, int columns, int rows, int rowSize, bool isInteractive);

    Agenda(AgendaView *agendaView, QScrollArea *scrollArea, int columns, bool isInteractive);

    ~Agenda() override;

    Q_REQUIRED_RESULT KCalendarCore::Incidence::Ptr selectedIncidence() const;
    Q_REQUIRED_RESULT QDate selectedIncidenceDate() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QSize minimumSize() const;
    int minimumHeight() const;
    // QSizePolicy sizePolicy() const;
    Q_REQUIRED_RESULT int contentsY() const
    {
        return -y();
    }

    Q_REQUIRED_RESULT int contentsX() const
    {
        return x();
    }

    QScrollBar *verticalScrollBar() const;

    QScrollArea *scrollArea() const;

    Q_REQUIRED_RESULT AgendaItem::List agendaItems(const QString &uid) const;

    /**
      Returns the uid of the last incidence that was selected. This
      persists across reloads and clear, so that if the same uid
      reappears, it can be reselected.
    */
    Q_REQUIRED_RESULT QString lastSelectedItemUid() const;

    bool eventFilter(QObject *, QEvent *) override;

    void paintEvent(QPaintEvent *) override;

    Q_REQUIRED_RESULT QPoint contentsToGrid(const QPoint &pos) const;
    Q_REQUIRED_RESULT QPoint gridToContents(const QPoint &gpos) const;

    Q_REQUIRED_RESULT int timeToY(const QTime &time) const;
    Q_REQUIRED_RESULT QTime gyToTime(int y) const;

    Q_REQUIRED_RESULT QVector<int> minContentsY() const;
    Q_REQUIRED_RESULT QVector<int> maxContentsY() const;

    Q_REQUIRED_RESULT int visibleContentsYMin() const;
    Q_REQUIRED_RESULT int visibleContentsYMax() const;

    void setStartTime(QTime startHour);

    AgendaItem::QPtr insertItem(const KCalendarCore::Incidence::Ptr &incidence, const QDateTime &recurrenceId, int X, int YTop, int YBottom, int itemPos, int itemCount, bool isSelected);

    AgendaItem::QPtr insertAllDayItem(const KCalendarCore::Incidence::Ptr &event, const QDateTime &recurrenceId, int XBegin, int XEnd, bool isSelected);

    void insertMultiItem(const KCalendarCore::Incidence::Ptr &event, const QDateTime &recurrenceId, int XBegin, int XEnd, int YTop, int YBottom, bool isSelected);

    /**
      Removes an event and all its multi-items from the agenda. This function
      removes the items from the view, but doesn't delete them immediately.
      Instead, they are queued in mItemsToDelete and later deleted by the
      slot deleteItemsToDelete() (called by QTimer::singleShot ).
      @param incidence The pointer to the incidence that should be removed.
    */
    void removeIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    void changeColumns(int columns);

    Q_REQUIRED_RESULT int columns() const;
    Q_REQUIRED_RESULT int rows() const;

    Q_REQUIRED_RESULT double gridSpacingX() const;
    Q_REQUIRED_RESULT double gridSpacingY() const;

    void clear();

    /** Update configuration from preference settings */
    void updateConfig();

    void checkScrollBoundaries();

    void setHolidayMask(QVector<bool> *);

    void setDateList(const KCalendarCore::DateList &selectedDates);
    Q_REQUIRED_RESULT KCalendarCore::DateList dateList() const;

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
    void selectItem(const AgendaItem::QPtr &);

    /**
      Selects the item associated with a given Akonadi Item id.
      Linear search, use carefully.
      @param id the item id of the item that should be selected. If no such
      item exists, the selection is not changed.
    */
    void selectIncidenceByUid(const QString &id);
    void selectItem(const Akonadi::Item &item);

    bool removeAgendaItem(const AgendaItem::QPtr &item);
    void showAgendaItem(const AgendaItem::QPtr &item);

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
    void droppedIncidences(const KCalendarCore::Incidence::List &, const QPoint &gpos, bool allDay);
    void droppedIncidences(const QList<QUrl> &, const QPoint &gpos, bool allDay);

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

    AgendaItem::QPtr createAgendaItem(const KCalendarCore::Incidence::Ptr &incidence, int itemPos, int itemCount, const QDateTime &recurrentId, bool isSelected);

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
    MouseActionType isInResizeArea(bool horizontal, const QPoint &pos, const AgendaItem::QPtr &item);
    /** Return whether the cell specified by the grid point belongs to the current select
    */
    bool ptInSelection(const QPoint &gpos) const;

    /** Start selecting time span. */
    void startSelectAction(const QPoint &viewportPos);

    /** Select time span. */
    void performSelectAction(const QPoint &viewportPos);

    /** Emd selecting time span. */
    void endSelectAction(const QPoint &viewportPos);

    /** Start moving/resizing agenda item */
    void startItemAction(const QPoint &viewportPos);

    /** Move/resize agenda item */
    void performItemAction(const QPoint &viewportPos);

    /** End moving/resizing agenda item */
    void endItemAction();

    /** Set cursor, when no item action is in progress */
    void setNoActionCursor(const AgendaItem::QPtr &moveItem, const QPoint &viewportPos);
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
    void init();
    void marcus_bains();

private:
    class Private;
    Private *const d;
};

class AgendaScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    AgendaScrollArea(bool allDay, AgendaView *agendaView, bool isInteractive, QWidget *parent);
    ~AgendaScrollArea();

    Agenda *agenda() const;

private:
    Agenda *mAgenda = nullptr;
};
}

#endif

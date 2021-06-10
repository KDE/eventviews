/*
  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "viewcalendar.h"

#include <CalendarSupport/CellItem>

#include <Akonadi/Calendar/ETMCalendar>
#include <AkonadiCore/Item>

#include <QDateTime>
#include <QPointer>
#include <QWidget>

namespace EventViews
{
class AgendaItem;
class EventView;

struct MultiItemInfo {
    int mStartCellXLeft, mStartCellXRight;
    int mStartCellYTop, mStartCellYBottom;
    QPointer<AgendaItem> mFirstMultiItem;
    QPointer<AgendaItem> mPrevMultiItem;
    QPointer<AgendaItem> mNextMultiItem;
    QPointer<AgendaItem> mLastMultiItem;
};

/**
  @class AgendaItem

  @brief This class describes the widgets that represent the various calendar
         items in the agenda view

  The AgendaItem has to make sure that it receives all mouse events, which
  are to be used for dragging and resizing. That means it has to be installed
  as event filter for its children, if it has children, and it has to pass
  mouse events from the children to itself. See eventFilter().

  Some comments on the movement of multi-day items:
  Basically, the agenda items are arranged in two implicit double-linked lists.
  The mMultiItemInfo works like before to describe the currently viewed
  multi-item.
  When moving, new events might need to be added to the beginning or the end of
  the multi-item sequence, or events might need to be hidden. I cannot just
  delete this items, since I have to restore/show them if the move is reset
  (i.e. if a drag started). So internally, I keep another doubly-linked list
  which is longer than the one defined by mMultiItemInfo, but includes the
  multi-item sequence, too.

  The mStartMoveInfo stores the first and last item of the multi-item sequence
  when the move started. The prev and next members of mStartMoveInfo are used
  for that longer sequence including all (shown and hidden) items.
*/

class AgendaItem : public QWidget, public CalendarSupport::CellItem
{
    Q_OBJECT
public:
    using QPtr = QPointer<AgendaItem>;
    using List = QList<QPtr>;

    AgendaItem(EventView *eventView,
               const MultiViewCalendar::Ptr &calendar,
               const KCalendarCore::Incidence::Ptr &incidence,
               int itemPos,
               int itemCount,
               const QDateTime &qd,
               bool isSelected,
               QWidget *parent);
    ~AgendaItem() override;

    Q_REQUIRED_RESULT int cellXLeft() const
    {
        return mCellXLeft;
    }

    Q_REQUIRED_RESULT int cellXRight() const
    {
        return mCellXRight;
    }

    Q_REQUIRED_RESULT int cellYTop() const
    {
        return mCellYTop;
    }

    Q_REQUIRED_RESULT int cellYBottom() const
    {
        return mCellYBottom;
    }

    Q_REQUIRED_RESULT int cellHeight() const;
    Q_REQUIRED_RESULT int cellWidth() const;

    Q_REQUIRED_RESULT int itemPos() const
    {
        return mItemPos;
    }

    Q_REQUIRED_RESULT int itemCount() const
    {
        return mItemCount;
    }

    void setCellXY(int X, int YTop, int YBottom);
    void setCellY(int YTop, int YBottom);
    void setCellX(int XLeft, int XRight);
    void setCellXRight(int XRight);

    /** Start movement */
    void startMove();

    /** Reset to original values */
    void resetMove();

    /** End the movement (i.e. clean up) */
    void endMove();

    void moveRelative(int dx, int dy);

    /**
     * Expands the item's top.
     *
     * @param dy             delta y, number of units to be added to mCellYTop
     * @param allowOverLimit If false, the new mCellYTop can't be bigger than
     *                       mCellYBottom, instead, it gets mCellYBottom's value.
     *                       If true, @p dy is always added, regardless if mCellYTop
     *                       becomes bigger than mCellYBottom, this is useful when
     *                       moving items because it guarantees expandTop and the
     *                       following expandBottom call add the same value.
     */
    void expandTop(int dy, const bool allowOverLimit = false);
    void expandBottom(int dy);
    void expandLeft(int dx);
    void expandRight(int dx);

    Q_REQUIRED_RESULT bool isMultiItem() const;

    AgendaItem::QPtr prevMoveItem() const
    {
        return (mStartMoveInfo) ? (mStartMoveInfo->mPrevMultiItem) : nullptr;
    }

    AgendaItem::QPtr nextMoveItem() const
    {
        return (mStartMoveInfo) ? (mStartMoveInfo->mNextMultiItem) : nullptr;
    }

    MultiItemInfo *moveInfo() const
    {
        return mStartMoveInfo;
    }

    void setMultiItem(const AgendaItem::QPtr &first, const AgendaItem::QPtr &prev, const AgendaItem::QPtr &next, const AgendaItem::QPtr &last);

    AgendaItem::QPtr prependMoveItem(const AgendaItem::QPtr &);

    AgendaItem::QPtr appendMoveItem(const AgendaItem::QPtr &);

    AgendaItem::QPtr removeMoveItem(const AgendaItem::QPtr &);

    AgendaItem::QPtr firstMultiItem() const
    {
        return (mMultiItemInfo) ? (mMultiItemInfo->mFirstMultiItem) : nullptr;
    }

    AgendaItem::QPtr prevMultiItem() const
    {
        return (mMultiItemInfo) ? (mMultiItemInfo->mPrevMultiItem) : nullptr;
    }

    AgendaItem::QPtr nextMultiItem() const
    {
        return (mMultiItemInfo) ? (mMultiItemInfo->mNextMultiItem) : nullptr;
    }

    AgendaItem::QPtr lastMultiItem() const
    {
        return (mMultiItemInfo) ? (mMultiItemInfo->mLastMultiItem) : nullptr;
    }

    Q_REQUIRED_RESULT bool dissociateFromMultiItem();

    void setIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    const KCalendarCore::Incidence::Ptr &incidence() const
    {
        return mIncidence;
    }

    Q_REQUIRED_RESULT QDateTime occurrenceDateTime() const
    {
        return mOccurrenceDateTime;
    }

    Q_REQUIRED_RESULT QDate occurrenceDate() const;

    // /** Update the date of this item's occurrence (not in the event) */
    void setOccurrenceDateTime(const QDateTime &qd);

    void setText(const QString &text)
    {
        mLabelText = text;
    }

    Q_REQUIRED_RESULT QString text() const
    {
        return mLabelText;
    }

    QList<AgendaItem::QPtr> &conflictItems();
    void setConflictItems(const QList<AgendaItem::QPtr> &);
    void addConflictItem(const AgendaItem::QPtr &ci);

    QString label() const override;

    /** Tells whether this item overlaps item @p o */
    bool overlaps(CellItem *o) const override;

    void setResourceColor(const QColor &color)
    {
        mResourceColor = color;
    }

    Q_REQUIRED_RESULT QColor resourceColor() const
    {
        return mResourceColor;
    }

Q_SIGNALS:
    void removeAgendaItem(const AgendaItem::QPtr &);
    void showAgendaItem(const AgendaItem::QPtr &);

public Q_SLOTS:
    void updateIcons();
    void select(bool selected = true);
    void addAttendee(const QString &);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    bool event(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

    /** private movement functions. startMove needs to be called of only one of
     *  the multitems. it will then loop through the whole series using
     *  startMovePrivate. Same for resetMove and endMove */
    void startMovePrivate();
    void resetMovePrivate();
    void endMovePrivate();

    // Variables to remember start position
    MultiItemInfo *mStartMoveInfo = nullptr;
    // Color of the resource
    QColor mResourceColor;

private:
    void paintIcon(QPainter *p, int &x, int y, int ft);

    // paint all visible icons
    void paintIcons(QPainter *p, int &x, int y, int ft);

    void drawRoundedRect(QPainter *p, QRect rect, bool selected, const QColor &bgcolor, bool frame, int ft, bool roundTop, bool roundBottom);

    Q_REQUIRED_RESULT QColor getCategoryColor() const;
    Q_REQUIRED_RESULT QColor getFrameColor(const QColor &resourceColor, const QColor &categoryColor) const;
    Q_REQUIRED_RESULT QColor getBackgroundColor(const QColor &resourceColor, const QColor &categoryColor) const;

    int mCellXLeft, mCellXRight;
    int mCellYTop, mCellYBottom;

    EventView *mEventView = nullptr;
    MultiViewCalendar::Ptr mCalendar;
    KCalendarCore::Incidence::Ptr mIncidence;
    QDateTime mOccurrenceDateTime;
    bool mValid = true;
    bool mCloned = false;
    QString mLabelText;
    bool mSelected;
    bool mIconAlarm;
    bool mIconRecur;
    bool mIconReadonly;
    bool mIconReply;
    bool mIconGroup;
    bool mIconGroupTent;
    bool mIconOrganizer;
    bool mSpecialEvent;

    // For incidences that expand through more than 1 day
    // Will be 1 for single day incidences
    int mItemPos;
    int mItemCount;

    // Multi item pointers
    MultiItemInfo *mMultiItemInfo = nullptr;

    QList<AgendaItem::QPtr> mConflictItems;

    static QPixmap *alarmPxmp;
    static QPixmap *recurPxmp;
    static QPixmap *readonlyPxmp;
    static QPixmap *replyPxmp;
    static QPixmap *groupPxmp;
    static QPixmap *groupPxmpTent;
    static QPixmap *organizerPxmp;
    static QPixmap *eventPxmp;
    static QPixmap *todoPxmp;
    static QPixmap *completedPxmp;
};
}


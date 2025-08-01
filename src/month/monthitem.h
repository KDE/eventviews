/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "eventviews_export.h"

#include <Akonadi/CollectionCalendar>
#include <Akonadi/Item>
#include <KCalendarCore/Incidence>

#include <QDate>
#include <QObject>

namespace EventViews
{
class MonthGraphicsItem;
class MonthScene;

/**
 * A month item manages different MonthGraphicsItems.
 */
class EVENTVIEWS_EXPORT MonthItem : public QObject
{
    Q_OBJECT

public:
    explicit MonthItem(MonthScene *monthScene);
    ~MonthItem() override;

    [[nodiscard]] QWidget *parentWidget() const;

    /**
      Compares two items to decide which to place in the view first.

      The month view displays a list of items. When loading (which occurs each
      time there is a change), the items are sorted in an order intended to
      avoid unsightly gaps:
      - biggest durations first
      - earliest date
      - finally, time in the day
      Holidays are sorted before events with the same start date and length,
      so they appear at the top of the day's box.
    */
    static bool greaterThan(const MonthItem *e1, const MonthItem *e2);

    /**
      Compare this event with a second one, if the former function is not
      able to sort them.
    */
    virtual bool greaterThanFallback(const MonthItem *other) const;

    /**
      The start date of the incidence, generally realStartDate. But it
      reflect changes, even during move.
    */
    [[nodiscard]] QDate startDate() const;

    /**
      The end date of the incidence, generally realEndDate. But it
      reflect changes, even during move.
     */
    [[nodiscard]] QDate endDate() const;

    /**
      The number of days this item spans.
    */
    [[nodiscard]] int daySpan() const;

    /**
      This is the real start date, usually the start date of the incidence.
    */
    virtual QDate realStartDate() const = 0;

    /**
      This is the real end date, usually the end date of the incidence.
    */
    virtual QDate realEndDate() const = 0;

    /**
      True if this item last all the day.
    */
    virtual bool allDay() const = 0;

    /**
      Updates geometry of all MonthGraphicsItems.
    */
    void updateGeometry();

    /**
      Find the lowest possible position for this item.

      The position of an item in a cell is it's vertical position. This is used
      to avoid overlapping of items. An item keeps the same position in every
      cell it crosses. The position is measured from top to bottom.
    */
    void updatePosition();

    /**
      Returns true if this item is selected.
    */
    [[nodiscard]] bool selected() const
    {
        return mSelected;
    }

    /**
      Returns the position of the item ( > 0 ).
    */
    [[nodiscard]] int position() const
    {
        return mPosition;
    }

    /**
      Returns the associated month scene to this item.
    */
    [[nodiscard]] MonthScene *monthScene() const
    {
        return mMonthScene;
    }

    /**
      Begin a move.
    */
    void beginMove();

    /**
      End a move.
    */
    void endMove();

    /**
      Begin a resize.
    */
    void beginResize();

    /**
      End a resize.
    */
    void endResize();

    /**
      Called during move to move the item a bit, relative to the previous
      move step.
    */
    void moveBy(int offsetToPreviousDate);

    /**
     * Called during a drag to move the item to a particular date.
     * An invalid date indicates a drag outside the month grid.
     */
    void moveTo(QDate date);

    /**
      Called during resize to resize the item a bit, relative to the previous
      resize step.
    */
    bool resizeBy(int offsetToPreviousDate);

    /**
      Returns true if the item is being moved.
    */
    [[nodiscard]] bool isMoving() const
    {
        return mMoving;
    }

    /**
      Returns true if the item is being resized.
    */
    [[nodiscard]] bool isResizing() const
    {
        return mResizing;
    }

    /**
      Returns true if the item can be moved.
    */
    virtual bool isMoveable() const = 0;

    /**
      Returns true if the item can be resized.
    */
    virtual bool isResizable() const = 0;

    /**
      Deletes all MonthGraphicsItem this item handles. Clear the list.
    */
    void deleteAll();

    /**
      Update the monthgraphicsitems

      This basically deletes and rebuild all the MonthGraphicsItems but tries
        to do it wisely:
      - If there is a moving item, it won't be deleted because then the
        new item won't receive anymore the MouseMove events.
      - If there is an item on a line where the new state needs an item,
        it is used and not deleted. This will avoid flickers.
    */
    void updateMonthGraphicsItems();

    /**
      Sets the selection state of this item.
    */
    void setSelected(bool selected)
    {
        mSelected = selected;
    }

    // METHODS NEEDED TO PAINT ITEMS

    /**
      Returns the text to draw in an item.

     @param end True if the text at the end of an item should be returned.
    */
    virtual QString text(bool end) const = 0;

    /**
       Returns the text for the tooltip of the item
     */
    virtual QString toolTipText(const QDate &date) const = 0;

    /**
      Returns the background color of the item.
    */
    [[nodiscard]] virtual QColor bgColor() const = 0;

    /**
      Returns the frame color of the item.
    */
    [[nodiscard]] virtual QColor frameColor() const = 0;

    /**
      Returns a list of pixmaps to draw next to the items.
    */
    virtual QList<QPixmap> icons() const = 0;

    QList<MonthGraphicsItem *> monthGraphicsItems() const;

protected:
    /**
      Called after a move operation.
    */
    virtual void finalizeMove(const QDate &newStartDate) = 0;

    /**
      Called after a resize operation.
    */
    virtual void finalizeResize(const QDate &newStartDate, const QDate &newEndDate) = 0;

private:
    /**
      Sets the value of all MonthGraphicsItem to @param z.
    */
    EVENTVIEWS_NO_EXPORT void setZValue(qreal z);

    QList<MonthGraphicsItem *> mMonthGraphicsItemList;

    MonthScene *mMonthScene = nullptr;

    bool mSelected = false;
    bool mMoving = false; // during move
    bool mResizing = false; // during resize
    QDate mOverrideStartDate = QDate();
    int mOverrideDaySpan = 0;

    int mPosition = 0;
};

class EVENTVIEWS_EXPORT IncidenceMonthItem : public MonthItem
{
    Q_OBJECT

public:
    IncidenceMonthItem(MonthScene *monthScene,
                       const Akonadi::CollectionCalendar::Ptr &calendar,
                       const Akonadi::Item &item,
                       const KCalendarCore::Incidence::Ptr &incidence,
                       QDate recurStartDate = QDate());

    ~IncidenceMonthItem() override;

    KCalendarCore::Incidence::Ptr incidence() const;
    Akonadi::Item akonadiItem() const;
    Akonadi::Item::Id akonadiItemId() const;
    Akonadi::CollectionCalendar::Ptr calendar() const;

    bool greaterThanFallback(const MonthItem *other) const override;

    QDate realStartDate() const override;
    QDate realEndDate() const override;
    bool allDay() const override;

    bool isMoveable() const override;
    bool isResizable() const override;

    QString text(bool end) const override;
    QString toolTipText(const QDate &date) const override;

    QColor bgColor() const override;
    QColor frameColor() const override;

    QList<QPixmap> icons() const override;

protected:
    void finalizeMove(const QDate &newStartDate) override;
    void finalizeResize(const QDate &newStartDate, const QDate &newEndDate) override;

protected Q_SLOTS:
    /**
      Update the selected state of this item.
      If will be selected if incidence is the incidence managed by this item.
      Else it will be deselected.
    */
    void updateSelection(const Akonadi::Item &incidence, QDate date);

private:
    EVENTVIEWS_NO_EXPORT void updateDates(int startOffset, int endOffset);

    EVENTVIEWS_NO_EXPORT void setNewDates(const KCalendarCore::Incidence::Ptr &incidence, int startOffset, int endOffset);

    /**
      Returns the category color for this incidence.
    */
    QColor catColor() const;

    Akonadi::CollectionCalendar::Ptr mCalendar;
    KCalendarCore::Incidence::Ptr mIncidence;
    Akonadi::Item::Id mAkonadiItemId;
    int mRecurDayOffset;
    bool mIsEvent, mIsTodo, mIsJournal;
};

class EVENTVIEWS_EXPORT HolidayMonthItem : public MonthItem
{
    Q_OBJECT

public:
    HolidayMonthItem(MonthScene *monthScene, QDate date, const QString &name);
    HolidayMonthItem(MonthScene *monthScene, QDate startDate, QDate endDate, const QString &name);
    ~HolidayMonthItem() override;

    bool greaterThanFallback(const MonthItem *other) const override;

    QDate realStartDate() const override
    {
        return mStartDate;
    }

    QDate realEndDate() const override
    {
        return mEndDate;
    }

    bool allDay() const override
    {
        return true;
    }

    bool isMoveable() const override
    {
        return false;
    }

    bool isResizable() const override
    {
        return false;
    }

    QString text(bool end) const override
    {
        Q_UNUSED(end)
        return mName;
    }

    QString toolTipText(const QDate &) const override
    {
        return mName;
    }

    QColor bgColor() const override;
    QColor frameColor() const override;

    QList<QPixmap> icons() const override;

protected:
    void finalizeMove(const QDate &newStartDate) override;
    void finalizeResize(const QDate &newStartDate, const QDate &newEndDate) override;

private:
    QDate mStartDate;
    QDate mEndDate;
    QString mName;
};
}

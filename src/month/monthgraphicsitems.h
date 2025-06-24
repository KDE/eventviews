/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include <QDate>
#include <QGraphicsItem>

namespace EventViews
{
class MonthItem;

/**
 * Graphics items which indicates that the view can be scrolled to display
 * more events.
 */
class ScrollIndicator : public QGraphicsItem
{
public:
    enum ArrowDirection {
        UpArrow,
        DownArrow
    };

    explicit ScrollIndicator(ArrowDirection direction);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    [[nodiscard]] ArrowDirection direction() const
    {
        return mDirection;
    }

private:
    ArrowDirection mDirection;

    static const int mWidth = 30;
    static const int mHeight = 10;
};

/**
 * Keeps information about a month cell.
 */
class MonthCell
{
public:
    MonthCell(int id, QDate date, QGraphicsScene *scene);
    ~MonthCell();

    /**
      This is used to get the height of the minimum height (vertical position)
      in the month cells.
    */
    QList<MonthItem *> mMonthItemList;

    QHash<int, MonthItem *> mHeightHash;

    [[nodiscard]] int firstFreeSpace();
    void addMonthItem(MonthItem *manager, int height);

    [[nodiscard]] int id() const
    {
        return mId;
    }

    [[nodiscard]] QDate date() const
    {
        return mDate;
    }

    [[nodiscard]] int x() const
    {
        return mId % 7;
    }

    [[nodiscard]] int y() const
    {
        return mId / 7;
    }

    static int topMargin();
    // returns true if the cell contains events below the height @p height
    bool hasEventBelow(int height);

    // TODO : move this to a new GUI class (monthcell could be GraphicsItems)
    ScrollIndicator *upArrow() const
    {
        return mUpArrow;
    }

    ScrollIndicator *downArrow() const
    {
        return mDownArrow;
    }

private:
    int mId;
    QDate mDate;

    QGraphicsScene *mScene = nullptr;

    ScrollIndicator *mUpArrow = nullptr;
    ScrollIndicator *mDownArrow = nullptr;

    // disable copy ctor
    MonthCell(const MonthCell &) = delete;
    MonthCell &operator=(const MonthCell &) = delete;
};

/**
 * A MonthGraphicsItem representing a part of an event. There should be
 * one part per row = week
 */
class MonthGraphicsItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    using List = QList<MonthGraphicsItem *>;

    explicit MonthGraphicsItem(MonthItem *manager);
    ~MonthGraphicsItem() override;

    /**
      Change QGraphicsItem pos and boundingRect in the scene
      according to the incidence start and end date.
    */
    void updateGeometry();

    /**
      Returns the associated MonthItem.
    */
    MonthItem *monthItem() const
    {
        return mMonthItem;
    }

    /**
      Returns the starting date of this item.
    */
    [[nodiscard]] QDate startDate() const;

    /**
      Returns the number of day this item spans on minus one
      to be compatible with QDate::addDays().
    */
    [[nodiscard]] int daySpan() const;

    /**
      Computed from startDate() and daySpan().
    */
    [[nodiscard]] QDate endDate() const;

    void setStartDate(QDate d);
    void setDaySpan(int span);

    /**
      Returns true if this item is currently being moved (ie. the
      associated MonthItem is being moved).
    */
    [[nodiscard]] bool isMoving() const;

    /**
      Returns true if this item is currently being resized (ie. the
      associated MonthItem is being moved).
    */
    [[nodiscard]] bool isResizing() const;

    /**
      Returns true if this MonthGraphicsItem is the first one of the
      MonthItem ones.
    */
    [[nodiscard]] bool isBeginItem() const;

    /**
      Returns true if this MonthGraphicsItem is the last one of the
      MonthItem ones.
    */
    [[nodiscard]] bool isEndItem() const;

    /**
      Reimplemented from QGraphicsItem
    */
    QRectF boundingRect() const override;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;

    [[nodiscard]] QString getToolTip() const;

private:
    // Shape of the item, see shape()
    [[nodiscard]] QPainterPath widgetPath(bool border) const;

    // See startDate()
    QDate mStartDate;

    // See daySpan()
    int mDaySpan;

    // The current item is part of a MonthItem
    MonthItem *mMonthItem = nullptr;
};
}

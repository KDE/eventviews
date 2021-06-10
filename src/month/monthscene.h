/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include <Collection>
#include <Item>

#include <QBasicTimer>
#include <QDate>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMap>

namespace Akonadi
{
class IncidenceChanger;
}

namespace EventViews
{
class MonthCell;
class MonthItem;
class MonthView;
class ScrollIndicator;

class MonthScene : public QGraphicsScene
{
    Q_OBJECT

    enum ActionType { None, Move, Resize };

public:
    enum ResizeType { ResizeLeft, ResizeRight };

    explicit MonthScene(MonthView *parent);
    ~MonthScene() override;

    Q_REQUIRED_RESULT int columnWidth() const;
    Q_REQUIRED_RESULT int rowHeight() const;

    MonthCell *firstCellForMonthItem(MonthItem *manager);
    Q_REQUIRED_RESULT int height(MonthItem *manager);
    Q_REQUIRED_RESULT int itemHeight();
    Q_REQUIRED_RESULT int itemHeightIncludingSpacing();
    QList<MonthItem *> mManagerList;
    MonthView *mMonthView = nullptr;

    MonthView *monthView() const
    {
        return mMonthView;
    }

    QMap<QDate, MonthCell *> mMonthCellMap;

    Q_REQUIRED_RESULT bool initialized() const
    {
        return mInitialized;
    }

    void setInitialized(bool i)
    {
        mInitialized = i;
    }

    void resetAll();
    Akonadi::IncidenceChanger *incidenceChanger() const;

    int totalHeight();

    /**
     * Returns the vertical position where the top of the cell should be
     * painted taking in account margins, rowHeight
     */
    Q_REQUIRED_RESULT int cellVerticalPos(const MonthCell *cell) const;

    /**
     * Idem, for the horizontal position
     */
    Q_REQUIRED_RESULT int cellHorizontalPos(const MonthCell *cell) const;

    /**
      Select item. If the argument is 0, the currently selected item gets
      deselected. This function emits the itemSelected(bool) signal to inform
      about selection/deselection of events.
    */
    void selectItem(MonthItem *);
    Q_REQUIRED_RESULT int maxRowCount();

    MonthCell *selectedCell() const;
    MonthCell *previousCell() const;

    /**
      Get the space on the right of the cell associated to the date @p date.
    */
    Q_REQUIRED_RESULT int getRightSpan(QDate date) const;

    /**
      Get the space on the left of the cell associated to the date @p date.
    */
    Q_REQUIRED_RESULT int getLeftSpan(QDate date) const;

    /**
      Returns the date in the first column of the row given by @p row.
    */
    Q_REQUIRED_RESULT QDate firstDateOnRow(int row) const;

    /**
      Calls updateGeometry() on each MonthItem
    */
    void updateGeometry();

    /**
      Returns the first height. Used for scrolling

      @see MonthItem::height()
    */
    Q_REQUIRED_RESULT int startHeight() const
    {
        return mStartHeight;
    }

    /**
      Set the current height using @p height.
      If height = 0, then the view is not scrolled. Else it will be scrolled
      by step of one item.
    */
    void setStartHeight(int height)
    {
        mStartHeight = height;
    }

    /**
      Returns the resize type.
    */
    Q_REQUIRED_RESULT ResizeType resizeType() const
    {
        return mResizeType;
    }

    /**
      Returns the currently selected item.
    */
    MonthItem *selectedItem()
    {
        return mSelectedItem;
    }

    Q_REQUIRED_RESULT QPixmap birthdayPixmap() const
    {
        return mBirthdayPixmap;
    }

    Q_REQUIRED_RESULT QPixmap anniversaryPixmap() const
    {
        return mAnniversaryPixmap;
    }

    Q_REQUIRED_RESULT QPixmap alarmPixmap() const
    {
        return mAlarmPixmap;
    }

    Q_REQUIRED_RESULT QPixmap recurPixmap() const
    {
        return mRecurPixmap;
    }

    Q_REQUIRED_RESULT QPixmap readonlyPixmap() const
    {
        return mReadonlyPixmap;
    }

    Q_REQUIRED_RESULT QPixmap replyPixmap() const
    {
        return mReplyPixmap;
    }

    Q_REQUIRED_RESULT QPixmap holidayPixmap() const
    {
        return mHolidayPixmap;
    }

    /**
       Removes an incidence from the scene
    */
    void removeIncidence(const QString &uid);

Q_SIGNALS:
    void incidenceSelected(const Akonadi::Item &incidence, const QDate &);
    void showIncidencePopupSignal(const Akonadi::Item &, const QDate &);
    void newEventSignal();
    void showNewEventPopupSignal();

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void wheelEvent(QGraphicsSceneWheelEvent *wheelEvent) override;
    void timerEvent(QTimerEvent *e) override;
    void helpEvent(QGraphicsSceneHelpEvent *helpEvent) override;
    /**
       Scrolls all incidences in cells up
     */
    virtual void scrollCellsUp();

    /**
       Scrolls all incidences in cells down
    */
    virtual void scrollCellsDown();

    /**
       A click on a scroll indicator has occurred
       TODO : move this handler to the scrollindicator
    */
    virtual void clickOnScrollIndicator(ScrollIndicator *scrollItem);

    /**
      Handles drag and drop events. Called from eventFilter.
    */
    //    virtual bool eventFilter_drag( QObject *, QDropEvent * );

    /**
      Returns true if the last item is visible in the given @p cell.
    */
    bool lastItemFit(MonthCell *cell);

private:
    /**
     * Returns the height of the header of the view
     */
    int headerHeight() const;

    int availableWidth() const;

    /**
     * Height available to draw the cells. Doesn't include header.
     */
    int availableHeight() const;

    /**
     * Removes all the margins, frames, etc. to give the
     * X coordinate in the MonthGrid.
     */
    int sceneXToMonthGridX(int xScene);

    /**
     * Removes all the margins, frames, headers etc. to give the
     * Y coordinate in the MonthGrid.
     */
    int sceneYToMonthGridY(int yScene);

    /**
     * Given a pos in the scene coordinates,
     * returns the cell containing @p pos.
     */
    MonthCell *getCellFromPos(QPointF pos);

    /**
       Returns true if (x, y) is in the monthgrid, false else.
    */
    bool isInMonthGrid(int x, int y) const;

    bool mInitialized;

    // User interaction.
    MonthItem *mClickedItem = nullptr; // todo ini in ctor
    MonthItem *mActionItem = nullptr;
    bool mActionInitiated;

    MonthItem *mSelectedItem = nullptr;
    QDate mSelectedCellDate;
    MonthCell *mStartCell = nullptr; // start cell when dragging
    MonthCell *mPreviousCell = nullptr; // the cell before that one during dragging

    ActionType mActionType;
    ResizeType mResizeType;

    // The item height at the top of the cell. This is generally 0 unless
    // the user scroll the view when there are too many items.
    int mStartHeight;

    // icons to draw in front of the events
    QPixmap mEventPixmap;
    QPixmap mBirthdayPixmap;
    QPixmap mAnniversaryPixmap;
    QPixmap mTodoPixmap;
    QPixmap mTodoDonePixmap;
    QPixmap mJournalPixmap;
    QPixmap mAlarmPixmap;
    QPixmap mRecurPixmap;
    QPixmap mReadonlyPixmap;
    QPixmap mReplyPixmap;
    QPixmap mHolidayPixmap;
    QBasicTimer repeatTimer;
    ScrollIndicator *mCurrentIndicator = nullptr;
    friend class MonthGraphicsView;
};

/**
 * Renders a MonthScene
 */
class MonthGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MonthGraphicsView(MonthView *parent);

    /**
      Draws the cells.
    */
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    void setScene(MonthScene *scene);

    /**
      Change the cursor according to @p actionType.
    */
    void setActionCursor(MonthScene::ActionType actionType);

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    MonthScene *mScene = nullptr;
    MonthView *mMonthView = nullptr;
};
}


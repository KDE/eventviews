/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "monthgraphicsitems.h"
#include "eventview.h"
#include "helper.h"
#include "monthitem.h"
#include "monthscene.h"
#include "monthview.h"
#include "prefs.h"

#include <QGraphicsScene>
#include <QPainter>

using namespace EventViews;

ScrollIndicator::ScrollIndicator(ScrollIndicator::ArrowDirection dir)
    : mDirection(dir)
{
    setZValue(200); // on top of everything
    hide();
}

QRectF ScrollIndicator::boundingRect() const
{
    // NOLINTBEGIN(bugprone-integer-division)
    return {-mWidth / 2, -mHeight / 2, mWidth, mHeight};
    // NOLINTEND(bugprone-integer-division)
}

void ScrollIndicator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);

    QPolygon arrow(3);
    if (mDirection == ScrollIndicator::UpArrow) {
        arrow.setPoint(0, 0, -mHeight / 2);
        arrow.setPoint(1, mWidth / 2, mHeight / 2);
        arrow.setPoint(2, -mWidth / 2, mHeight / 2);
    } else if (mDirection == ScrollIndicator::DownArrow) { // down
        arrow.setPoint(1, mWidth / 2, -mHeight / 2);
        arrow.setPoint(2, -mWidth / 2, -mHeight / 2);
        arrow.setPoint(0, 0, mHeight / 2);
    }
    QColor color(QPalette().color(QPalette::WindowText));
    color.setAlpha(155);
    painter->setBrush(color);
    painter->setPen(color);
    painter->drawPolygon(arrow);
}

//-------------------------------------------------------------
MonthCell::MonthCell(int id, QDate date, QGraphicsScene *scene)
    : mId(id)
    , mDate(date)
    , mScene(scene)
{
    mUpArrow = new ScrollIndicator(ScrollIndicator::UpArrow);
    mDownArrow = new ScrollIndicator(ScrollIndicator::DownArrow);
    mScene->addItem(mUpArrow);
    mScene->addItem(mDownArrow);
}

MonthCell::~MonthCell()
{
    mScene->removeItem(mUpArrow);
    mScene->removeItem(mDownArrow);
    delete mUpArrow; // we've taken ownership, so this is safe
    delete mDownArrow;
}

bool MonthCell::hasEventBelow(int height)
{
    if (mHeightHash.isEmpty()) {
        return false;
    }

    for (int i = 0; i < height; ++i) {
        if (mHeightHash.value(i) != nullptr) {
            return true;
        }
    }

    return false;
}

int MonthCell::topMargin()
{
    return 18;
}

void MonthCell::addMonthItem(MonthItem *manager, int height)
{
    mHeightHash[height] = manager;
}

int MonthCell::firstFreeSpace()
{
    const MonthItem *manager = nullptr;
    int i = 0;
    while (true) {
        manager = mHeightHash[i];
        if (manager == nullptr) {
            return i;
        }
        i++;
    }
}

//-------------------------------------------------------------
// MONTHGRAPHICSITEM
static const int ft = 1; // frame thickness

MonthGraphicsItem::MonthGraphicsItem(MonthItem *manager)
    : QGraphicsItem(nullptr)
    , mMonthItem(manager)
{
    manager->monthScene()->addItem(this);
    QTransform transform;
    transform = transform.translate(0.5, 0.5);
    setTransform(transform);
}

MonthGraphicsItem::~MonthGraphicsItem() = default;

bool MonthGraphicsItem::isMoving() const
{
    return mMonthItem->isMoving();
}

bool MonthGraphicsItem::isEndItem() const
{
    return startDate().addDays(daySpan()) == mMonthItem->endDate();
}

bool MonthGraphicsItem::isBeginItem() const
{
    return startDate() == mMonthItem->startDate();
}

QPainterPath MonthGraphicsItem::shape() const
{
    // The returned shape must be a closed path,
    // otherwise MonthScene:itemAt(pos) can have
    // problems detecting the item
    return widgetPath(false);
}

// TODO: remove this method.
QPainterPath MonthGraphicsItem::widgetPath(bool border) const
{
    // If border is set we won't draw all the path. Items spanning on multiple
    // rows won't have borders on their boundaries.
    // If this is the mask, we draw it one pixel bigger
    const int x0 = (!border && !isBeginItem()) ? -1 : 0;
    const int y0 = 0;
    const int x1 = static_cast<int>(boundingRect().width());
    const int y1 = static_cast<int>(boundingRect().height());

    const int beginRound = 2;
    const int margin = 1;

    QPainterPath path(QPoint(x0 + beginRound, y0));
    if (isBeginItem()) {
        path.quadTo(QPoint(x0 + margin, y0), QPoint(x0 + margin, y0 + beginRound));
        path.lineTo(QPoint(x0 + margin, y1 - beginRound));
        path.quadTo(QPoint(x0 + margin, y1), QPoint(x0 + beginRound + margin, y1));
    } else {
        path.lineTo(x0, y0);
        if (!border) {
            path.lineTo(x0, y1);
        } else {
            path.moveTo(x0, y1);
        }
        path.lineTo(x0 + beginRound, y1);
    }

    if (isEndItem()) {
        path.lineTo(x1 - beginRound, y1);
        path.quadTo(QPoint(x1 - margin, y1), QPoint(x1 - margin, y1 - beginRound));
        path.lineTo(QPoint(x1 - margin, y0 + beginRound));
        path.quadTo(QPoint(x1 - margin, y0), QPoint(x1 - margin - beginRound, y0));
    } else {
        path.lineTo(x1, y1);
        if (!border) {
            path.lineTo(x1, y0);
        } else {
            path.moveTo(x1, y0);
        }
    }

    // close path
    path.lineTo(x0 + beginRound, y0);

    return path;
}

QRectF MonthGraphicsItem::boundingRect() const
{
    // width - 2 because of the cell-dividing line with width == 1 at beginning and end
    return QRectF(0, 0, (daySpan() + 1) * mMonthItem->monthScene()->columnWidth() - 2, mMonthItem->monthScene()->itemHeight());
}

void MonthGraphicsItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!mMonthItem->monthScene()->initialized()) {
        return;
    }

    MonthScene *scene = mMonthItem->monthScene();

    int const textMargin = 7;

    QColor bgColor = mMonthItem->bgColor();
    bgColor = mMonthItem->selected() ? bgColor.lighter(EventView::BRIGHTNESS_FACTOR) : bgColor;
    QColor frameColor = mMonthItem->frameColor();
    frameColor = mMonthItem->selected() ? frameColor.lighter(EventView::BRIGHTNESS_FACTOR) : frameColor;
    QColor const textColor = EventViews::getTextColor(bgColor);

    // make moving or resizing items translucent
    if (mMonthItem->isMoving() || mMonthItem->isResizing()) {
        bgColor.setAlphaF(0.75F);
    }

    // draw the widget without border
    p->setRenderHint(QPainter::Antialiasing, false);
    p->setBrush(bgColor);
    p->setPen(Qt::NoPen);
    p->drawPath(widgetPath(false));

    p->setRenderHint(QPainter::Antialiasing, true);
    // draw the border without fill
    const QPen pen(frameColor, ft, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawPath(widgetPath(true));

    // draw text
    p->setPen(textColor);

    int alignFlag = Qt::AlignVCenter;
    if (isBeginItem()) {
        alignFlag |= Qt::AlignLeft;
    } else if (isEndItem()) {
        alignFlag |= Qt::AlignRight;
    } else {
        alignFlag |= Qt::AlignHCenter;
    }

    // !isBeginItem() is not always isEndItem()
    QString text = mMonthItem->text(!isBeginItem());
    p->setFont(mMonthItem->monthScene()->monthView()->preferences()->monthViewFont());

    // Every item should set its own LayoutDirection, or eliding fails miserably
    p->setLayoutDirection(text.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);

    QRect textRect = QRect(textMargin, 0, static_cast<int>(boundingRect().width() - 2 * textMargin), scene->itemHeight());

    if (mMonthItem->monthScene()->monthView()->preferences()->enableMonthItemIcons()) {
        const QList<QPixmap> icons = mMonthItem->icons();
        int iconWidths = 0;

        for (const QPixmap &icon : icons) {
            iconWidths += icon.width();
        }

        if (!icons.isEmpty()) {
            // add some margin between the icons and the text
            iconWidths += textMargin / 2;
        }

        int textWidth = p->fontMetrics().size(0, text).width();
        if (textWidth + iconWidths > textRect.width()) {
            textWidth = textRect.width() - iconWidths;
            text = p->fontMetrics().elidedText(text, Qt::ElideRight, textWidth);
        }

        int curXPos = textRect.left();
        if (alignFlag & Qt::AlignRight) {
            curXPos += textRect.width() - textWidth - iconWidths;
        } else if (alignFlag & Qt::AlignHCenter) {
            curXPos += (textRect.width() - textWidth - iconWidths) / 2;
        }
        alignFlag &= ~(Qt::AlignRight | Qt::AlignCenter);
        alignFlag |= Qt::AlignLeft;

        // update the rect, where the text will be displayed
        textRect.setLeft(curXPos + iconWidths);

        const int iconHeightMax = 16; // we always use 16x16 icons
        int const pixYPos = icons.isEmpty() ? 0 : (textRect.height() - iconHeightMax) / 2;
        for (const QPixmap &icon : std::as_const(icons)) {
            p->drawPixmap(curXPos, pixYPos, icon);
            curXPos += icon.width();
        }

        p->drawText(textRect, alignFlag | Qt::AlignVCenter, text);
    } else {
        text = p->fontMetrics().elidedText(text, Qt::ElideRight, textRect.width());
        p->drawText(textRect, alignFlag, text);
    }
}

void MonthGraphicsItem::setStartDate(QDate date)
{
    mStartDate = date;
}

QDate MonthGraphicsItem::endDate() const
{
    return startDate().addDays(daySpan());
}

QDate MonthGraphicsItem::startDate() const
{
    return mStartDate;
}

void MonthGraphicsItem::setDaySpan(int span)
{
    mDaySpan = span;
}

int MonthGraphicsItem::daySpan() const
{
    return mDaySpan;
}

void MonthGraphicsItem::updateGeometry()
{
    const MonthCell *cell = mMonthItem->monthScene()->mMonthCellMap.value(startDate());

    // If the item is moving and this one is moved outside the view,
    // cell will be null
    if (mMonthItem->isMoving() && !cell) {
        hide();
        return;
    }

    Q_ASSERT(cell);

    prepareGeometryChange();

    int const beginX = 1 + mMonthItem->monthScene()->cellHorizontalPos(cell);
    int beginY = 1 + cell->topMargin() + mMonthItem->monthScene()->cellVerticalPos(cell);

    beginY += mMonthItem->position() * mMonthItem->monthScene()->itemHeightIncludingSpacing()
        - mMonthItem->monthScene()->startHeight() * mMonthItem->monthScene()->itemHeightIncludingSpacing(); // scrolling

    setPos(beginX, beginY);

    if (mMonthItem->position() < mMonthItem->monthScene()->startHeight()
        || mMonthItem->position() - mMonthItem->monthScene()->startHeight() >= mMonthItem->monthScene()->maxRowCount()) {
        hide();
    } else {
        show();
        update();
    }
}

QString MonthGraphicsItem::getToolTip() const
{
    return mMonthItem->toolTipText(mStartDate);
}

#include "moc_monthgraphicsitems.cpp"

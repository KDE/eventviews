/*
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  Marcus Bains line.
  SPDX-FileCopyrightText: 2001 Ali Rahimi <ali@mit.edu>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "agenda.h"
#include "agendaview.h"
#include "prefs.h"

#include <Akonadi/CalendarUtils>
#include <Akonadi/IncidenceChanger>
#include <CalendarSupport/Utils>

#include <KCalendarCore/Incidence>

#include <KCalUtils/RecurrenceActions>

#include "calendarview_debug.h"
#include <KMessageBox>

#include <KLocalizedString>
#include <QApplication>
#include <QHash>
#include <QLabel>
#include <QMouseEvent>
#include <QMultiHash>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

#include <chrono>
#include <cmath>

using namespace std::chrono_literals; // for fabs()

using namespace EventViews;

///////////////////////////////////////////////////////////////////////////////
class EventViews::MarcusBainsPrivate
{
public:
    MarcusBainsPrivate(EventView *eventView, Agenda *agenda)
        : mEventView(eventView)
        , mAgenda(agenda)
    {
    }

    [[nodiscard]] int todayColumn() const;

    EventView *const mEventView;
    Agenda *const mAgenda;
    QTimer *mTimer = nullptr;
    QLabel *mTimeBox = nullptr; // Label showing the current time
    QDateTime mOldDateTime;
    int mOldTodayCol = -1;
};

int MarcusBainsPrivate::todayColumn() const
{
    const QDate currentDate = QDate::currentDate();

    int col = 0;
    const KCalendarCore::DateList dateList = mAgenda->dateList();
    for (const QDate &date : dateList) {
        if (date == currentDate) {
            return QApplication::isRightToLeft() ? mAgenda->columns() - 1 - col : col;
        }
        ++col;
    }

    return -1;
}

MarcusBains::MarcusBains(EventView *eventView, Agenda *agenda)
    : QFrame(agenda)
    , d(new MarcusBainsPrivate(eventView, agenda))
{
    d->mTimeBox = new QLabel(d->mAgenda);
    d->mTimeBox->setAlignment(Qt::AlignRight | Qt::AlignBottom);

    d->mTimer = new QTimer(this);
    d->mTimer->setSingleShot(true);
    connect(d->mTimer, &QTimer::timeout, this, &MarcusBains::updateLocation);
    d->mTimer->start(0);
}

MarcusBains::~MarcusBains() = default;

void MarcusBains::updateLocation()
{
    updateLocationRecalc();
}

void MarcusBains::updateLocationRecalc(bool recalculate)
{
    const bool showSeconds = d->mEventView->preferences()->marcusBainsShowSeconds();
    const QColor color = d->mEventView->preferences()->agendaMarcusBainsLineLineColor();

    const QDateTime now = QDateTime::currentDateTime();
    const QTime time = now.time();

    if (now.date() != d->mOldDateTime.date()) {
        recalculate = true; // New day
    }
    const int todayCol = recalculate ? d->todayColumn() : d->mOldTodayCol;

    // Number of minutes since beginning of the day
    const int minutes = time.hour() * 60 + time.minute();
    const int minutesPerCell = 24 * 60 / d->mAgenda->rows();

    d->mOldDateTime = now;
    d->mOldTodayCol = todayCol;

    int y = int(minutes * d->mAgenda->gridSpacingY() / minutesPerCell);
    int x = int(d->mAgenda->gridSpacingX() * todayCol);

    bool const hideIt = !(d->mEventView->preferences()->marcusBainsEnabled());
    if (!isHidden() && (hideIt || (todayCol < 0))) {
        hide();
        d->mTimeBox->hide();
        return;
    }

    if (isHidden() && !hideIt) {
        show();
        d->mTimeBox->show();
    }

    /* Line */
    // It seems logical to adjust the line width with the label's font weight
    const int fw = d->mEventView->preferences()->agendaMarcusBainsLineFont().weight();
    setLineWidth(1 + abs(fw - QFont::Normal) / QFont::Light);
    setFrameStyle(QFrame::HLine | QFrame::Plain);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, color); // for Oxygen
    pal.setColor(QPalette::WindowText, color); // for Plastique
    setPalette(pal);
    if (recalculate) {
        setFixedSize(int(d->mAgenda->gridSpacingX()), 1);
    }
    move(x, y);
    raise();

    /* Label */
    d->mTimeBox->setFont(d->mEventView->preferences()->agendaMarcusBainsLineFont());
    QPalette pal1 = d->mTimeBox->palette();
    pal1.setColor(QPalette::WindowText, color);
    if (!d->mEventView->preferences()->useSystemColor()) {
        pal1.setColor(QPalette::Window, d->mEventView->preferences()->agendaGridBackgroundColor());
    } else {
        pal1.setColor(QPalette::Window, palette().color(QPalette::AlternateBase));
    }
    d->mTimeBox->setPalette(pal1);
    d->mTimeBox->setAutoFillBackground(true);
    d->mTimeBox->setText(QLocale::system().toString(time, showSeconds ? QLocale::LongFormat : QLocale::ShortFormat));
    d->mTimeBox->adjustSize();
    if (y - d->mTimeBox->height() >= 0) {
        y -= d->mTimeBox->height();
    } else {
        y++;
    }
    if (x - d->mTimeBox->width() + d->mAgenda->gridSpacingX() > 0) {
        x += int(d->mAgenda->gridSpacingX() - d->mTimeBox->width() - 1);
    } else {
        x++;
    }
    d->mTimeBox->move(x, y);
    d->mTimeBox->raise();

    if (showSeconds || recalculate) {
        d->mTimer->start(1s);
    } else {
        d->mTimer->start(1000 * (60 - time.second()));
    }
}

////////////////////////////////////////////////////////////////////////////

class EventViews::AgendaPrivate
{
public:
    AgendaPrivate(AgendaView *agendaView, QScrollArea *scrollArea, int columns, int rows, int rowSize, bool isInteractive)
        : mAgendaView(agendaView)
        , mScrollArea(scrollArea)
        , mColumns(columns)
        , mRows(rows)
        , mGridSpacingY(rowSize)
        , mDesiredGridSpacingY(rowSize)
        , mIsInteractive(isInteractive)
    {
        if (mGridSpacingY < 4 || mGridSpacingY > 30) {
            mGridSpacingY = 10;
        }
    }

    PrefsPtr preferences() const
    {
        return mAgendaView->preferences();
    }

    bool isQueuedForDeletion(const QString &uid) const
    {
        // if mAgendaItemsById contains it it means that a createAgendaItem() was called
        // before the previous agenda items were deleted.
        return mItemsQueuedForDeletion.contains(uid) && !mAgendaItemsById.contains(uid);
    }

    QMultiHash<QString, AgendaItem::QPtr> mAgendaItemsById; // A QMultiHash because recurring incs
                                                            // might have many agenda items
    QSet<QString> mItemsQueuedForDeletion;

    AgendaView *mAgendaView = nullptr;
    QScrollArea *mScrollArea = nullptr;

    bool mAllDayMode{false};

    // Number of Columns/Rows of agenda grid
    int mColumns;
    int mRows;

    // Width and height of agenda cells. mDesiredGridSpacingY is the height
    // set in the config. The actual height might be larger since otherwise
    // more than 24 hours might be displayed.
    double mGridSpacingX{0.0};
    double mGridSpacingY;
    double mDesiredGridSpacingY;

    Akonadi::IncidenceChanger *mChanger = nullptr;

    // size of border, where mouse action will resize the AgendaItem
    int mResizeBorderWidth{0};

    // size of border, where mouse mve will cause a scroll of the agenda
    int mScrollBorderWidth{0};
    int mScrollDelay{0};
    int mScrollOffset{0};

    QTimer mScrollUpTimer;
    QTimer mScrollDownTimer;

    // Cells to store Move and Resize coordinates while performing the action
    QPoint mStartCell;
    QPoint mEndCell;

    // Working Hour coordinates
    bool mWorkingHoursEnable{false};
    QList<bool> *mHolidayMask = nullptr;
    int mWorkingHoursYTop{0};
    int mWorkingHoursYBottom{0};

    // Selection
    bool mHasSelection{false};
    QPoint mSelectionStartPoint;
    QPoint mSelectionStartCell;
    QPoint mSelectionEndCell;

    // List of dates to be displayed
    KCalendarCore::DateList mSelectedDates;

    // The AgendaItem, which has been right-clicked last
    QPointer<AgendaItem> mClickedItem;

    // The AgendaItem, which is being moved/resized
    QPointer<AgendaItem> mActionItem;

    // Currently selected item
    QPointer<AgendaItem> mSelectedItem;
    // Uid of the last selected incidence. Used for reselecting in situations
    // where the selected item points to a no longer valid incidence, for
    // example during resource reload.
    QString mSelectedId;

    // The Marcus Bains Line widget.
    MarcusBains *mMarcusBains = nullptr;

    Agenda::MouseActionType mActionType{Agenda::NOP};

    bool mItemMoved{false};

    // List of all Items contained in agenda
    QList<AgendaItem::QPtr> mItems;
    QList<AgendaItem::QPtr> mItemsToDelete;

    int mOldLowerScrollValue{0};
    int mOldUpperScrollValue{0};

    bool mIsInteractive;

    MultiViewCalendar::Ptr mCalendar;
};

/*
  Create an agenda widget with rows rows and columns columns.
*/
Agenda::Agenda(AgendaView *agendaView, QScrollArea *scrollArea, int columns, int rows, int rowSize, bool isInteractive)
    : QWidget(scrollArea)
    , d(new AgendaPrivate(agendaView, scrollArea, columns, rows, rowSize, isInteractive))
{
    setMouseTracking(true);

    init();
}

/*
  Create an agenda widget with columns columns and one row. This is used for
  all-day events.
*/
Agenda::Agenda(AgendaView *agendaView, QScrollArea *scrollArea, int columns, bool isInteractive)
    : QWidget(scrollArea)
    , d(new AgendaPrivate(agendaView, scrollArea, columns, 1, 24, isInteractive))
{
    d->mAllDayMode = true;

    init();
}

Agenda::~Agenda()
{
    delete d->mMarcusBains;
}

KCalendarCore::Incidence::Ptr Agenda::selectedIncidence() const
{
    return d->mSelectedItem ? d->mSelectedItem->incidence() : KCalendarCore::Incidence::Ptr();
}

QDate Agenda::selectedIncidenceDate() const
{
    return d->mSelectedItem ? d->mSelectedItem->occurrenceDate() : QDate();
}

QString Agenda::lastSelectedItemUid() const
{
    return d->mSelectedId;
}

void Agenda::init()
{
    setAttribute(Qt::WA_OpaquePaintEvent);

    d->mGridSpacingX = static_cast<double>(d->mScrollArea->width()) / d->mColumns;
    d->mDesiredGridSpacingY = d->preferences()->hourSize();
    if (d->mDesiredGridSpacingY < 4 || d->mDesiredGridSpacingY > 30) {
        d->mDesiredGridSpacingY = 10;
    }

    // make sure that there are not more than 24 per day
    d->mGridSpacingY = static_cast<double>(height()) / d->mRows;
    if (d->mGridSpacingY < d->mDesiredGridSpacingY) {
        d->mGridSpacingY = d->mDesiredGridSpacingY;
    }

    d->mResizeBorderWidth = 12;
    d->mScrollBorderWidth = 12;
    d->mScrollDelay = 30;
    d->mScrollOffset = 10;

    // Grab key strokes for keyboard navigation of agenda. Seems to have no
    // effect. Has to be fixed.
    setFocusPolicy(Qt::WheelFocus);

    connect(&d->mScrollUpTimer, &QTimer::timeout, this, &Agenda::scrollUp);
    connect(&d->mScrollDownTimer, &QTimer::timeout, this, &Agenda::scrollDown);

    d->mStartCell = QPoint(0, 0);
    d->mEndCell = QPoint(0, 0);

    d->mHasSelection = false;
    d->mSelectionStartPoint = QPoint(0, 0);
    d->mSelectionStartCell = QPoint(0, 0);
    d->mSelectionEndCell = QPoint(0, 0);

    d->mOldLowerScrollValue = -1;
    d->mOldUpperScrollValue = -1;

    d->mClickedItem = nullptr;

    d->mActionItem = nullptr;
    d->mActionType = NOP;
    d->mItemMoved = false;

    d->mSelectedItem = nullptr;

    setAcceptDrops(true);
    installEventFilter(this);

    /*  resizeContents(int(mGridSpacingX * mColumns), int(mGridSpacingY * mRows)); */

    d->mScrollArea->viewport()->update();
    //  mScrollArea->viewport()->setAttribute(Qt::WA_NoSystemBackground, true);
    d->mScrollArea->viewport()->setFocusPolicy(Qt::WheelFocus);

    calculateWorkingHours();

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, qOverload<int>(&Agenda::checkScrollBoundaries));

    // Create the Marcus Bains line.
    if (d->mAllDayMode) {
        d->mMarcusBains = nullptr;
    } else {
        d->mMarcusBains = new MarcusBains(d->mAgendaView, this);
    }
}

void Agenda::clear()
{
    qDeleteAll(d->mItems);
    qDeleteAll(d->mItemsToDelete);
    d->mItems.clear();
    d->mItemsToDelete.clear();
    d->mAgendaItemsById.clear();
    d->mItemsQueuedForDeletion.clear();

    d->mSelectedItem = nullptr;

    clearSelection();
}

void Agenda::clearSelection()
{
    d->mHasSelection = false;
    d->mActionType = NOP;
    update();
}

void Agenda::marcus_bains()
{
    if (d->mMarcusBains) {
        d->mMarcusBains->updateLocationRecalc(true);
    }
}

void Agenda::changeColumns(int columns)
{
    if (columns == 0) {
        qCDebug(CALENDARVIEW_LOG) << "called with argument 0";
        return;
    }

    clear();
    d->mColumns = columns;
    //  setMinimumSize(mColumns * 10, mGridSpacingY + 1);
    //  init();
    //  update();

    QResizeEvent event(size(), size());

    QApplication::sendEvent(this, &event);
}

int Agenda::columns() const
{
    return d->mColumns;
}

int Agenda::rows() const
{
    return d->mRows;
}

double Agenda::gridSpacingX() const
{
    return d->mGridSpacingX;
}

double Agenda::gridSpacingY() const
{
    return d->mGridSpacingY;
}

/*
  This is the eventFilter function, which gets all events from the AgendaItems
  contained in the agenda. It has to handle moving and resizing for all items.
*/
bool Agenda::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        return eventFilter_mouse(object, static_cast<QMouseEvent *>(event));
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        return eventFilter_wheel(object, static_cast<QWheelEvent *>(event));
#endif
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        return eventFilter_key(object, static_cast<QKeyEvent *>(event));

    case QEvent::Leave:
#ifndef QT_NO_CURSOR
        if (!d->mActionItem) {
            setCursor(Qt::ArrowCursor);
        }
#endif

        if (object == this) {
            // so timelabels hide the mouse cursor
            Q_EMIT leaveAgenda();
        }
        return true;

    case QEvent::Enter:
        Q_EMIT enterAgenda();
        return QWidget::eventFilter(object, event);

#ifndef QT_NO_DRAGANDDROP
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
        //  case QEvent::DragResponse:
        return eventFilter_drag(object, static_cast<QDropEvent *>(event));
#endif

    default:
        return QWidget::eventFilter(object, event);
    }
}

bool Agenda::eventFilter_drag(QObject *obj, QDropEvent *de)
{
#ifndef QT_NO_DRAGANDDROP
    const QMimeData *md = de->mimeData();

    switch (de->type()) {
    case QEvent::DragEnter:
    case QEvent::DragMove:
        if (!CalendarSupport::canDecode(md)) {
            return false;
        }

        if (CalendarSupport::mimeDataHasIncidence(md)) {
            de->accept();
        } else {
            de->ignore();
        }
        return true;
        break;
    case QEvent::DragLeave:
        return false;
        break;
    case QEvent::Drop: {
        if (!CalendarSupport::canDecode(md)) {
            return false;
        }

        const QList<QUrl> incidenceUrls = CalendarSupport::incidenceItemUrls(md);
        const KCalendarCore::Incidence::List incidences = CalendarSupport::incidences(md);

        Q_ASSERT(!incidenceUrls.isEmpty() || !incidences.isEmpty());

        de->setDropAction(Qt::MoveAction);

        QWidget *dropTarget = qobject_cast<QWidget *>(obj);
        QPoint dropPosition = de->position().toPoint();
        if (dropTarget && dropTarget != this) {
            dropPosition = dropTarget->mapTo(this, dropPosition);
        }

        const QPoint gridPosition = contentsToGrid(dropPosition);
        if (!incidenceUrls.isEmpty()) {
            Q_EMIT droppedUrlsSignal(incidenceUrls, gridPosition, d->mAllDayMode);
        } else {
            Q_EMIT droppedIncidencesSignal(incidences, gridPosition, d->mAllDayMode);
        }
        return true;
    }

    case QEvent::DragResponse:
    default:
        break;
    }
#endif
    return false;
}

#ifndef QT_NO_WHEELEVENT
bool Agenda::eventFilter_wheel(QObject *object, QWheelEvent *e)
{
    QPoint viewportPos;
    bool accepted = false;
    const QPoint pos = e->position().toPoint();
    if ((e->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) {
        if (object != this) {
            const QWidget *w = static_cast<QWidget *>(object);
            viewportPos = w->mapToParent(pos);
        } else {
            viewportPos = pos;
        }
        // qCDebug(CALENDARVIEW_LOG) << type:" << e->type() << "angleDelta:" << e->angleDelta();
        Q_EMIT zoomView(-e->angleDelta().y(), contentsToGrid(viewportPos), Qt::Horizontal);
        accepted = true;
    }

    if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
        if (object != this) {
            const QWidget *w = static_cast<QWidget *>(object);
            viewportPos = w->mapToParent(pos);
        } else {
            viewportPos = pos;
        }
        Q_EMIT zoomView(-e->angleDelta().y(), contentsToGrid(viewportPos), Qt::Vertical);
        Q_EMIT mousePosSignal(gridToContents(contentsToGrid(viewportPos)));
        accepted = true;
    }
    if (accepted) {
        e->accept();
    }
    return accepted;
}

#endif

bool Agenda::eventFilter_key(QObject *, QKeyEvent *ke)
{
    return d->mAgendaView->processKeyEvent(ke);
}

bool Agenda::eventFilter_mouse(QObject *object, QMouseEvent *me)
{
    QPoint viewportPos;
    if (object != this) {
        viewportPos = static_cast<QWidget *>(object)->mapToParent(me->pos());
    } else {
        viewportPos = me->pos();
    }

    switch (me->type()) {
    case QEvent::MouseButtonPress:
        if (object != this) {
            if (me->button() == Qt::RightButton) {
                d->mClickedItem = qobject_cast<AgendaItem *>(object);
                if (d->mClickedItem) {
                    selectItem(d->mClickedItem);
                    Q_EMIT showIncidencePopupSignal(d->mClickedItem->incidence(), d->mClickedItem->occurrenceDate());
                }
            } else {
                AgendaItem::QPtr const item = qobject_cast<AgendaItem *>(object);
                if (item) {
                    KCalendarCore::Incidence::Ptr const incidence = item->incidence();
                    if (incidence->isReadOnly()) {
                        d->mActionItem = nullptr;
                    } else {
                        d->mActionItem = item;
                        startItemAction(viewportPos);
                    }
                    // Warning: do selectItem() as late as possible, since all
                    // sorts of things happen during this call. Some can lead to
                    // this filter being run again and mActionItem being set to
                    // null.
                    selectItem(item);
                }
            }
        } else {
            if (me->button() == Qt::RightButton) {
                // if mouse pointer is not in selection, select the cell below the cursor
                QPoint const gpos = contentsToGrid(viewportPos);
                if (!ptInSelection(gpos)) {
                    d->mSelectionStartCell = gpos;
                    d->mSelectionEndCell = gpos;
                    d->mHasSelection = true;
                    Q_EMIT newStartSelectSignal();
                    Q_EMIT newTimeSpanSignal(d->mSelectionStartCell, d->mSelectionEndCell);
                    //          updateContents();
                }
                Q_EMIT showNewEventPopupSignal();
            } else {
                selectItem(nullptr);
                d->mActionItem = nullptr;
#ifndef QT_NO_CURSOR
                setCursor(Qt::ArrowCursor);
#endif
                startSelectAction(viewportPos);
                update();
            }
        }
        break;

    case QEvent::MouseButtonRelease:
        if (d->mActionItem) {
            endItemAction();
        } else if (d->mActionType == SELECT) {
            endSelectAction(viewportPos);
        }
        // This nasty gridToContents(contentsToGrid(..)) is needed to
        // avoid an offset of a few pixels. Don't ask me why...
        Q_EMIT mousePosSignal(gridToContents(contentsToGrid(viewportPos)));
        break;

    case QEvent::MouseMove: {
        if (!d->mIsInteractive) {
            return true;
        }

        // This nasty gridToContents(contentsToGrid(..)) is needed todos
        // avoid an offset of a few pixels. Don't ask me why...
        QPoint indicatorPos = gridToContents(contentsToGrid(viewportPos));
        if (object != this) {
            AgendaItem::QPtr const moveItem = qobject_cast<AgendaItem *>(object);
            KCalendarCore::Incidence::Ptr const incidence = moveItem ? moveItem->incidence() : KCalendarCore::Incidence::Ptr();
            if (incidence && !incidence->isReadOnly()) {
                if (!d->mActionItem) {
                    setNoActionCursor(moveItem, viewportPos);
                } else {
                    performItemAction(viewportPos);

                    if (d->mActionType == MOVE) {
                        // show cursor at the current begin of the item
                        AgendaItem::QPtr firstItem = d->mActionItem->firstMultiItem();
                        if (!firstItem) {
                            firstItem = d->mActionItem;
                        }
                        indicatorPos = gridToContents(QPoint(firstItem->cellXLeft(), firstItem->cellYTop()));
                    } else if (d->mActionType == RESIZEBOTTOM) {
                        // RESIZETOP is handled correctly, only resizebottom works differently
                        indicatorPos = gridToContents(QPoint(d->mActionItem->cellXLeft(), d->mActionItem->cellYBottom() + 1));
                    }
                } // If we have an action item
            } // If move item && !read only
        } else {
            if (d->mActionType == SELECT) {
                performSelectAction(viewportPos);

                // show cursor at end of timespan
                if (((d->mStartCell.y() < d->mEndCell.y()) && (d->mEndCell.x() >= d->mStartCell.x())) || (d->mEndCell.x() > d->mStartCell.x())) {
                    indicatorPos = gridToContents(QPoint(d->mEndCell.x(), d->mEndCell.y() + 1));
                } else {
                    indicatorPos = gridToContents(d->mEndCell);
                }
            }
        }
        Q_EMIT mousePosSignal(indicatorPos);
        break;
    }

    case QEvent::MouseButtonDblClick:
        if (object == this) {
            selectItem(nullptr);
            Q_EMIT newEventSignal();
        } else {
            AgendaItem::QPtr const doubleClickedItem = qobject_cast<AgendaItem *>(object);
            if (doubleClickedItem) {
                selectItem(doubleClickedItem);
                Q_EMIT editIncidenceSignal(doubleClickedItem->incidence());
            }
        }
        break;

    default:
        break;
    }

    return true;
}

bool Agenda::ptInSelection(QPoint gpos) const
{
    // NOLINTBEGIN(bugprone-branch-clone)
    if (!d->mHasSelection) {
        return false;
    } else if (gpos.x() < d->mSelectionStartCell.x() || gpos.x() > d->mSelectionEndCell.x()) {
        return false;
    } else if ((gpos.x() == d->mSelectionStartCell.x()) && (gpos.y() < d->mSelectionStartCell.y())) {
        return false;
    } else if ((gpos.x() == d->mSelectionEndCell.x()) && (gpos.y() > d->mSelectionEndCell.y())) {
        return false;
    }
    return true;
    // NOLINTEND(bugprone-branch-clone)
}

void Agenda::startSelectAction(QPoint viewportPos)
{
    Q_EMIT newStartSelectSignal();

    d->mActionType = SELECT;
    d->mSelectionStartPoint = viewportPos;
    d->mHasSelection = true;

    QPoint const pos = viewportPos;
    QPoint const gpos = contentsToGrid(pos);

    // Store new selection
    d->mStartCell = gpos;
    d->mEndCell = gpos;
    d->mSelectionStartCell = gpos;
    d->mSelectionEndCell = gpos;

    //  updateContents();
}

void Agenda::performSelectAction(QPoint pos)
{
    const QPoint gpos = contentsToGrid(pos);

    // Scroll if cursor was moved to upper or lower end of agenda.
    if (pos.y() - contentsY() < d->mScrollBorderWidth && contentsY() > 0) {
        d->mScrollUpTimer.start(d->mScrollDelay);
    } else if (contentsY() + d->mScrollArea->viewport()->height() - d->mScrollBorderWidth < pos.y()) {
        d->mScrollDownTimer.start(d->mScrollDelay);
    } else {
        d->mScrollUpTimer.stop();
        d->mScrollDownTimer.stop();
    }

    if (gpos != d->mEndCell) {
        d->mEndCell = gpos;
        if (d->mStartCell.x() > d->mEndCell.x() || (d->mStartCell.x() == d->mEndCell.x() && d->mStartCell.y() > d->mEndCell.y())) {
            // backward selection
            d->mSelectionStartCell = d->mEndCell;
            d->mSelectionEndCell = d->mStartCell;
        } else {
            d->mSelectionStartCell = d->mStartCell;
            d->mSelectionEndCell = d->mEndCell;
        }

        update();
    }
}

void Agenda::endSelectAction(const QPoint &viewportPos)
{
    d->mScrollUpTimer.stop();
    d->mScrollDownTimer.stop();

    d->mActionType = NOP;

    Q_EMIT newTimeSpanSignal(d->mSelectionStartCell, d->mSelectionEndCell);

    if (d->preferences()->selectionStartsEditor()) {
        if ((d->mSelectionStartPoint - viewportPos).manhattanLength() > QApplication::startDragDistance()) {
            Q_EMIT newEventSignal();
        }
    }
}

Agenda::MouseActionType Agenda::isInResizeArea(bool horizontal, QPoint pos, const AgendaItem::QPtr &item)
{
    if (!item) {
        return NOP;
    }
    QPoint const gridpos = contentsToGrid(pos);
    QPoint const contpos = gridToContents(gridpos + QPoint((QApplication::isRightToLeft()) ? 1 : 0, 0));

    if (horizontal) {
        int clXLeft = item->cellXLeft();
        int clXRight = item->cellXRight();
        if (QApplication::isRightToLeft()) {
            int const tmp = clXLeft;
            clXLeft = clXRight;
            clXRight = tmp;
        }
        int const gridDistanceX = int(pos.x() - contpos.x());
        if (gridDistanceX < d->mResizeBorderWidth && clXLeft == gridpos.x()) {
            if (QApplication::isRightToLeft()) {
                return RESIZERIGHT;
            } else {
                return RESIZELEFT;
            }
        } else if ((d->mGridSpacingX - gridDistanceX) < d->mResizeBorderWidth && clXRight == gridpos.x()) {
            if (QApplication::isRightToLeft()) {
                return RESIZELEFT;
            } else {
                return RESIZERIGHT;
            }
        } else {
            return MOVE;
        }
    } else {
        int const gridDistanceY = int(pos.y() - contpos.y());
        if (gridDistanceY < d->mResizeBorderWidth && item->cellYTop() == gridpos.y() && !item->firstMultiItem()) {
            return RESIZETOP;
        } else if ((d->mGridSpacingY - gridDistanceY) < d->mResizeBorderWidth && item->cellYBottom() == gridpos.y() && !item->lastMultiItem()) {
            return RESIZEBOTTOM;
        } else {
            return MOVE;
        }
    }
}

void Agenda::startItemAction(const QPoint &pos)
{
    Q_ASSERT(d->mActionItem);

    d->mStartCell = contentsToGrid(pos);
    d->mEndCell = d->mStartCell;

    bool const noResize = CalendarSupport::hasTodo(d->mActionItem->incidence());

    d->mActionType = MOVE;
    if (!noResize) {
        d->mActionType = isInResizeArea(d->mAllDayMode, pos, d->mActionItem);
    }

    d->mActionItem->startMove();
    setActionCursor(d->mActionType, true);
}

void Agenda::performItemAction(QPoint pos)
{
    QPoint const gpos = contentsToGrid(pos);

    // Cursor left active agenda area.
    // This starts a drag.
    if (pos.y() < 0 || pos.y() >= contentsY() + d->mScrollArea->viewport()->height() || pos.x() < 0 || pos.x() >= width()) {
        if (d->mActionType == MOVE) {
            d->mScrollUpTimer.stop();
            d->mScrollDownTimer.stop();
            d->mActionItem->resetMove();
            placeSubCells(d->mActionItem);
            Q_EMIT startDragSignal(d->mActionItem->incidence());
#ifndef QT_NO_CURSOR
            setCursor(Qt::ArrowCursor);
#endif
            if (d->mChanger) {
                //        d->mChanger->cancelChange(d->mActionItem->incidence());
            }
            d->mActionItem = nullptr;
            d->mActionType = NOP;
            d->mItemMoved = false;
            return;
        }
    } else {
        setActionCursor(d->mActionType, true);
    }

    // Scroll if item was moved to upper or lower end of agenda.
    const int distanceToTop = pos.y() - contentsY();
    if (distanceToTop < d->mScrollBorderWidth && distanceToTop > -d->mScrollBorderWidth) {
        d->mScrollUpTimer.start(d->mScrollDelay);
    } else if (contentsY() + d->mScrollArea->viewport()->height() - d->mScrollBorderWidth < pos.y()) {
        d->mScrollDownTimer.start(d->mScrollDelay);
    } else {
        d->mScrollUpTimer.stop();
        d->mScrollDownTimer.stop();
    }

    // Move or resize item if necessary
    if (d->mEndCell != gpos) {
        if (!d->mItemMoved) {
            if (!d->mChanger) {
                KMessageBox::information(this,
                                         i18nc("@info",
                                               "Unable to lock item for modification. "
                                               "You cannot make any changes."),
                                         i18nc("@title:window", "Locking Failed"),
                                         QStringLiteral("AgendaLockingFailed"));
                d->mScrollUpTimer.stop();
                d->mScrollDownTimer.stop();
                d->mActionItem->resetMove();
                placeSubCells(d->mActionItem);
#ifndef QT_NO_CURSOR
                setCursor(Qt::ArrowCursor);
#endif
                d->mActionItem = nullptr;
                d->mActionType = NOP;
                d->mItemMoved = false;
                return;
            }
            d->mItemMoved = true;
        }
        d->mActionItem->raise();
        if (d->mActionType == MOVE) {
            // Move all items belonging to a multi item
            AgendaItem::QPtr firstItem = d->mActionItem->firstMultiItem();
            if (!firstItem) {
                firstItem = d->mActionItem;
            }
            AgendaItem::QPtr lastItem = d->mActionItem->lastMultiItem();
            if (!lastItem) {
                lastItem = d->mActionItem;
            }
            QPoint const deltapos = gpos - d->mEndCell;
            AgendaItem::QPtr moveItem = firstItem;
            while (moveItem) {
                bool changed = false;
                if (deltapos.x() != 0) {
                    moveItem->moveRelative(deltapos.x(), 0);
                    changed = true;
                }
                // in all day view don't try to move multi items, since there are none
                if (moveItem == firstItem && !d->mAllDayMode) { // is the first item
                    int const newY = deltapos.y() + moveItem->cellYTop();
                    // If event start moved earlier than 0:00, it starts the previous day
                    if (newY < 0 && newY > d->mScrollBorderWidth) {
                        moveItem->expandTop(-moveItem->cellYTop());
                        // prepend a new item at (x-1, rows()+newY to rows())
                        AgendaItem::QPtr newFirst = firstItem->prevMoveItem();
                        // cell's y values are first and last cell of the bar,
                        // so if newY=-1, they need to be the same
                        if (newFirst) {
                            newFirst->setCellXY(moveItem->cellXLeft() - 1, rows() + newY, rows() - 1);
                            d->mItems.append(newFirst);
                            moveItem->resize(int(d->mGridSpacingX * newFirst->cellWidth()), int(d->mGridSpacingY * newFirst->cellHeight()));
                            QPoint const cpos = gridToContents(QPoint(newFirst->cellXLeft(), newFirst->cellYTop()));
                            newFirst->setParent(this);
                            newFirst->move(cpos.x(), cpos.y());
                        } else {
                            newFirst = insertItem(moveItem->incidence(),
                                                  moveItem->occurrenceDateTime(),
                                                  moveItem->cellXLeft() - 1,
                                                  rows() + newY,
                                                  rows() - 1,
                                                  moveItem->itemPos(),
                                                  moveItem->itemCount(),
                                                  false);
                        }
                        if (newFirst) {
                            newFirst->show();
                        }
                        moveItem->prependMoveItem(newFirst);
                        firstItem = newFirst;
                    } else if (newY >= rows()) {
                        // If event start is moved past 24:00, it starts the next day
                        // erase current item (i.e. remove it from the multiItem list)
                        firstItem = moveItem->nextMultiItem();
                        moveItem->hide();
                        d->mItems.removeAll(moveItem);
                        //            removeChild(moveItem);
                        d->mActionItem->removeMoveItem(moveItem);
                        moveItem = firstItem;
                        // adjust next day's item
                        if (moveItem) {
                            moveItem->expandTop(rows() - newY);
                        }
                    } else {
                        moveItem->expandTop(deltapos.y(), true);
                    }
                    changed = true;
                }
                if (moveItem && !moveItem->lastMultiItem() && !d->mAllDayMode) { // is the last item
                    int const newY = deltapos.y() + moveItem->cellYBottom();
                    if (newY < 0) {
                        // erase current item
                        lastItem = moveItem->prevMultiItem();
                        moveItem->hide();
                        d->mItems.removeAll(moveItem);
                        //            removeChild(moveItem);
                        moveItem->removeMoveItem(moveItem);
                        moveItem = lastItem;
                        moveItem->expandBottom(newY + 1);
                    } else if (newY >= rows()) {
                        moveItem->expandBottom(rows() - moveItem->cellYBottom() - 1);
                        // append item at (x+1, 0 to newY-rows())
                        AgendaItem::QPtr newLast = lastItem->nextMoveItem();
                        if (newLast) {
                            newLast->setCellXY(moveItem->cellXLeft() + 1, 0, newY - rows() - 1);
                            d->mItems.append(newLast);
                            moveItem->resize(int(d->mGridSpacingX * newLast->cellWidth()), int(d->mGridSpacingY * newLast->cellHeight()));
                            QPoint const cpos = gridToContents(QPoint(newLast->cellXLeft(), newLast->cellYTop()));
                            newLast->setParent(this);
                            newLast->move(cpos.x(), cpos.y());
                        } else {
                            newLast = insertItem(moveItem->incidence(),
                                                 moveItem->occurrenceDateTime(),
                                                 moveItem->cellXLeft() + 1,
                                                 0,
                                                 newY - rows() - 1,
                                                 moveItem->itemPos(),
                                                 moveItem->itemCount(),
                                                 false);
                        }
                        moveItem->appendMoveItem(newLast);
                        newLast->show();
                        lastItem = newLast;
                    } else {
                        moveItem->expandBottom(deltapos.y());
                    }
                    changed = true;
                }
                if (changed) {
                    adjustItemPosition(moveItem);
                }
                if (moveItem) {
                    moveItem = moveItem->nextMultiItem();
                }
            }
        } else if (d->mActionType == RESIZETOP) {
            if (d->mEndCell.y() <= d->mActionItem->cellYBottom()) {
                d->mActionItem->expandTop(gpos.y() - d->mEndCell.y());
                adjustItemPosition(d->mActionItem);
            }
        } else if (d->mActionType == RESIZEBOTTOM) {
            if (d->mEndCell.y() >= d->mActionItem->cellYTop()) {
                d->mActionItem->expandBottom(gpos.y() - d->mEndCell.y());
                adjustItemPosition(d->mActionItem);
            }
        } else if (d->mActionType == RESIZELEFT) {
            if (d->mEndCell.x() <= d->mActionItem->cellXRight()) {
                d->mActionItem->expandLeft(gpos.x() - d->mEndCell.x());
                adjustItemPosition(d->mActionItem);
            }
        } else if (d->mActionType == RESIZERIGHT) {
            if (d->mEndCell.x() >= d->mActionItem->cellXLeft()) {
                d->mActionItem->expandRight(gpos.x() - d->mEndCell.x());
                adjustItemPosition(d->mActionItem);
            }
        }
        d->mEndCell = gpos;
    }
}

void Agenda::endItemAction()
{
    // PENDING(AKONADI_PORT) review all this cloning and changer calls
    d->mActionType = NOP;
    d->mScrollUpTimer.stop();
    d->mScrollDownTimer.stop();
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif

    if (!d->mChanger) {
        qCCritical(CALENDARVIEW_LOG) << "No IncidenceChanger set";
        return;
    }

    bool multiModify = false;
    // FIXME: do the cloning here...
    KCalendarCore::Incidence::Ptr incidence = d->mActionItem->incidence();
    const auto recurrenceId = d->mActionItem->occurrenceDateTime();

    d->mItemMoved = d->mItemMoved && !(d->mStartCell.x() == d->mEndCell.x() && d->mStartCell.y() == d->mEndCell.y());

    if (d->mItemMoved) {
        bool addIncidence = false;
        bool modify = false;

        // get the main event and not the exception
        if (incidence->hasRecurrenceId() && !incidence->recurs()) {
            KCalendarCore::Incidence::Ptr mainIncidence;
            KCalendarCore::Calendar::Ptr const cal = d->mCalendar->findCalendar(incidence)->getCalendar();
            if (CalendarSupport::hasEvent(incidence)) {
                mainIncidence = cal->event(incidence->uid());
            } else if (CalendarSupport::hasTodo(incidence)) {
                mainIncidence = cal->todo(incidence->uid());
            }
            incidence = mainIncidence;
        }

        Akonadi::Item item = d->mCalendar->item(incidence);
        if (incidence && incidence->recurs()) {
            const int res = d->mAgendaView->showMoveRecurDialog(incidence, recurrenceId.date());

            if (!d->mActionItem) {
                qCWarning(CALENDARVIEW_LOG) << "mActionItem was reset while the 'move' dialog was active";
                d->mItemMoved = false;
                return;
            }

            switch (res) {
            case KCalUtils::RecurrenceActions::AllOccurrences: // All occurrences
                // Moving the whole sequence of events is handled by the itemModified below.
                modify = true;
                break;
            case KCalUtils::RecurrenceActions::SelectedOccurrence:
            case KCalUtils::RecurrenceActions::FutureOccurrences: {
                const bool thisAndFuture = (res == KCalUtils::RecurrenceActions::FutureOccurrences);
                modify = true;
                multiModify = true;
                d->mChanger->startAtomicOperation(i18nc("@info/plain", "Dissociate event from recurrence"));
                KCalendarCore::Incidence::Ptr const newInc(KCalendarCore::Calendar::createException(incidence, recurrenceId, thisAndFuture));
                if (newInc) {
                    newInc->removeCustomProperty("VOLATILE", "AKONADI-ID");
                    Akonadi::Item const newItem = d->mCalendar->item(newInc);

                    if (newItem.isValid() && newItem != item) { // it is not a new exception
                        item = newItem;
                        newInc->setCustomProperty("VOLATILE", "AKONADI-ID", QString::number(newItem.id()));
                        addIncidence = false;
                    } else {
                        addIncidence = true;
                    }
                    // don't recreate items, they already have the correct position
                    d->mAgendaView->enableAgendaUpdate(false);

                    d->mActionItem->setIncidence(newInc);
                    (void)d->mActionItem->dissociateFromMultiItem(); // returns false if not a multi-item. we don't care in this case

                    d->mAgendaView->enableAgendaUpdate(true);
                } else {
                    KMessageBox::error(this,
                                       i18nc("@info",
                                             "Unable to add the exception item to the calendar. "
                                             "No change will be done."),
                                       i18nc("@title:window", "Error Occurred"));
                }
                break;
            }
            default:
                modify = false;
                d->mActionItem->resetMove();
                placeSubCells(d->mActionItem); // PENDING(AKONADI_PORT) should this be done after
                // the new item was asynchronously added?
            }
        }

        AgendaItem::QPtr placeItem = d->mActionItem->firstMultiItem();
        if (!placeItem) {
            placeItem = d->mActionItem;
        }

        Akonadi::Collection::Id saveCollection = -1;

        if (item.isValid()) {
            saveCollection = item.parentCollection().id();

            // if parent collection is only a search collection for example
            if (!(item.parentCollection().rights() & Akonadi::Collection::CanCreateItem)) {
                saveCollection = item.storageCollectionId();
            }
        }

        if (modify) {
            d->mActionItem->endMove();

            AgendaItem::QPtr const modif = placeItem;

            QList<AgendaItem::QPtr> oldconflictItems = placeItem->conflictItems();
            QList<AgendaItem::QPtr>::iterator it;
            for (it = oldconflictItems.begin(); it != oldconflictItems.end(); ++it) {
                if (*it) {
                    placeSubCells(*it);
                }
            }
            while (placeItem) {
                placeSubCells(placeItem);
                placeItem = placeItem->nextMultiItem();
            }

            // Notify about change
            // The agenda view will apply the changes to the actual Incidence*!
            // Bug #228696 don't call endChanged now it's async in Akonadi so it can
            // be called before that modified item was done.  And endChange is
            // calling when we move item.
            // Not perfect need to improve it!
            // mChanger->endChange(inc);
            if (item.isValid()) {
                d->mAgendaView->updateEventDates(modif, addIncidence, saveCollection);
            }
            if (addIncidence) {
                // delete the one we dragged, there's a new one being added async, due to dissociation.
                delete modif;
            }
        } else {
            // the item was moved, but not further modified, since it's not recurring
            // make sure the view updates anyhow, with the right item
            if (item.isValid()) {
                d->mAgendaView->updateEventDates(placeItem, addIncidence, saveCollection);
            }
        }
    }

    d->mActionItem = nullptr;
    d->mItemMoved = false;

    if (multiModify) {
        d->mChanger->endAtomicOperation();
    }
}

void Agenda::setActionCursor(int actionType, bool acting)
{
#ifndef QT_NO_CURSOR
    switch (actionType) {
    case MOVE:
        if (acting) {
            setCursor(Qt::SizeAllCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        break;
    case RESIZETOP:
    case RESIZEBOTTOM:
        setCursor(Qt::SizeVerCursor);
        break;
    case RESIZELEFT:
    case RESIZERIGHT:
        setCursor(Qt::SizeHorCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
    }
#endif
}

void Agenda::setNoActionCursor(const AgendaItem::QPtr &moveItem, QPoint pos)
{
    const KCalendarCore::Incidence::Ptr item = moveItem ? moveItem->incidence() : KCalendarCore::Incidence::Ptr();

    const bool noResize = CalendarSupport::hasTodo(item);

    Agenda::MouseActionType resizeType = MOVE;
    if (!noResize) {
        resizeType = isInResizeArea(d->mAllDayMode, pos, moveItem);
    }
    setActionCursor(resizeType);
}

/** calculate the width of the column subcells of the given item
 */
double Agenda::calcSubCellWidth(const AgendaItem::QPtr &item)
{
    QPoint pt;
    QPoint pt1;
    pt = gridToContents(QPoint(item->cellXLeft(), item->cellYTop()));
    pt1 = gridToContents(QPoint(item->cellXLeft(), item->cellYTop()) + QPoint(1, 1));
    pt1 -= pt;
    int const maxSubCells = item->subCells();
    double newSubCellWidth;
    if (d->mAllDayMode) {
        newSubCellWidth = static_cast<double>(pt1.y()) / maxSubCells;
    } else {
        newSubCellWidth = static_cast<double>(pt1.x()) / maxSubCells;
    }
    return newSubCellWidth;
}

void Agenda::adjustItemPosition(const AgendaItem::QPtr &item)
{
    if (!item) {
        return;
    }
    item->resize(int(d->mGridSpacingX * item->cellWidth()), int(d->mGridSpacingY * item->cellHeight()));
    int clXLeft = item->cellXLeft();
    if (QApplication::isRightToLeft()) {
        clXLeft = item->cellXRight() + 1;
    }
    QPoint const cpos = gridToContents(QPoint(clXLeft, item->cellYTop()));
    item->move(cpos.x(), cpos.y());
}

void Agenda::placeAgendaItem(const AgendaItem::QPtr &item, double subCellWidth)
{
    // "left" upper corner, no subcells yet, RTL layouts have right/left
    // switched, widths are negative then
    QPoint const pt = gridToContents(QPoint(item->cellXLeft(), item->cellYTop()));
    // right lower corner
    QPoint const pt1 = gridToContents(QPoint(item->cellXLeft() + item->cellWidth(), item->cellYBottom() + 1));

    double const subCellPos = item->subCell() * subCellWidth;

    // we need to add 0.01 to make sure we don't loose one pixed due to numerics
    // (i.e. if it would be x.9998, we want the integer, not rounded down.
    double delta = 0.01;
    if (subCellWidth < 0) {
        delta = -delta;
    }
    int height;
    int width;
    int xpos;
    int ypos;
    if (d->mAllDayMode) {
        width = pt1.x() - pt.x();
        height = int(subCellPos + subCellWidth + delta) - int(subCellPos);
        xpos = pt.x();
        ypos = pt.y() + int(subCellPos);
    } else {
        width = int(subCellPos + subCellWidth + delta) - int(subCellPos);
        height = pt1.y() - pt.y();
        xpos = pt.x() + int(subCellPos);
        ypos = pt.y();
    }
    if (QApplication::isRightToLeft()) { // RTL language/layout
        xpos += width;
        width = -width;
    }
    if (height < 0) { // BTT (bottom-to-top) layout ?!?
        ypos += height;
        height = -height;
    }
    item->resize(width, height);
    item->move(xpos, ypos);
}

/*
  Place item in cell and take care that multiple items using the same cell do
  not overlap. This method is not yet optimal. It doesn't use the maximum space
  it can get in all cases.
  At the moment the method has a bug: When an item is placed only the sub cell
  widths of the items are changed, which are within the Y region the item to
  place spans. When the sub cell width change of one of this items affects a
  cell, where other items are, which do not overlap in Y with the item to
  place, the display gets corrupted, although the corruption looks quite nice.
*/
void Agenda::placeSubCells(const AgendaItem::QPtr &placeItem)
{
    QList<CalendarSupport::CellItem *> cells;
    for (CalendarSupport::CellItem *item : std::as_const(d->mItems)) {
        if (item) {
            cells.append(item);
        }
    }

    QList<CalendarSupport::CellItem *> items = CalendarSupport::CellItem::placeItem(cells, placeItem);

    placeItem->setConflictItems(QList<AgendaItem::QPtr>());
    double const newSubCellWidth = calcSubCellWidth(placeItem);
    QList<CalendarSupport::CellItem *>::iterator it;
    for (it = items.begin(); it != items.end(); ++it) {
        if (*it) {
            AgendaItem::QPtr const item = static_cast<AgendaItem *>(*it);
            placeAgendaItem(item, newSubCellWidth);
            item->addConflictItem(placeItem);
            placeItem->addConflictItem(item);
        }
    }
    if (items.isEmpty()) {
        placeAgendaItem(placeItem, newSubCellWidth);
    }
    placeItem->update();
}

int Agenda::columnWidth(int column) const
{
    int const start = gridToContents(QPoint(column, 0)).x();
    if (QApplication::isRightToLeft()) {
        column--;
    } else {
        column++;
    }
    int const end = gridToContents(QPoint(column, 0)).x();
    return end - start;
}

void Agenda::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    drawContents(&p, 0, -y(), d->mGridSpacingX * d->mColumns, d->mGridSpacingY * d->mRows + y());
}

/*
  Draw grid in the background of the agenda.
*/
void Agenda::drawContents(QPainter *p, int cx, int cy, int cw, int ch)
{
    QPixmap db(cw, ch);
    db.fill(); // We don't want to see leftovers from previous paints
    QPainter dbp(&db);
    // TODO: CHECK THIS
    //  if (! d->preferences()->agendaGridBackgroundImage().isEmpty()) {
    //    QPixmap bgImage(d->preferences()->agendaGridBackgroundImage());
    //    dbp.drawPixmap(0, 0, cw, ch, bgImage); FIXME
    //  }
    if (!d->preferences()->useSystemColor()) {
        dbp.fillRect(0, 0, cw, ch, d->preferences()->agendaGridBackgroundColor());
    } else {
        dbp.fillRect(0, 0, cw, ch, palette().color(QPalette::Window));
    }

    dbp.translate(-cx, -cy);

    double const lGridSpacingY = d->mGridSpacingY * 2;

    // If work day, use work color
    // If busy day, use busy color
    // if work and busy day, mix both, and busy color has alpha

    const QList<bool> busyDayMask = d->mAgendaView->busyDayMask();

    // Highlight working hours
    if (d->mWorkingHoursEnable && d->mHolidayMask) {
        QColor workColor;
        if (!d->preferences()->useSystemColor()) {
            workColor = d->preferences()->workingHoursColor();
        } else {
            workColor = palette().color(QPalette::Base);
        }

        QPoint const pt1(cx, d->mWorkingHoursYTop);
        QPoint const pt2(cx + cw, d->mWorkingHoursYBottom);
        if (pt2.x() >= pt1.x() /*&& pt2.y() >= pt1.y()*/) {
            int gxStart = contentsToGrid(pt1).x();
            int gxEnd = contentsToGrid(pt2).x();
            // correct start/end for rtl layouts
            if (gxStart > gxEnd) {
                int const tmp = gxStart;
                gxStart = gxEnd;
                gxEnd = tmp;
            }
            int const xoffset = (QApplication::isRightToLeft() ? 1 : 0);
            while (gxStart <= gxEnd) {
                int const xStart = gridToContents(QPoint(gxStart + xoffset, 0)).x();
                int const xWidth = columnWidth(gxStart) + 1;

                if (pt2.y() < pt1.y()) {
                    // overnight working hours
                    if (((gxStart == 0) && !d->mHolidayMask->at(d->mHolidayMask->count() - 1))
                        || ((gxStart > 0) && (gxStart < int(d->mHolidayMask->count())) && (!d->mHolidayMask->at(gxStart - 1)))) {
                        if (pt2.y() > cy) {
                            dbp.fillRect(xStart, cy, xWidth, pt2.y() - cy + 1, workColor);
                        }
                    }
                    if ((gxStart < int(d->mHolidayMask->count() - 1)) && (!d->mHolidayMask->at(gxStart))) {
                        if (pt1.y() < cy + ch - 1) {
                            dbp.fillRect(xStart, pt1.y(), xWidth, cy + ch - pt1.y() + 1, workColor);
                        }
                    }
                } else {
                    // last entry in holiday mask denotes the previous day not visible
                    // (needed for overnight shifts)
                    if (gxStart < int(d->mHolidayMask->count() - 1) && !d->mHolidayMask->at(gxStart)) {
                        dbp.fillRect(xStart, pt1.y(), xWidth, pt2.y() - pt1.y() + 1, workColor);
                    }
                }
                ++gxStart;
            }
        }
    }

    // busy days
    if (d->preferences()->colorAgendaBusyDays() && !d->mAllDayMode) {
        for (int i = 0; i < busyDayMask.count(); ++i) {
            if (busyDayMask[i]) {
                const QPoint pt1(cx + d->mGridSpacingX * i, 0);
                // const QPoint pt2(cx + mGridSpacingX * (i+1), ch);
                QColor busyColor;
                if (!d->preferences()->useSystemColor()) {
                    busyColor = d->preferences()->viewBgBusyColor();
                } else {
                    busyColor = palette().color(QPalette::Window);
                    if ((busyColor.blue() + busyColor.red() + busyColor.green()) > (256 / 2 * 3)) {
                        // dark
                        busyColor = busyColor.lighter(140);
                    } else {
                        // light
                        busyColor = busyColor.darker(140);
                    }
                }
                busyColor.setAlpha(EventViews::BUSY_BACKGROUND_ALPHA);
                dbp.fillRect(pt1.x(), pt1.y(), d->mGridSpacingX, cy + ch, busyColor);
            }
        }
    }

    // draw selection
    if (d->mHasSelection && d->mAgendaView->dateRangeSelectionEnabled()) {
        QPoint pt;
        QPoint pt1;
        QColor highlightColor;
        if (!d->preferences()->useSystemColor()) {
            highlightColor = d->preferences()->agendaGridHighlightColor();
        } else {
            highlightColor = palette().color(QPalette::Highlight);
        }

        if (d->mSelectionEndCell.x() > d->mSelectionStartCell.x()) { // multi day selection
            // draw start day
            pt = gridToContents(d->mSelectionStartCell);
            pt1 = gridToContents(QPoint(d->mSelectionStartCell.x() + 1, d->mRows + 1));
            dbp.fillRect(QRect(pt, pt1), highlightColor);
            // draw all other days between the start day and the day of the selection end
            for (int c = d->mSelectionStartCell.x() + 1; c < d->mSelectionEndCell.x(); ++c) {
                pt = gridToContents(QPoint(c, 0));
                pt1 = gridToContents(QPoint(c + 1, d->mRows + 1));
                dbp.fillRect(QRect(pt, pt1), highlightColor);
            }
            // draw end day
            pt = gridToContents(QPoint(d->mSelectionEndCell.x(), 0));
            pt1 = gridToContents(d->mSelectionEndCell + QPoint(1, 1));
            dbp.fillRect(QRect(pt, pt1), highlightColor);
        } else { // single day selection
            pt = gridToContents(d->mSelectionStartCell);
            pt1 = gridToContents(d->mSelectionEndCell + QPoint(1, 1));
            dbp.fillRect(QRect(pt, pt1), highlightColor);
        }
    }

    // Compute the grid line color for both the hour and half-hour
    // The grid colors are always computed as a function of the palette's windowText color.
    QPen hourPen;
    QPen halfHourPen;

    const QColor windowTextColor = palette().color(QPalette::WindowText);
    if (windowTextColor.red() + windowTextColor.green() + windowTextColor.blue() < (256 / 2 * 3)) {
        // dark grey line
        hourPen = windowTextColor.lighter(200);
        halfHourPen = windowTextColor.lighter(500);
    } else {
        // light grey line
        hourPen = windowTextColor.darker(150);
        halfHourPen = windowTextColor.darker(200);
    }

    dbp.setPen(hourPen);

    // Draw vertical lines of grid, start with the last line not yet visible
    double x = (int(cx / d->mGridSpacingX)) * d->mGridSpacingX;
    while (x < cx + cw) {
        dbp.drawLine(int(x), cy, int(x), cy + ch);
        x += d->mGridSpacingX;
    }

    // Draw horizontal lines of grid
    double y = (int(cy / (2 * lGridSpacingY))) * 2 * lGridSpacingY;
    while (y < cy + ch) {
        dbp.drawLine(cx, int(y), cx + cw, int(y));
        y += 2 * lGridSpacingY;
    }
    y = (2 * int(cy / (2 * lGridSpacingY)) + 1) * lGridSpacingY;
    dbp.setPen(halfHourPen);
    while (y < cy + ch) {
        dbp.drawLine(cx, int(y), cx + cw, int(y));
        y += 2 * lGridSpacingY;
    }
    p->drawPixmap(cx, cy, db);
}

/*
  Convert srcollview contents coordinates to agenda grid coordinates.
*/
QPoint Agenda::contentsToGrid(QPoint pos) const
{
    int const gx = int(QApplication::isRightToLeft() ? d->mColumns - pos.x() / d->mGridSpacingX : pos.x() / d->mGridSpacingX);
    int const gy = int(pos.y() / d->mGridSpacingY);
    return {gx, gy};
}

/*
  Convert agenda grid coordinates to scrollview contents coordinates.
*/
QPoint Agenda::gridToContents(QPoint gpos) const
{
    int const x = int(QApplication::isRightToLeft() ? (d->mColumns - gpos.x()) * d->mGridSpacingX : gpos.x() * d->mGridSpacingX);
    int const y = int(gpos.y() * d->mGridSpacingY);
    return {x, y};
}

/*
  Return Y coordinate corresponding to time. Coordinates are rounded to
  fit into the grid.
*/
int Agenda::timeToY(QTime time) const
{
    int const minutesPerCell = 24 * 60 / d->mRows;
    int const timeMinutes = time.hour() * 60 + time.minute();
    int const Y = (timeMinutes + (minutesPerCell / 2)) / minutesPerCell;

    return Y;
}

/*
  Return time corresponding to cell y coordinate. Coordinates are rounded to
  fit into the grid.
*/
QTime Agenda::gyToTime(int gy) const
{
    int const secondsPerCell = 24 * 60 * 60 / d->mRows;
    int const timeSeconds = secondsPerCell * gy;

    QTime time(0, 0, 0);
    if (timeSeconds < 24 * 60 * 60) {
        time = time.addSecs(timeSeconds);
    } else {
        time.setHMS(23, 59, 59);
    }
    return time;
}

QList<int> Agenda::minContentsY() const
{
    QList<int> minArray;
    minArray.fill(timeToY(QTime(23, 59)), d->mSelectedDates.count());
    for (const AgendaItem::QPtr &item : std::as_const(d->mItems)) {
        if (item) {
            int const ymin = item->cellYTop();
            int const index = item->cellXLeft();
            if (index >= 0 && index < (int)(d->mSelectedDates.count())) {
                if (ymin < minArray[index] && !d->mItemsToDelete.contains(item)) {
                    minArray[index] = ymin;
                }
            }
        }
    }

    return minArray;
}

QList<int> Agenda::maxContentsY() const
{
    QList<int> maxArray;
    maxArray.fill(timeToY(QTime(0, 0)), d->mSelectedDates.count());
    for (const AgendaItem::QPtr &item : std::as_const(d->mItems)) {
        if (item) {
            int const ymax = item->cellYBottom();

            int const index = item->cellXLeft();
            if (index >= 0 && index < (int)(d->mSelectedDates.count())) {
                if (ymax > maxArray[index] && !d->mItemsToDelete.contains(item)) {
                    maxArray[index] = ymax;
                }
            }
        }
    }

    return maxArray;
}

void Agenda::setStartTime(QTime startHour)
{
    const double startPos = (startHour.hour() / 24. + startHour.minute() / 1440. + startHour.second() / 86400.) * d->mRows * gridSpacingY();

    verticalScrollBar()->setValue(startPos);
}

/*
  Insert AgendaItem into agenda.
*/
AgendaItem::QPtr Agenda::insertItem(const KCalendarCore::Incidence::Ptr &incidence,
                                    const QDateTime &recurrenceId,
                                    int X,
                                    int YTop,
                                    int YBottom,
                                    int itemPos,
                                    int itemCount,
                                    bool isSelected)
{
    if (d->mAllDayMode) {
        qCDebug(CALENDARVIEW_LOG) << "using this in all-day mode is illegal.";
        return nullptr;
    }

    d->mActionType = NOP;

    AgendaItem::QPtr agendaItem = createAgendaItem(incidence, itemPos, itemCount, recurrenceId, isSelected);
    if (!agendaItem) {
        return {};
    }

    if (YTop >= d->mRows) {
        YBottom -= YTop - (d->mRows - 1); // Slide the item up into view.
        YTop = d->mRows - 1;
    }
    if (YBottom <= YTop) {
        YBottom = YTop;
    }

    agendaItem->resize(int((X + 1) * d->mGridSpacingX) - int(X * d->mGridSpacingX), int(YTop * d->mGridSpacingY) - int((YBottom + 1) * d->mGridSpacingY));
    agendaItem->setCellXY(X, YTop, YBottom);
    agendaItem->setCellXRight(X);
    agendaItem->setResourceColor(d->mCalendar->resourceColor(incidence));
    agendaItem->installEventFilter(this);

    agendaItem->move(int(X * d->mGridSpacingX), int(YTop * d->mGridSpacingY));

    d->mItems.append(agendaItem);

    placeSubCells(agendaItem);

    agendaItem->show();

    marcus_bains();

    return agendaItem;
}

/*
  Insert all-day AgendaItem into agenda.
*/
AgendaItem::QPtr Agenda::insertAllDayItem(const KCalendarCore::Incidence::Ptr &incidence, const QDateTime &recurrenceId, int XBegin, int XEnd, bool isSelected)
{
    if (!d->mAllDayMode) {
        qCCritical(CALENDARVIEW_LOG) << "using this in non all-day mode is illegal.";
        return nullptr;
    }

    d->mActionType = NOP;

    AgendaItem::QPtr agendaItem = createAgendaItem(incidence, 1, 1, recurrenceId, isSelected);
    if (!agendaItem) {
        return {};
    }

    agendaItem->setCellXY(XBegin, 0, 0);
    agendaItem->setCellXRight(XEnd);

    const double startIt = d->mGridSpacingX * (agendaItem->cellXLeft());
    const double endIt = d->mGridSpacingX * (agendaItem->cellWidth() + agendaItem->cellXLeft());

    agendaItem->resize(int(endIt) - int(startIt), int(d->mGridSpacingY));

    agendaItem->installEventFilter(this);
    agendaItem->setResourceColor(d->mCalendar->resourceColor(incidence));
    agendaItem->move(int(XBegin * d->mGridSpacingX), 0);
    d->mItems.append(agendaItem);

    placeSubCells(agendaItem);

    agendaItem->show();

    return agendaItem;
}

AgendaItem::QPtr
Agenda::createAgendaItem(const KCalendarCore::Incidence::Ptr &incidence, int itemPos, int itemCount, const QDateTime &recurrenceId, bool isSelected)
{
    if (!incidence) {
        qCWarning(CALENDARVIEW_LOG) << "Agenda::createAgendaItem() item is invalid.";
        return {};
    }

    AgendaItem::QPtr agendaItem = new AgendaItem(d->mAgendaView, d->mCalendar, incidence, itemPos, itemCount, recurrenceId, isSelected, this);

    connect(agendaItem.data(), &AgendaItem::removeAgendaItem, this, &Agenda::removeAgendaItem);
    connect(agendaItem.data(), &AgendaItem::showAgendaItem, this, &Agenda::showAgendaItem);

    d->mAgendaItemsById.insert(incidence->uid(), agendaItem);

    return agendaItem;
}

void Agenda::insertMultiItem(const KCalendarCore::Incidence::Ptr &event,
                             const QDateTime &recurrenceId,
                             int XBegin,
                             int XEnd,
                             int YTop,
                             int YBottom,
                             bool isSelected)
{
    KCalendarCore::Event::Ptr const ev = CalendarSupport::event(event);
    Q_ASSERT(ev);
    if (d->mAllDayMode) {
        qCDebug(CALENDARVIEW_LOG) << "using this in all-day mode is illegal.";
        return;
    }

    d->mActionType = NOP;
    int cellX;
    int cellYTop;
    int cellYBottom;
    QString newtext;
    int const width = XEnd - XBegin + 1;
    int count = 0;
    AgendaItem::QPtr current = nullptr;
    QList<AgendaItem::QPtr> multiItems;
    int const visibleCount = d->mSelectedDates.first().daysTo(d->mSelectedDates.last());
    for (cellX = XBegin; cellX <= XEnd; ++cellX) {
        ++count;
        // Only add the items that are visible.
        if (cellX >= 0 && cellX <= visibleCount) {
            if (cellX == XBegin) {
                cellYTop = YTop;
            } else {
                cellYTop = 0;
            }
            if (cellX == XEnd) {
                cellYBottom = YBottom;
            } else {
                cellYBottom = rows() - 1;
            }
            newtext = QStringLiteral("(%1/%2): ").arg(count).arg(width);
            newtext.append(ev->summary());

            current = insertItem(event, recurrenceId, cellX, cellYTop, cellYBottom, width, count, isSelected);
            Q_ASSERT(current);
            current->setText(newtext);
            multiItems.append(current);
        }
    }

    QList<AgendaItem::QPtr>::iterator it = multiItems.begin();
    QList<AgendaItem::QPtr>::iterator const e = multiItems.end();

    if (it != e) { // .first asserts if the list is empty
        const AgendaItem::QPtr &first = multiItems.first();
        const AgendaItem::QPtr &last = multiItems.last();
        AgendaItem::QPtr prev = nullptr;
        AgendaItem::QPtr next = nullptr;

        while (it != e) {
            AgendaItem::QPtr const item = *it;
            ++it;
            next = (it == e) ? nullptr : (*it);
            if (item) {
                item->setMultiItem((item == first) ? nullptr : first, prev, next, (item == last) ? nullptr : last);
            }
            prev = item;
        }
    }

    marcus_bains();
}

void Agenda::removeIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (!incidence) {
        qCWarning(CALENDARVIEW_LOG) << "Agenda::removeIncidence() incidence is invalid" << incidence->uid();
        return;
    }

    if (d->isQueuedForDeletion(incidence->uid())) {
        return; // It's already queued for deletion
    }

    const AgendaItem::List agendaItemList = d->mAgendaItemsById.values(incidence->uid());
    if (agendaItemList.isEmpty()) {
        return;
    }
    for (const AgendaItem::QPtr &agendaItem : agendaItemList) {
        if (agendaItem) {
            if (incidence->instanceIdentifier() != agendaItem->incidence()->instanceIdentifier()) {
                continue;
            }
            if (!removeAgendaItem(agendaItem)) {
                qCWarning(CALENDARVIEW_LOG) << "Agenda::removeIncidence() Failed to remove " << incidence->uid();
            }
        }
    }
}

void Agenda::showAgendaItem(const AgendaItem::QPtr &agendaItem)
{
    if (!agendaItem) {
        qCCritical(CALENDARVIEW_LOG) << "Show what?";
        return;
    }

    agendaItem->hide();

    agendaItem->setParent(this);

    if (!d->mItems.contains(agendaItem)) {
        d->mItems.append(agendaItem);
    }
    placeSubCells(agendaItem);

    agendaItem->show();
}

bool Agenda::removeAgendaItem(const AgendaItem::QPtr &agendaItem)
{
    Q_ASSERT(agendaItem);
    // we found the item. Let's remove it and update the conflicts
    QList<AgendaItem::QPtr> conflictItems = agendaItem->conflictItems();
    // removeChild(thisItem);

    bool const taken = d->mItems.removeAll(agendaItem) > 0;
    d->mAgendaItemsById.remove(agendaItem->incidence()->uid(), agendaItem);

    QList<AgendaItem::QPtr>::iterator it;
    for (it = conflictItems.begin(); it != conflictItems.end(); ++it) {
        if (*it) {
            (*it)->setSubCells((*it)->subCells() - 1);
        }
    }

    for (it = conflictItems.begin(); it != conflictItems.end(); ++it) {
        // the item itself is also in its own conflictItems list!
        if (*it && *it != agendaItem) {
            placeSubCells(*it);
        }
    }
    d->mItemsToDelete.append(agendaItem);
    d->mItemsQueuedForDeletion.insert(agendaItem->incidence()->uid());
    agendaItem->setVisible(false);
    QTimer::singleShot(0, this, &Agenda::deleteItemsToDelete);
    return taken;
}

void Agenda::deleteItemsToDelete()
{
    qDeleteAll(d->mItemsToDelete);
    d->mItemsToDelete.clear();
    d->mItemsQueuedForDeletion.clear();
}

/*QSizePolicy Agenda::sizePolicy() const
{
  // Thought this would make the all-day event agenda minimum size and the
  // normal agenda take the remaining space. But it doesn't work. The QSplitter
  // don't seem to think that an Expanding widget needs more space than a
  // Preferred one.
  // But it doesn't hurt, so it stays.
  if (mAllDayMode) {
    return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  } else {
  return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  }
}*/

/*
  Overridden from QScrollView to provide proper resizing of AgendaItems.
*/
void Agenda::resizeEvent(QResizeEvent *ev)
{
    QSize const newSize(ev->size());

    if (d->mAllDayMode) {
        d->mGridSpacingX = static_cast<double>(newSize.width()) / d->mColumns;
        d->mGridSpacingY = newSize.height();
    } else {
        d->mGridSpacingX = static_cast<double>(newSize.width()) / d->mColumns;
        // make sure that there are not more than 24 per day
        d->mGridSpacingY = static_cast<double>(newSize.height()) / d->mRows;
        if (d->mGridSpacingY < d->mDesiredGridSpacingY) {
            d->mGridSpacingY = d->mDesiredGridSpacingY;
        }
    }
    calculateWorkingHours();

    QTimer::singleShot(0, this, &Agenda::resizeAllContents);
    Q_EMIT gridSpacingYChanged(d->mGridSpacingY * 4);

    QWidget::resizeEvent(ev);
    updateGeometry();
}

void Agenda::resizeAllContents()
{
    double subCellWidth;
    for (const AgendaItem::QPtr &item : std::as_const(d->mItems)) {
        if (item) {
            subCellWidth = calcSubCellWidth(item);
            placeAgendaItem(item, subCellWidth);
        }
    }
    /*
    if (d->mAllDayMode) {
        foreach (const AgendaItem::QPtr &item, d->mItems) {
            if (item) {
                subCellWidth = calcSubCellWidth(item);
                placeAgendaItem(item, subCellWidth);
            }
        }
    } else {
        foreach (const AgendaItem::QPtr &item, d->mItems) {
            if (item) {
                subCellWidth = calcSubCellWidth(item);
                placeAgendaItem(item, subCellWidth);
            }
        }
    }
    */
    checkScrollBoundaries();
    marcus_bains();
    update();
}

void Agenda::scrollUp()
{
    int const currentValue = verticalScrollBar()->value();
    verticalScrollBar()->setValue(currentValue - d->mScrollOffset);
}

void Agenda::scrollDown()
{
    int const currentValue = verticalScrollBar()->value();
    verticalScrollBar()->setValue(currentValue + d->mScrollOffset);
}

QSize Agenda::minimumSize() const
{
    return sizeHint();
}

QSize Agenda::minimumSizeHint() const
{
    return sizeHint();
}

int Agenda::minimumHeight() const
{
    // all day agenda never has scrollbars and the scrollarea will
    // resize it to fit exactly on the viewport.

    if (d->mAllDayMode) {
        return 0;
    } else {
        return d->mGridSpacingY * d->mRows;
    }
}

void Agenda::updateConfig()
{
    const double oldGridSpacingY = d->mGridSpacingY;

    if (!d->mAllDayMode) {
        d->mDesiredGridSpacingY = d->preferences()->hourSize();
        if (d->mDesiredGridSpacingY < 4 || d->mDesiredGridSpacingY > 30) {
            d->mDesiredGridSpacingY = 10;
        }

        /*
        // make sure that there are not more than 24 per day
        d->mGridSpacingY = static_cast<double>(height()) / d->mRows;
        if (d->mGridSpacingY < d->mDesiredGridSpacingY  || true) {
          d->mGridSpacingY = d->mDesiredGridSpacingY;
        }
        */

        // can be two doubles equal?, it's better to compare them with an epsilon
        if (fabs(oldGridSpacingY - d->mDesiredGridSpacingY) > 0.1) {
            d->mGridSpacingY = d->mDesiredGridSpacingY;
            updateGeometry();
        }
    }

    calculateWorkingHours();

    marcus_bains();
}

void Agenda::checkScrollBoundaries()
{
    // Invalidate old values to force update
    d->mOldLowerScrollValue = -1;
    d->mOldUpperScrollValue = -1;

    checkScrollBoundaries(verticalScrollBar()->value());
}

void Agenda::checkScrollBoundaries(int v)
{
    int const yMin = int((v) / d->mGridSpacingY);
    int const yMax = int((v + d->mScrollArea->height()) / d->mGridSpacingY);

    if (yMin != d->mOldLowerScrollValue) {
        d->mOldLowerScrollValue = yMin;
        Q_EMIT lowerYChanged(yMin);
    }
    if (yMax != d->mOldUpperScrollValue) {
        d->mOldUpperScrollValue = yMax;
        Q_EMIT upperYChanged(yMax);
    }
}

int Agenda::visibleContentsYMin() const
{
    int const v = verticalScrollBar()->value();
    return int(v / d->mGridSpacingY);
}

int Agenda::visibleContentsYMax() const
{
    int const v = verticalScrollBar()->value();
    return int((v + d->mScrollArea->height()) / d->mGridSpacingY);
}

void Agenda::deselectItem()
{
    if (d->mSelectedItem.isNull()) {
        return;
    }

    const KCalendarCore::Incidence::Ptr selectedItem = d->mSelectedItem->incidence();

    for (const AgendaItem::QPtr &item : std::as_const(d->mItems)) {
        if (item) {
            const KCalendarCore::Incidence::Ptr itemInc = item->incidence();
            if (itemInc && selectedItem && itemInc->uid() == selectedItem->uid()) {
                item->select(false);
            }
        }
    }

    d->mSelectedItem = nullptr;
}

void Agenda::selectItem(const AgendaItem::QPtr &item)
{
    if ((AgendaItem::QPtr)d->mSelectedItem == item) {
        return;
    }
    deselectItem();
    if (item == nullptr) {
        Q_EMIT incidenceSelected(KCalendarCore::Incidence::Ptr(), QDate());
        return;
    }
    d->mSelectedItem = item;
    d->mSelectedItem->select();
    Q_ASSERT(d->mSelectedItem->incidence());
    d->mSelectedId = d->mSelectedItem->incidence()->uid();

    for (const AgendaItem::QPtr &agendaItem : std::as_const(d->mItems)) {
        if (agendaItem && agendaItem->incidence()->uid() == d->mSelectedId) {
            agendaItem->select();
        }
    }
    Q_EMIT incidenceSelected(d->mSelectedItem->incidence(), d->mSelectedItem->occurrenceDate());
}

void Agenda::selectIncidenceByUid(const QString &uid)
{
    for (const AgendaItem::QPtr &item : std::as_const(d->mItems)) {
        if (item && item->incidence()->uid() == uid) {
            selectItem(item);
            break;
        }
    }
}

void Agenda::selectItem(const Akonadi::Item &item)
{
    selectIncidenceByUid(Akonadi::CalendarUtils::incidence(item)->uid());
}

// This function seems never be called.
void Agenda::keyPressEvent(QKeyEvent *kev)
{
    switch (kev->key()) {
    case Qt::Key_PageDown:
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
        break;
    case Qt::Key_PageUp:
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
        break;
    case Qt::Key_Down:
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        break;
    case Qt::Key_Up:
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        break;
    default:;
    }
}

void Agenda::calculateWorkingHours()
{
    d->mWorkingHoursEnable = !d->mAllDayMode;

    QTime tmp = d->preferences()->workingHoursStart().time();
    d->mWorkingHoursYTop = int(4 * d->mGridSpacingY * (tmp.hour() + tmp.minute() / 60. + tmp.second() / 3600.));
    tmp = d->preferences()->workingHoursEnd().time();
    d->mWorkingHoursYBottom = int(4 * d->mGridSpacingY * (tmp.hour() + tmp.minute() / 60. + tmp.second() / 3600.) - 1);
}

void Agenda::setDateList(const KCalendarCore::DateList &selectedDates)
{
    d->mSelectedDates = selectedDates;
    marcus_bains();
}

KCalendarCore::DateList Agenda::dateList() const
{
    return d->mSelectedDates;
}

void Agenda::setCalendar(const MultiViewCalendar::Ptr &cal)
{
    d->mCalendar = cal;
}

void Agenda::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    d->mChanger = changer;
}

void Agenda::setHolidayMask(QList<bool> *mask)
{
    d->mHolidayMask = mask;
}

void Agenda::contentsMousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

QSize Agenda::sizeHint() const
{
    if (d->mAllDayMode) {
        return QWidget::sizeHint();
    } else {
        return {parentWidget()->width(), static_cast<int>(d->mGridSpacingY * d->mRows)};
    }
}

QScrollBar *Agenda::verticalScrollBar() const
{
    return d->mScrollArea->verticalScrollBar();
}

QScrollArea *Agenda::scrollArea() const
{
    return d->mScrollArea;
}

AgendaItem::List Agenda::agendaItems(const QString &uid) const
{
    return d->mAgendaItemsById.values(uid);
}

AgendaScrollArea::AgendaScrollArea(bool isAllDay, AgendaView *agendaView, bool isInteractive, QWidget *parent)
    : QScrollArea(parent)
{
    if (isAllDay) {
        mAgenda = new Agenda(agendaView, this, 1, isInteractive);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        mAgenda = new Agenda(agendaView, this, 1, 96, agendaView->preferences()->hourSize(), isInteractive);
    }

    setWidgetResizable(true);
    setWidget(mAgenda);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mAgenda->setStartTime(agendaView->preferences()->dayBegins().time());
}

AgendaScrollArea::~AgendaScrollArea() = default;

Agenda *AgendaScrollArea::agenda() const
{
    return mAgenda;
}

#include "moc_agenda.cpp"

/*
  Copyright (c) 2007 Till Adam <adam@kde.org>
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Andras Mantia <andras@kdab.com>

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

#include "timelineview.h"
#include "timelineview_p.h"
#include "timelineitem.h"
#include "helper.h"

#include <KGantt/KGanttGraphicsItem>
#include <KGantt/KGanttGraphicsView>
#include <KGantt/KGanttAbstractRowController>
#include <KGantt/KGanttDateTimeGrid>
#include <KGantt/KGanttItemDelegate>
#include <KGantt/KGanttStyleOptionGanttItem>

#include <Akonadi/Calendar/ETMCalendar>
#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/Utils>
#include <CalendarSupport/KCalPrefs>
#include <Akonadi/Calendar/IncidenceChanger>

#include "calendarview_debug.h"

#include <QApplication>
#include <QPainter>
#include <QStandardItemModel>
#include <QSplitter>
#include <QTreeWidget>
#include <QHeaderView>
#include <QPointer>
#include <QVBoxLayout>
#include <QHelpEvent>
#include <KLocalizedString>

using namespace KCalCore;
using namespace EventViews;

namespace EventViews
{
class RowController : public KGantt::AbstractRowController
{
private:
    static const int ROW_HEIGHT;
    QPointer<QAbstractItemModel> m_model;

public:
    RowController()
    {
        mRowHeight = 20;
    }

    void setModel(QAbstractItemModel *model)
    {
        m_model = model;
    }

    int headerHeight() const override
    {
        return 2 * mRowHeight + 10;
    }

    bool isRowVisible(const QModelIndex &) const override
    {
        return true;
    }

    bool isRowExpanded(const QModelIndex &) const override
    {
        return false;
    }

    KGantt::Span rowGeometry(const QModelIndex &idx) const override
    {
        return KGantt::Span(idx.row() * mRowHeight, mRowHeight);
    }

    int maximumItemHeight() const override
    {
        return mRowHeight / 2;
    }

    int totalHeight() const override
    {
        return m_model->rowCount() * mRowHeight;
    }

    QModelIndex indexAt(int height) const override
    {
        return m_model->index(height / mRowHeight, 0);
    }

    QModelIndex indexBelow(const QModelIndex &idx) const override
    {
        if (!idx.isValid()) {
            return QModelIndex();
        }
        return idx.model()->index(idx.row() + 1, idx.column(), idx.parent());
    }

    QModelIndex indexAbove(const QModelIndex &idx) const override
    {
        if (!idx.isValid()) {
            return QModelIndex();
        }
        return idx.model()->index(idx.row() - 1, idx.column(), idx.parent());
    }

    void setRowHeight(int height)
    {
        mRowHeight = height;
    }

private:
    int mRowHeight;
};

class GanttHeaderView : public QHeaderView
{
public:
    explicit GanttHeaderView(QWidget *parent = nullptr) : QHeaderView(Qt::Horizontal, parent)
    {
    }

    QSize sizeHint() const override
    {
        QSize s = QHeaderView::sizeHint();
        s.rheight() *= 2;
        return s;
    }
};
class GanttItemDelegate : public KGantt::ItemDelegate
{
    void paintGanttItem(QPainter *painter,
                        const KGantt::StyleOptionGanttItem &opt,
                        const QModelIndex &idx) override {
        painter->setRenderHints(QPainter::Antialiasing);
        if (!idx.isValid())
        {
            return;
        }
        KGantt::ItemType type = static_cast<KGantt::ItemType>(
            idx.model()->data(idx, KGantt::ItemTypeRole).toInt());

        QString txt = idx.model()->data(idx, Qt::DisplayRole).toString();
        QRectF itemRect = opt.itemRect;
        QRectF boundingRect = opt.boundingRect;
        boundingRect.setY(itemRect.y());
        boundingRect.setHeight(itemRect.height());

        QBrush brush = defaultBrush(type);
        if (opt.state & QStyle::State_Selected)
        {
            QLinearGradient selectedGrad(0., 0., 0.,
            QApplication::fontMetrics().height());
            selectedGrad.setColorAt(0., Qt::red);
            selectedGrad.setColorAt(1., Qt::darkRed);

            brush = QBrush(selectedGrad);
            painter->setBrush(brush);
        } else {
            painter->setBrush(idx.model()->data(idx, Qt::DecorationRole).value<QColor>());
        }

        painter->setPen(defaultPen(type));
        painter->setBrushOrigin(itemRect.topLeft());

        switch (type)
        {
        case KGantt::TypeTask:
            if (itemRect.isValid()) {
                QRectF r = itemRect;
                painter->drawRect(r);
                bool drawText = true;
                Qt::Alignment ta;
                switch (opt.displayPosition) {
                case KGantt::StyleOptionGanttItem::Left:
                    ta = Qt::AlignLeft;
                    break;
                case KGantt::StyleOptionGanttItem::Right:
                    ta = Qt::AlignRight;
                    break;
                case KGantt::StyleOptionGanttItem::Center:
                    ta = Qt::AlignCenter;
                    break;
                case KGantt::StyleOptionGanttItem::Hidden:
                    drawText = false;
                    break;
                }
                if (drawText) {
                    painter->drawText(boundingRect, ta, txt);
                }
            }
            break;
        default:
            KGantt::ItemDelegate::paintGanttItem(painter, opt, idx);
            break;
        }
    }
};

}

TimelineView::TimelineView(QWidget *parent)
    : EventView(parent), d(new Private(this))
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    d->mLeftView = new QTreeWidget;
    d->mLeftView->setHeader(new GanttHeaderView);
    d->mLeftView->setHeaderLabel(i18n("Calendar"));
    d->mLeftView->setRootIsDecorated(false);
    d->mLeftView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->mLeftView->setUniformRowHeights(true);

    d->mGantt = new KGantt::GraphicsView();
    splitter->addWidget(d->mLeftView);
    splitter->addWidget(d->mGantt);
    connect(splitter, &QSplitter::splitterMoved,
            d, &Private::splitterMoved);
    QStandardItemModel *model = new QStandardItemModel(this);

    d->mRowController = new RowController;

    QStyleOptionViewItem opt;
    opt.initFrom(d->mLeftView);
    const auto h = d->mLeftView->style()->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), d->mLeftView).height();
    d->mRowController->setRowHeight(h);

    d->mRowController->setModel(model);
    d->mGantt->setRowController(d->mRowController);
    KGantt::DateTimeGrid *grid = new KGantt::DateTimeGrid;
    grid->setScale(KGantt::DateTimeGrid::ScaleHour);
    grid->setDayWidth(800);
    grid->setRowSeparators(true);
    d->mGantt->setGrid(grid);
    d->mGantt->setModel(model);
    d->mGantt->viewport()->setFixedWidth(8000);

    d->mGantt->viewport()->installEventFilter(this);

#if 0
    d->mGantt->setCalendarMode(true);
    d->mGantt->setShowLegendButton(false);
    d->mGantt->setFixedHorizon(true);
    d->mGantt->removeColumn(0);
    d->mGantt->addColumn(i18n("Calendar"));
    d->mGantt->setHeaderVisible(true);
    if (KLocale::global()->use12Clock()) {
        d->mGantt->setHourFormat(KDGanttView::Hour_12);
    } else {
        d->mGantt->setHourFormat(KDGanttView::Hour_24_FourDigit);
    }
#else
    qCDebug(CALENDARVIEW_LOG) << "Disabled code, port to KDGantt2";
#endif
    d->mGantt->setItemDelegate(new GanttItemDelegate);

    vbox->addWidget(splitter);

#if 0
    connect(d->mGantt, SIGNAL(rescaling(KDGanttView::Scale)),
            SLOT(overscale(KDGanttView::Scale)));
#else
    qCDebug(CALENDARVIEW_LOG) << "Disabled code, port to KDGantt2";
#endif
    connect(model, &QStandardItemModel::itemChanged,
            d, &Private::itemChanged);

    //TODO FIXME doubleClicked doesn't exist PORTING KDIAGRAM
    //connect(d->mGantt, &KGantt::GraphicsView::doubleClicked,
    //        d, &Private::itemDoubleClicked);
    connect(d->mGantt, &KGantt::GraphicsView::activated,
            d, &Private::itemSelected);
    d->mGantt->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->mGantt, &QWidget::customContextMenuRequested,
            d, &Private::contextMenuRequested);

#if 0
    connect(d->mGantt, SIGNAL(dateTimeDoubleClicked(QDateTime)),
            d, SLOT(newEventWithHint(QDateTime)));
#else
    qCDebug(CALENDARVIEW_LOG) << "Disabled code, port to KDGantt2";
#endif
}

TimelineView::~TimelineView()
{
    delete d->mRowController;
    delete d;
}

Akonadi::Item::List TimelineView::selectedIncidences() const
{
    return d->mSelectedItemList;
}

KCalCore::DateList TimelineView::selectedIncidenceDates() const
{
    return KCalCore::DateList();
}

int TimelineView::currentDateCount() const
{
    return 0;
}

void TimelineView::showDates(const QDate &start, const QDate &end, const QDate &preferredMonth)
{
    Q_UNUSED(preferredMonth);
    Q_ASSERT_X(calendar(), "showDates()", "set a Akonadi::ETMCalendar");
    Q_ASSERT_X(start.isValid(), "showDates()", "start date must be valid");
    Q_ASSERT_X(end.isValid(), "showDates()", "end date must be valid");

    qCDebug(CALENDARVIEW_LOG) << "start=" << start << "end=" << end;

    d->mStartDate = start;
    d->mEndDate = end;
    d->mHintDate = QDateTime();

    KGantt::DateTimeGrid *grid = static_cast<KGantt::DateTimeGrid *>(d->mGantt->grid());
    grid->setStartDateTime(QDateTime(start));
#if 0
    d->mGantt->setHorizonStart(QDateTime(start));
    d->mGantt->setHorizonEnd(QDateTime(end.addDays(1)));
    d->mGantt->setMinorScaleCount(1);
    d->mGantt->setScale(KDGanttView::Hour);
    d->mGantt->setMinimumScale(KDGanttView::Hour);
    d->mGantt->setMaximumScale(KDGanttView::Hour);
    d->mGantt->zoomToFit();

    d->mGantt->setUpdateEnabled(false);
    d->mGantt->clear();
#else
    qCDebug(CALENDARVIEW_LOG) << "Disabled code, port to KDGantt2";
#endif

    d->mLeftView->clear();
    uint index = 0;
    // item for every calendar
    TimelineItem *item = nullptr;
    Akonadi::ETMCalendar::Ptr calres = calendar();
    if (!calres) {
        item = new TimelineItem(calendar(),
                                index++,
                                static_cast<QStandardItemModel *>(d->mGantt->model()),
                                d->mGantt);
        d->mLeftView->addTopLevelItem(new QTreeWidgetItem(QStringList() << i18n("Calendar")));
        d->mCalendarItemMap.insert(-1, item);

    } else {
        const CalendarSupport::CollectionSelection *colSel = collectionSelection();
        const Akonadi::Collection::List collections = colSel->selectedCollections();

        for (const Akonadi::Collection &collection : collections) {
            if (collection.contentMimeTypes().contains(Event::eventMimeType())) {
                item = new TimelineItem(calendar(),
                                        index++,
                                        static_cast<QStandardItemModel *>(d->mGantt->model()),
                                        d->mGantt);
                d->mLeftView->addTopLevelItem(
                    new QTreeWidgetItem(
                        QStringList() << CalendarSupport::displayName(calendar().data(), collection)));
                const QColor resourceColor = EventViews::resourceColor(collection, preferences());
                if (resourceColor.isValid()) {
                    item->setColor(resourceColor);
                }
                qCDebug(CALENDARVIEW_LOG) << "Created item " << item
                                          << " (" <<  CalendarSupport::displayName(calendar().data(), collection) << ") "
                                          << "with index " <<  index - 1 << " from collection " << collection.id();
                d->mCalendarItemMap.insert(collection.id(), item);
            }
        }
    }

    // add incidences

    /**
     * We remove the model from the view here while we fill it with items,
     * because every call to insertIncidence will cause the view to do an expensive
     * updateScene() call otherwise.
     */
    QAbstractItemModel *ganttModel = d->mGantt->model();
    d->mGantt->setModel(nullptr);

    KCalCore::Event::List events;
    KDateTime::Spec timeSpec = CalendarSupport::KCalPrefs::instance()->timeSpec();
    for (QDate day = start; day <= end; day = day.addDays(1)) {
        events = calendar()->events(day, timeSpec,
                                    KCalCore::EventSortStartDate,
                                    KCalCore::SortDirectionAscending);
        Q_FOREACH (const KCalCore::Event::Ptr &event, events) {
            if (event->hasRecurrenceId()) {
                continue;
            }
            Akonadi::Item item = calendar()->item(event);
            d->insertIncidence(item, day);
        }
    }
    d->mGantt->setModel(ganttModel);
    d->splitterMoved();
}

void TimelineView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    Q_UNUSED(incidenceList);
    Q_UNUSED(date);
}

void TimelineView::updateView()
{
    if (d->mStartDate.isValid() && d->mEndDate.isValid()) {
        showDates(d->mStartDate, d->mEndDate);
    }
}

void TimelineView::changeIncidenceDisplay(const Akonadi::Item &incidence, int mode)
{
    switch (mode) {
    case Akonadi::IncidenceChanger::ChangeTypeCreate:
        d->insertIncidence(incidence);
        break;
    case Akonadi::IncidenceChanger::ChangeTypeModify:
        d->removeIncidence(incidence);
        d->insertIncidence(incidence);
        break;
    case Akonadi::IncidenceChanger::ChangeTypeDelete:
        d->removeIncidence(incidence);
        break;
    default:
        updateView();
    }
}

bool TimelineView::eventDurationHint(QDateTime &startDt, QDateTime &endDt,
                                     bool &allDay) const
{
    startDt = QDateTime(d->mHintDate);
    endDt = QDateTime(d->mHintDate.addSecs(2 * 60 * 60));
    allDay = false;
    return d->mHintDate.isValid();
}

// void TimelineView::overscale( KDGanttView::Scale scale )
// {
//   Q_UNUSED( scale );
//   /* Disabled, looks *really* bogus:
//      this triggers and endless rescaling loop; we want to set
//      a fixed scale, the Gantt view doesn't like it and rescales
//      (emitting a rescaling signal that leads here) and so on...
//   //set a relative zoom factor of 1 (?!)
//   d->mGantt->setZoomFactor( 1, false );
//   d->mGantt->setScale( KDGanttView::Hour );
//   d->mGantt->setMinorScaleCount( 12 );
//   */
// }

QDate TimelineView::startDate() const
{
    return d->mStartDate;
}

QDate TimelineView::endDate() const
{
    return d->mEndDate;
}

bool TimelineView::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QGraphicsItem *item = d->mGantt->itemAt(helpEvent->pos());
        if (item) {
            if (item->type() == KGantt::GraphicsItem::Type) {
                KGantt::GraphicsItem *graphicsItem = static_cast<KGantt::GraphicsItem *>(item);
                const QModelIndex itemIndex = graphicsItem->index();

                QStandardItemModel *itemModel =
                    qobject_cast<QStandardItemModel *>(d->mGantt->model());

                TimelineSubItem *timelineItem =
                    dynamic_cast<TimelineSubItem *>(itemModel->item(itemIndex.row(), itemIndex.column()));

                if (timelineItem) {
                    timelineItem->updateToolTip();
                }
            }
        }
    }

    return EventView::eventFilter(object, event);
}


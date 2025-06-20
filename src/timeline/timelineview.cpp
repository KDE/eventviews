/*
  SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "timelineview.h"
#include "helper.h"
#include "timelineitem.h"
#include "timelineview_p.h"

#include <KGanttAbstractRowController>
#include <KGanttDateTimeGrid>
#include <KGanttGraphicsItem>
#include <KGanttGraphicsView>
#include <KGanttItemDelegate>
#include <KGanttStyleOptionGanttItem>

#include <Akonadi/CalendarUtils>
#include <Akonadi/IncidenceChanger>
#include <CalendarSupport/CollectionSelection>

#include "calendarview_debug.h"

#include <KLocalizedString>
#include <QApplication>
#include <QHeaderView>
#include <QHelpEvent>
#include <QPainter>
#include <QPointer>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <chrono>

using namespace KCalendarCore;
using namespace EventViews;

namespace EventViews
{
class RowController : public KGantt::AbstractRowController
{
private:
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
            return {};
        }
        return idx.model()->index(idx.row() + 1, idx.column(), idx.parent());
    }

    QModelIndex indexAbove(const QModelIndex &idx) const override
    {
        if (!idx.isValid()) {
            return {};
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
    explicit GanttHeaderView(QWidget *parent = nullptr)
        : QHeaderView(Qt::Horizontal, parent)
    {
        setSectionResizeMode(QHeaderView::Stretch);
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
public:
    explicit GanttItemDelegate(QObject *parent)
        : KGantt::ItemDelegate(parent)
    {
    }

private:
    void paintGanttItem(QPainter *painter, const KGantt::StyleOptionGanttItem &opt, const QModelIndex &idx) override
    {
        painter->setRenderHints(QPainter::Antialiasing);
        if (!idx.isValid()) {
            return;
        }
        const KGantt::ItemType type = static_cast<KGantt::ItemType>(idx.model()->data(idx, KGantt::ItemTypeRole).toInt());

        const QString txt = idx.model()->data(idx, Qt::DisplayRole).toString();
        const QRectF itemRect = opt.itemRect;
        QRectF boundingRect = opt.boundingRect;
        boundingRect.setY(itemRect.y());
        boundingRect.setHeight(itemRect.height());

        QBrush brush = defaultBrush(type);
        if (opt.state & QStyle::State_Selected) {
            QLinearGradient selectedGrad(0., 0., 0., QFontMetricsF(painter->font()).height());
            selectedGrad.setColorAt(0., Qt::red);
            selectedGrad.setColorAt(1., Qt::darkRed);

            brush = QBrush(selectedGrad);
            painter->setBrush(brush);
        } else {
            painter->setBrush(idx.model()->data(idx, Qt::DecorationRole).value<QColor>());
        }

        painter->setPen(defaultPen(type));
        painter->setBrushOrigin(itemRect.topLeft());

        switch (type) {
        case KGantt::TypeTask:
            if (itemRect.isValid()) {
                const QRectF r = itemRect;
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

TimelineView::TimelineView(const EventViews::PrefsPtr &preferences, QWidget *parent)
    : TimelineView(parent)
{
    setPreferences(preferences);
}

TimelineView::TimelineView(QWidget *parent)
    : EventView(parent)
    , d(new TimelineViewPrivate(this))
{
    auto vbox = new QVBoxLayout(this);
    auto splitter = new QSplitter(Qt::Horizontal, this);
    d->mLeftView = new QTreeWidget;
    d->mLeftView->setColumnCount(1);
    d->mLeftView->setHeader(new GanttHeaderView);
    d->mLeftView->setHeaderLabel(i18n("Calendar"));
    d->mLeftView->setRootIsDecorated(false);
    d->mLeftView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->mLeftView->setUniformRowHeights(true);

    d->mGantt = new KGantt::GraphicsView(this);
    splitter->addWidget(d->mLeftView);
    splitter->addWidget(d->mGantt);
    splitter->setSizes({200, 600});
    auto model = new QStandardItemModel(this);

    d->mRowController = new RowController;

    QStyleOptionViewItem opt;
    opt.initFrom(d->mLeftView);
    const auto h = d->mLeftView->style()->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), d->mLeftView).height();
    d->mRowController->setRowHeight(h);

    d->mRowController->setModel(model);
    d->mGantt->setRowController(d->mRowController);
    auto grid = new KGantt::DateTimeGrid();
    grid->setScale(KGantt::DateTimeGrid::ScaleHour);
    grid->setDayWidth(800);
    grid->setRowSeparators(true);
    d->mGantt->setGrid(grid);
    d->mGantt->setModel(model);
    d->mGantt->viewport()->setFixedWidth(8000);

    d->mGantt->viewport()->installEventFilter(this);
    d->mGantt->setItemDelegate(new GanttItemDelegate(this));

    vbox->addWidget(splitter);

    connect(model, &QStandardItemModel::itemChanged, d.get(), &TimelineViewPrivate::itemChanged);

    connect(d->mGantt, &KGantt::GraphicsView::activated, d.get(), &TimelineViewPrivate::itemSelected);
    d->mGantt->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->mGantt, &QWidget::customContextMenuRequested, d.get(), &TimelineViewPrivate::contextMenuRequested);
}

TimelineView::~TimelineView()
{
    delete d->mRowController;
}

Akonadi::Item::List TimelineView::selectedIncidences() const
{
    return d->mSelectedItemList;
}

KCalendarCore::DateList TimelineView::selectedIncidenceDates() const
{
    return {};
}

int TimelineView::currentDateCount() const
{
    return 0;
}

void TimelineView::showDates(const QDate &start, const QDate &end, const QDate &preferredMonth)
{
    Q_UNUSED(preferredMonth)
    /* cppcheck-suppress assertWithSideEffect */
    Q_ASSERT_X(start.isValid(), "showDates()", "start date must be valid");
    /* cppcheck-suppress assertWithSideEffect */
    Q_ASSERT_X(end.isValid(), "showDates()", "end date must be valid");

    qCDebug(CALENDARVIEW_LOG) << "start=" << start << "end=" << end;

    d->mStartDate = start;
    d->mEndDate = end;
    d->mHintDate = QDateTime();

    auto grid = static_cast<KGantt::DateTimeGrid *>(d->mGantt->grid());
    grid->setStartDateTime(QDateTime(start.startOfDay()));
    d->mLeftView->clear();
    qDeleteAll(d->mCalendarItemMap);
    d->mCalendarItemMap.clear();

    uint index = 0;
    const auto cals = calendars();
    for (const auto &calendar : cals) {
        auto item = new TimelineItem(calendar, index++, static_cast<QStandardItemModel *>(d->mGantt->model()), d->mGantt);
        const auto name = Akonadi::CalendarUtils::displayName(calendar->model(), calendar->collection());
        d->mLeftView->addTopLevelItem(new QTreeWidgetItem(QStringList{name}));
        const QColor resourceColor = EventViews::resourceColor(calendar->collection(), preferences());
        if (resourceColor.isValid()) {
            item->setColor(resourceColor);
        }
        qCDebug(CALENDARVIEW_LOG) << "Created item " << item << " (" << name << ")"
                                  << "with index " << index - 1 << " from collection " << calendar->collection().id();
        d->mCalendarItemMap.insert(calendar->collection().id(), item);
    }

    // add incidences

    /**
     * We remove the model from the view here while we fill it with items,
     * because every call to insertIncidence will cause the view to do an expensive
     * updateScene() call otherwise.
     */
    QAbstractItemModel *ganttModel = d->mGantt->model();
    d->mGantt->setModel(nullptr);

    for (const auto &calendar : cals) {
        for (QDate day = d->mStartDate; day <= d->mEndDate; day = day.addDays(1)) {
            const auto events = calendar->events(day, QTimeZone::systemTimeZone(), KCalendarCore::EventSortStartDate, KCalendarCore::SortDirectionAscending);
            for (const KCalendarCore::Event::Ptr &event : std::as_const(events)) {
                if (event->hasRecurrenceId()) {
                    continue;
                }
                const Akonadi::Item item = calendar->item(event);
                d->insertIncidence(calendar, item, day);
            }
        }
    }
    d->mGantt->setModel(ganttModel);
}

void TimelineView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    Q_UNUSED(incidenceList)
    Q_UNUSED(date)
}

void TimelineView::updateView()
{
    if (d->mStartDate.isValid() && d->mEndDate.isValid()) {
        showDates(d->mStartDate, d->mEndDate);
    }
}

void TimelineView::changeIncidenceDisplay(const Akonadi::Item &item, int mode)
{
    const auto cal = calendar3(item);
    switch (mode) {
    case Akonadi::IncidenceChanger::ChangeTypeCreate:
        d->insertIncidence(cal, item);
        break;
    case Akonadi::IncidenceChanger::ChangeTypeModify:
        d->removeIncidence(item);
        d->insertIncidence(cal, item);
        break;
    case Akonadi::IncidenceChanger::ChangeTypeDelete:
        d->removeIncidence(item);
        break;
    default:
        updateView();
    }
}

bool TimelineView::eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const
{
    bool modified = false;
    if (d->mHintDate.isValid() && !startDt.isValid()) {
        startDt = QDateTime(d->mHintDate);
        modified = true;
    }

    if (modified || !endDt.isValid() || endDt == startDt) {
        endDt = QDateTime(startDt.addDuration(std::chrono::hours(2)));
        modified = true;
    }

    if (allDay) {
        allDay = false;
        modified = true;
    }

    return modified;
}

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
        auto helpEvent = static_cast<QHelpEvent *>(event);
        QGraphicsItem *item = d->mGantt->itemAt(helpEvent->pos());
        if (item) {
            if (item->type() == KGantt::GraphicsItem::Type) {
                auto graphicsItem = static_cast<KGantt::GraphicsItem *>(item);
                const QModelIndex itemIndex = graphicsItem->index();

                auto itemModel = qobject_cast<QStandardItemModel *>(d->mGantt->model());

                auto timelineItem = dynamic_cast<TimelineSubItem *>(itemModel->item(itemIndex.row(), itemIndex.column()));

                if (timelineItem) {
                    timelineItem->updateToolTip();
                }
            }
        }
    }

    return EventView::eventFilter(object, event);
}

#include "moc_timelineview.cpp"

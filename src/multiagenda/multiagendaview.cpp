/*
  SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Sergio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "multiagendaview.h"

#include "agenda/agenda.h"
#include "agenda/agendaview.h"
#include "agenda/timelabelszone.h"
#include "calendarview_debug.h"
#include "configdialoginterface.h"
#include "prefs.h"

#include <Akonadi/CalendarUtils>
#include <Akonadi/ETMViewStateSaver>
#include <Akonadi/EntityTreeModel>

#include <CalendarSupport/CollectionSelection>

#include <KCheckableProxyModel>
#include <KLocalizedString>
#include <KRearrangeColumnsProxyModel>
#include <KViewStateMaintainer>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTimer>

using namespace Akonadi;
using namespace EventViews;

/**
   Function for debugging purposes:
   prints an object's sizeHint()/minimumSizeHint()/policy
   and it's children's too, recursively
*/
/*
static void printObject( QObject *o, int level = 0 )
{
  QMap<int,QString> map;
  map.insert( 0, "Fixed" );
  map.insert( 1, "Minimum" );
  map.insert( 4, "Maximum" );
  map.insert( 5, "Preferred" );
  map.insert( 7, "Expanding" );
  map.insert( 3, "MinimumExpaning" );
  map.insert( 13, "Ignored" );

  QWidget *w = qobject_cast<QWidget*>( o );

  if ( w ) {
    qCDebug(CALENDARVIEW_LOG) << QString( level*2, '-' ) << o
             << w->sizeHint() << "/" << map[w->sizePolicy().verticalPolicy()]
             << "; minimumSize = " << w->minimumSize()
             << "; minimumSizeHint = " << w->minimumSizeHint();
  } else {
    qCDebug(CALENDARVIEW_LOG) << QString( level*2, '-' ) << o ;
  }

  foreach( QObject *child, o->children() ) {
    printObject( child, level + 1 );
  }
}
*/

class DefaultCalendarFactory : public MultiAgendaView::CalendarFactory
{
public:
    using Ptr = QSharedPointer<DefaultCalendarFactory>;

    explicit DefaultCalendarFactory(MultiAgendaView *view)
        : mView(view)
    {
    }

    Akonadi::CollectionCalendar::Ptr calendarForCollection(const Akonadi::Collection &collection) override
    {
        return Akonadi::CollectionCalendar::Ptr::create(mView->entityTreeModel(), collection);
    }

private:
    MultiAgendaView *mView;
};

static QString generateColumnLabel(int c)
{
    return i18n("Agenda %1", c + 1);
}

class EventViews::MultiAgendaViewPrivate
{
private:
    class ElidedLabel : public QFrame
    {
    public:
        ElidedLabel(const QString &text)
            : mText(text)
        {
        }

        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        const QString mText;
    };

public:
    explicit MultiAgendaViewPrivate(const MultiAgendaView::CalendarFactory::Ptr &factory, MultiAgendaView *qq)
        : q(qq)
        , mCalendarFactory(factory)
    {
    }

    ~MultiAgendaViewPrivate()
    {
        qDeleteAll(mSelectionSavers);
    }

    void addView(const Akonadi::CollectionCalendar::Ptr &calendar);
    void addView(KCheckableProxyModel *selectionProxy, const QString &title);
    AgendaView *createView(const QString &calendar);
    void deleteViews();
    void setupViews();
    void resizeScrollView(QSize size);

    MultiAgendaView *const q;
    QList<AgendaView *> mAgendaViews;
    QList<QWidget *> mAgendaWidgets;
    QWidget *mTopBox = nullptr;
    QScrollArea *mScrollArea = nullptr;
    TimeLabelsZone *mTimeLabelsZone = nullptr;
    QSplitter *mLeftSplitter = nullptr;
    QSplitter *mRightSplitter = nullptr;
    QScrollBar *mScrollBar = nullptr;
    QWidget *mLeftBottomSpacer = nullptr;
    QWidget *mRightBottomSpacer = nullptr;
    QDate mStartDate, mEndDate;
    bool mUpdateOnShow = true;
    bool mPendingChanges = true;
    bool mCustomColumnSetupUsed = false;
    QVector<KCheckableProxyModel *> mCollectionSelectionModels;
    QStringList mCustomColumnTitles;
    int mCustomNumberOfColumns = 2;
    QLabel *mLabel = nullptr;
    QWidget *mRightDummyWidget = nullptr;
    QHash<QString, KViewStateMaintainer<ETMViewStateSaver> *> mSelectionSavers;
    QMetaObject::Connection m_selectionChangeConn;
    MultiAgendaView::CalendarFactory::Ptr mCalendarFactory;
};

MultiAgendaView::MultiAgendaView(QWidget *parent)
    : MultiAgendaView(DefaultCalendarFactory::Ptr::create(this), parent)
{
}

MultiAgendaView::MultiAgendaView(const CalendarFactory::Ptr &factory, QWidget *parent)
    : EventView(parent)
    , d(new MultiAgendaViewPrivate(factory, this))
{
    auto topLevelLayout = new QHBoxLayout(this);
    topLevelLayout->setSpacing(0);
    topLevelLayout->setContentsMargins(0, 0, 0, 0);

    QFontMetrics fm(font());
    int topLabelHeight = 2 * fm.height() + fm.lineSpacing();

    auto topSideBox = new QWidget(this);
    auto topSideBoxVBoxLayout = new QVBoxLayout(topSideBox);
    topSideBoxVBoxLayout->setContentsMargins(0, 0, 0, 0);

    auto topSideSpacer = new QWidget(topSideBox);
    topSideBoxVBoxLayout->addWidget(topSideSpacer);
    topSideSpacer->setFixedHeight(topLabelHeight);

    d->mLeftSplitter = new QSplitter(Qt::Vertical, topSideBox);
    topSideBoxVBoxLayout->addWidget(d->mLeftSplitter);

    d->mLabel = new QLabel(i18n("All Day"), d->mLeftSplitter);
    d->mLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->mLabel->setWordWrap(true);

    auto sideBox = new QWidget(d->mLeftSplitter);
    auto sideBoxVBoxLayout = new QVBoxLayout(sideBox);
    sideBoxVBoxLayout->setContentsMargins(0, 0, 0, 0);

    // compensate for the frame the agenda views but not the timelabels have
    auto timeLabelTopAlignmentSpacer = new QWidget(sideBox);
    sideBoxVBoxLayout->addWidget(timeLabelTopAlignmentSpacer);

    d->mTimeLabelsZone = new TimeLabelsZone(sideBox, PrefsPtr(new Prefs()));

    auto timeLabelBotAlignmentSpacer = new QWidget(sideBox);
    sideBoxVBoxLayout->addWidget(timeLabelBotAlignmentSpacer);

    d->mLeftBottomSpacer = new QWidget(topSideBox);
    topSideBoxVBoxLayout->addWidget(d->mLeftBottomSpacer);

    topLevelLayout->addWidget(topSideBox);

    d->mScrollArea = new QScrollArea(this);
    d->mScrollArea->setWidgetResizable(true);

    d->mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // BUG: timelabels aren't aligned with the agenda's grid, 2 or 3 pixels of offset.
    // asymmetric since the timelabels
    timeLabelTopAlignmentSpacer->setFixedHeight(d->mScrollArea->frameWidth() - 1);
    // have 25 horizontal lines
    timeLabelBotAlignmentSpacer->setFixedHeight(d->mScrollArea->frameWidth() - 2);

    d->mScrollArea->setFrameShape(QFrame::NoFrame);
    topLevelLayout->addWidget(d->mScrollArea, 100);
    d->mTopBox = new QWidget(d->mScrollArea->viewport());
    auto mTopBoxHBoxLayout = new QHBoxLayout(d->mTopBox);
    mTopBoxHBoxLayout->setContentsMargins(0, 0, 0, 0);
    d->mScrollArea->setWidget(d->mTopBox);

    topSideBox = new QWidget(this);
    topSideBoxVBoxLayout = new QVBoxLayout(topSideBox);
    topSideBoxVBoxLayout->setContentsMargins(0, 0, 0, 0);

    topSideSpacer = new QWidget(topSideBox);
    topSideBoxVBoxLayout->addWidget(topSideSpacer);
    topSideSpacer->setFixedHeight(topLabelHeight);

    d->mRightSplitter = new QSplitter(Qt::Vertical, topSideBox);
    topSideBoxVBoxLayout->addWidget(d->mRightSplitter);

    connect(d->mLeftSplitter, &QSplitter::splitterMoved, this, &MultiAgendaView::resizeSplitters);
    connect(d->mRightSplitter, &QSplitter::splitterMoved, this, &MultiAgendaView::resizeSplitters);

    d->mRightDummyWidget = new QWidget(d->mRightSplitter);

    d->mScrollBar = new QScrollBar(Qt::Vertical, d->mRightSplitter);

    d->mRightBottomSpacer = new QWidget(topSideBox);
    topSideBoxVBoxLayout->addWidget(d->mRightBottomSpacer);
    topLevelLayout->addWidget(topSideBox);
}

void MultiAgendaView::addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    EventView::addCalendar(calendar);
    d->mPendingChanges = true;
    recreateViews();
}

void MultiAgendaView::removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    EventView::removeCalendar(calendar);
    d->mPendingChanges = true;
    recreateViews();
}

void MultiAgendaView::setModel(QAbstractItemModel *model)
{
    EventView::setModel(model);
    // Workaround: when we create the multiagendaview with custom columns too early
    // during start, when Collections in ETM are not fully loaded yet, then
    // the KCheckableProxyModels are restored from config with incomplete selections.
    // But when the Collections are finally loaded into ETM, there's nothing to update
    // the selections, so we end up with some calendars not displayed in the individual
    // AgendaViews. Thus, we force-recreate everything once collection tree is fetched.
    connect(
        entityTreeModel(),
        &Akonadi::EntityTreeModel::collectionTreeFetched,
        this,
        [this]() {
            d->mPendingChanges = true;
            recreateViews();
        },
        Qt::QueuedConnection);
}

void MultiAgendaView::recreateViews()
{
    if (!d->mPendingChanges) {
        return;
    }
    d->mPendingChanges = false;

    d->deleteViews();

    if (d->mCustomColumnSetupUsed) {
        Q_ASSERT(d->mCollectionSelectionModels.size() == d->mCustomNumberOfColumns);
        for (int i = 0; i < d->mCustomNumberOfColumns; ++i) {
            d->addView(d->mCollectionSelectionModels[i], d->mCustomColumnTitles[i]);
        }
    } else {
        for (const auto &calendar : calendars()) {
            d->addView(calendar);
        }
    }

    // no resources activated, so stop here to avoid crashing somewhere down the line
    // TODO: show a nice message instead
    if (d->mAgendaViews.isEmpty()) {
        return;
    }

    d->setupViews();
    QTimer::singleShot(0, this, &MultiAgendaView::slotResizeScrollView);
    d->mTimeLabelsZone->updateAll();

    QScrollArea *timeLabel = d->mTimeLabelsZone->timeLabels().at(0);
    connect(timeLabel->verticalScrollBar(), &QAbstractSlider::valueChanged, d->mScrollBar, &QAbstractSlider::setValue);
    connect(d->mScrollBar, &QAbstractSlider::valueChanged, timeLabel->verticalScrollBar(), &QAbstractSlider::setValue);

    resizeSplitters();
    QTimer::singleShot(0, this, &MultiAgendaView::setupScrollBar);

    d->mTimeLabelsZone->updateTimeLabelsPosition();
}

void MultiAgendaView::forceRecreateViews()
{
    d->mPendingChanges = true;
    recreateViews();
}

void MultiAgendaViewPrivate::deleteViews()
{
    for (AgendaView *const i : std::as_const(mAgendaViews)) {
        KCheckableProxyModel *proxy = i->takeCustomCollectionSelectionProxyModel();
        if (proxy && !mCollectionSelectionModels.contains(proxy)) {
            delete proxy;
        }
        delete i;
    }

    mAgendaViews.clear();
    mTimeLabelsZone->setAgendaView(nullptr);
    qDeleteAll(mAgendaWidgets);
    mAgendaWidgets.clear();
}

void MultiAgendaViewPrivate::setupViews()
{
    for (AgendaView *agendaView : std::as_const(mAgendaViews)) {
        q->connect(agendaView, qOverload<>(&EventView::newEventSignal), q, qOverload<>(&EventView::newEventSignal));
        q->connect(agendaView, qOverload<const QDate &>(&EventView::newEventSignal), q, qOverload<const QDate &>(&EventView::newEventSignal));
        q->connect(agendaView, qOverload<const QDateTime &>(&EventView::newEventSignal), q, qOverload<const QDateTime &>(&EventView::newEventSignal));
        q->connect(agendaView,
                   qOverload<const QDateTime &, const QDateTime &>(&EventView::newEventSignal),
                   q,
                   qOverload<const QDateTime &>(&EventView::newEventSignal));

        q->connect(agendaView, &EventView::editIncidenceSignal, q, &EventView::editIncidenceSignal);
        q->connect(agendaView, &EventView::showIncidenceSignal, q, &EventView::showIncidenceSignal);
        q->connect(agendaView, &EventView::deleteIncidenceSignal, q, &EventView::deleteIncidenceSignal);

        q->connect(agendaView, &EventView::incidenceSelected, q, &EventView::incidenceSelected);

        q->connect(agendaView, &EventView::cutIncidenceSignal, q, &EventView::cutIncidenceSignal);
        q->connect(agendaView, &EventView::copyIncidenceSignal, q, &EventView::copyIncidenceSignal);
        q->connect(agendaView, &EventView::pasteIncidenceSignal, q, &EventView::pasteIncidenceSignal);
        q->connect(agendaView, &EventView::toggleAlarmSignal, q, &EventView::toggleAlarmSignal);
        q->connect(agendaView, &EventView::dissociateOccurrencesSignal, q, &EventView::dissociateOccurrencesSignal);

        q->connect(agendaView, &EventView::newTodoSignal, q, &EventView::newTodoSignal);

        q->connect(agendaView, &EventView::incidenceSelected, q, &MultiAgendaView::slotSelectionChanged);

        q->connect(agendaView, &AgendaView::timeSpanSelectionChanged, q, &MultiAgendaView::slotClearTimeSpanSelection);

        q->disconnect(agendaView->agenda(), &Agenda::zoomView, agendaView, nullptr);
        q->connect(agendaView->agenda(), &Agenda::zoomView, q, &MultiAgendaView::zoomView);
    }

    AgendaView *lastView = mAgendaViews.last();
    for (AgendaView *agendaView : std::as_const(mAgendaViews)) {
        if (agendaView != lastView) {
            q->connect(agendaView->agenda()->verticalScrollBar(),
                       &QAbstractSlider::valueChanged,
                       lastView->agenda()->verticalScrollBar(),
                       &QAbstractSlider::setValue);
        }
    }

    for (AgendaView *agenda : std::as_const(mAgendaViews)) {
        agenda->readSettings();
    }
}

MultiAgendaView::~MultiAgendaView() = default;

Akonadi::Item::List MultiAgendaView::selectedIncidences() const
{
    Akonadi::Item::List list;
    for (AgendaView *agendaView : std::as_const(d->mAgendaViews)) {
        list += agendaView->selectedIncidences();
    }
    return list;
}

KCalendarCore::DateList MultiAgendaView::selectedIncidenceDates() const
{
    KCalendarCore::DateList list;
    for (AgendaView *agendaView : std::as_const(d->mAgendaViews)) {
        list += agendaView->selectedIncidenceDates();
    }
    return list;
}

int MultiAgendaView::currentDateCount() const
{
    for (AgendaView *agendaView : std::as_const(d->mAgendaViews)) {
        return agendaView->currentDateCount();
    }
    return 0;
}

void MultiAgendaView::showDates(const QDate &start, const QDate &end, const QDate &preferredMonth)
{
    Q_UNUSED(preferredMonth)
    d->mStartDate = start;
    d->mEndDate = end;
    slotResizeScrollView();
    d->mTimeLabelsZone->updateAll();
    for (AgendaView *agendaView : std::as_const(d->mAgendaViews)) {
        agendaView->showDates(start, end);
    }
}

void MultiAgendaView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    for (AgendaView *agendaView : std::as_const(d->mAgendaViews)) {
        agendaView->showIncidences(incidenceList, date);
    }
}

void MultiAgendaView::updateView()
{
    recreateViews();
    for (AgendaView *agendaView : std::as_const(d->mAgendaViews)) {
        agendaView->updateView();
    }
}

int MultiAgendaView::maxDatesHint() const
{
    // these maxDatesHint functions aren't used
    return AgendaView::MAX_DAY_COUNT;
}

void MultiAgendaView::slotSelectionChanged()
{
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        if (agenda != sender()) {
            agenda->clearSelection();
        }
    }
}

bool MultiAgendaView::eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const
{
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        bool valid = agenda->eventDurationHint(startDt, endDt, allDay);
        if (valid) {
            return true;
        }
    }
    return false;
}

void MultiAgendaView::slotClearTimeSpanSelection()
{
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        if (agenda != sender()) {
            agenda->clearTimeSpanSelection();
        }
    }
}

AgendaView *MultiAgendaViewPrivate::createView(const QString &title)
{
    auto box = new QWidget(mTopBox);
    mTopBox->layout()->addWidget(box);
    auto layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new ElidedLabel(title));
    auto av = new AgendaView(q->preferences(), q->startDateTime().date(), q->endDateTime().date(), true, true, q);
    layout->addWidget(av);
    av->setIncidenceChanger(q->changer());
    av->agenda()->scrollArea()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mAgendaViews.append(av);
    mAgendaWidgets.append(box);
    box->show();
    mTimeLabelsZone->setAgendaView(av);

    q->connect(mScrollBar, &QAbstractSlider::valueChanged, av->agenda()->verticalScrollBar(), &QAbstractSlider::setValue);

    q->connect(av->splitter(), &QSplitter::splitterMoved, q, &MultiAgendaView::resizeSplitters);
    q->connect(av, &AgendaView::showIncidencePopupSignal, q, &MultiAgendaView::showIncidencePopupSignal);

    q->connect(av, &AgendaView::showNewEventPopupSignal, q, &MultiAgendaView::showNewEventPopupSignal);

    const QSize minHint = av->allDayAgenda()->scrollArea()->minimumSizeHint();

    if (minHint.isValid()) {
        mLabel->setMinimumHeight(minHint.height());
        mRightDummyWidget->setMinimumHeight(minHint.height());
    }

    return av;
}

void MultiAgendaViewPrivate::addView(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    const auto title = Akonadi::CalendarUtils::displayName(calendar->model(), calendar->collection());
    auto *view = createView(title);
    view->addCalendar(calendar);
}

static void updateViewFromSelection(AgendaView *view,
                                    const QItemSelection &selected,
                                    const QItemSelection &deselected,
                                    const MultiAgendaView::CalendarFactory::Ptr &factory)
{
    for (const auto index : selected.indexes()) {
        if (const auto col = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>(); col.isValid()) {
            const auto calendar = factory->calendarForCollection(col);
            view->addCalendar(calendar);
        }
    }
    for (const auto index : deselected.indexes()) {
        if (const auto col = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>(); col.isValid()) {
            if (const auto calendar = view->calendarForCollection(col); calendar) {
                view->removeCalendar(calendar);
            }
        }
    }
}

void MultiAgendaViewPrivate::addView(KCheckableProxyModel *sm, const QString &title)
{
    auto *view = createView(title);
    // During launch the underlying ETM doesn't have the entire Collection tree populated,
    // so the "sm" contains an icomplete selection - we must listen for changes and upated
    // the view later on
    QObject::connect(sm->selectionModel(),
                     &QItemSelectionModel::selectionChanged,
                     view,
                     [this, view](const QItemSelection &selected, const QItemSelection &deselected) {
                         updateViewFromSelection(view, selected, deselected, mCalendarFactory);
                     });

    // Initial update
    updateViewFromSelection(view, sm->selectionModel()->selection(), QItemSelection{}, mCalendarFactory);
}

void MultiAgendaView::resizeEvent(QResizeEvent *ev)
{
    d->resizeScrollView(ev->size());
    EventView::resizeEvent(ev);
}

void MultiAgendaViewPrivate::resizeScrollView(QSize size)
{
    const int widgetWidth = size.width() - mTimeLabelsZone->width() - mScrollBar->width();

    int height = size.height();
    if (mScrollArea->horizontalScrollBar()->isVisible()) {
        const int sbHeight = mScrollArea->horizontalScrollBar()->height();
        height -= sbHeight;
        mLeftBottomSpacer->setFixedHeight(sbHeight);
        mRightBottomSpacer->setFixedHeight(sbHeight);
    } else {
        mLeftBottomSpacer->setFixedHeight(0);
        mRightBottomSpacer->setFixedHeight(0);
    }

    mTopBox->resize(widgetWidth, height);
}

void MultiAgendaView::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    EventView::setIncidenceChanger(changer);
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        agenda->setIncidenceChanger(changer);
    }
}

void MultiAgendaView::setPreferences(const PrefsPtr &prefs)
{
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        agenda->setPreferences(prefs);
    }
    EventView::setPreferences(prefs);
}

void MultiAgendaView::updateConfig()
{
    EventView::updateConfig();
    d->mTimeLabelsZone->setPreferences(preferences());
    d->mTimeLabelsZone->updateAll();
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        agenda->updateConfig();
    }
}

void MultiAgendaView::resizeSplitters()
{
    if (d->mAgendaViews.isEmpty()) {
        return;
    }

    auto lastMovedSplitter = qobject_cast<QSplitter *>(sender());
    if (!lastMovedSplitter) {
        lastMovedSplitter = d->mLeftSplitter;
    }
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        if (agenda->splitter() == lastMovedSplitter) {
            continue;
        }
        agenda->splitter()->setSizes(lastMovedSplitter->sizes());
    }
    if (lastMovedSplitter != d->mLeftSplitter) {
        d->mLeftSplitter->setSizes(lastMovedSplitter->sizes());
    }
    if (lastMovedSplitter != d->mRightSplitter) {
        d->mRightSplitter->setSizes(lastMovedSplitter->sizes());
    }
}

void MultiAgendaView::zoomView(const int delta, QPoint pos, const Qt::Orientation ori)
{
    const int hourSz = preferences()->hourSize();
    if (ori == Qt::Vertical) {
        if (delta > 0) {
            if (hourSz > 4) {
                preferences()->setHourSize(hourSz - 1);
            }
        } else {
            preferences()->setHourSize(hourSz + 1);
        }
    }

    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        agenda->zoomView(delta, pos, ori);
    }

    d->mTimeLabelsZone->updateAll();
}

void MultiAgendaView::slotResizeScrollView()
{
    d->resizeScrollView(size());
}

void MultiAgendaView::showEvent(QShowEvent *event)
{
    EventView::showEvent(event);
    if (d->mUpdateOnShow) {
        d->mUpdateOnShow = false;
        d->mPendingChanges = true; // force a full view recreation
        showDates(d->mStartDate, d->mEndDate);
    }
}

void MultiAgendaView::setChanges(Changes changes)
{
    EventView::setChanges(changes);
    for (AgendaView *agenda : std::as_const(d->mAgendaViews)) {
        agenda->setChanges(changes);
    }
}

void MultiAgendaView::setupScrollBar()
{
    if (!d->mAgendaViews.isEmpty() && d->mAgendaViews.constFirst()->agenda()) {
        QScrollBar *scrollBar = d->mAgendaViews.constFirst()->agenda()->verticalScrollBar();
        d->mScrollBar->setMinimum(scrollBar->minimum());
        d->mScrollBar->setMaximum(scrollBar->maximum());
        d->mScrollBar->setSingleStep(scrollBar->singleStep());
        d->mScrollBar->setPageStep(scrollBar->pageStep());
        d->mScrollBar->setValue(scrollBar->value());
    }
}

void MultiAgendaView::collectionSelectionChanged()
{
    qCDebug(CALENDARVIEW_LOG);
    d->mPendingChanges = true;
    recreateViews();
}

bool MultiAgendaView::hasConfigurationDialog() const
{
    /** The wrapper in korg has the dialog. Too complicated to move to CalendarViews.
        Depends on korg/AkonadiCollectionView, and will be refactored some day
        to get rid of CollectionSelectionProxyModel/EntityStateSaver */
    return false;
}

void MultiAgendaView::doRestoreConfig(const KConfigGroup &configGroup)
{
    /*
    if (!calendar()) {
        qCCritical(CALENDARVIEW_LOG) << "Calendar is not set.";
        Q_ASSERT(false);
        return;
    }
    */

    d->mCustomColumnSetupUsed = configGroup.readEntry("UseCustomColumnSetup", false);
    d->mCustomNumberOfColumns = configGroup.readEntry("CustomNumberOfColumns", 2);
    d->mCustomColumnTitles = configGroup.readEntry("ColumnTitles", QStringList());
    if (d->mCustomColumnTitles.size() != d->mCustomNumberOfColumns) {
        const int orig = d->mCustomColumnTitles.size();
        d->mCustomColumnTitles.reserve(d->mCustomNumberOfColumns);
        for (int i = orig; i < d->mCustomNumberOfColumns; ++i) {
            d->mCustomColumnTitles.push_back(generateColumnLabel(i));
        }
    }

    QVector<KCheckableProxyModel *> oldModels = d->mCollectionSelectionModels;
    d->mCollectionSelectionModels.clear();

    if (d->mCustomColumnSetupUsed) {
        d->mCollectionSelectionModels.resize(d->mCustomNumberOfColumns);
        for (int i = 0; i < d->mCustomNumberOfColumns; ++i) {
            // Sort the calanders by name
            auto sortProxy = new QSortFilterProxyModel(this);
            sortProxy->setDynamicSortFilter(true);
            sortProxy->setSourceModel(model());

            // Only show the first column
            auto columnFilterProxy = new KRearrangeColumnsProxyModel(this);
            columnFilterProxy->setSourceColumns(QVector<int>() << 0);
            columnFilterProxy->setSourceModel(sortProxy);

            // Keep track of selection.
            auto qsm = new QItemSelectionModel(columnFilterProxy);

            // Make the model checkable.
            auto checkableProxy = new KCheckableProxyModel(this);
            checkableProxy->setSourceModel(columnFilterProxy);
            checkableProxy->setSelectionModel(qsm);
            const QString groupName = configGroup.name() + QLatin1String("_subView_") + QString::number(i);
            const KConfigGroup group = configGroup.config()->group(groupName);

            if (!d->mSelectionSavers.contains(groupName)) {
                d->mSelectionSavers.insert(groupName, new KViewStateMaintainer<ETMViewStateSaver>(group));
                d->mSelectionSavers[groupName]->setSelectionModel(checkableProxy->selectionModel());
            }

            d->mSelectionSavers[groupName]->restoreState();
            d->mCollectionSelectionModels[i] = checkableProxy;
        }
    }

    d->mPendingChanges = true;
    recreateViews();
    qDeleteAll(oldModels);
}

void MultiAgendaView::doSaveConfig(KConfigGroup &configGroup)
{
    configGroup.writeEntry("UseCustomColumnSetup", d->mCustomColumnSetupUsed);
    configGroup.writeEntry("CustomNumberOfColumns", d->mCustomNumberOfColumns);
    configGroup.writeEntry("ColumnTitles", d->mCustomColumnTitles);
    int idx = 0;
    for (KCheckableProxyModel *checkableProxyModel : std::as_const(d->mCollectionSelectionModels)) {
        const QString groupName = configGroup.name() + QLatin1String("_subView_") + QString::number(idx);
        KConfigGroup group = configGroup.config()->group(groupName);
        ++idx;
        // TODO never used ?
        KViewStateMaintainer<ETMViewStateSaver> saver(group);
        if (!d->mSelectionSavers.contains(groupName)) {
            d->mSelectionSavers.insert(groupName, new KViewStateMaintainer<ETMViewStateSaver>(group));
            d->mSelectionSavers[groupName]->setSelectionModel(checkableProxyModel->selectionModel());
        }
        d->mSelectionSavers[groupName]->saveState();
    }
}

void MultiAgendaView::customCollectionsChanged(ConfigDialogInterface *dlg)
{
    if (!d->mCustomColumnSetupUsed && !dlg->useCustomColumns()) {
        // Config didn't change, no need to recreate views
        return;
    }

    d->mCustomColumnSetupUsed = dlg->useCustomColumns();
    d->mCustomNumberOfColumns = dlg->numberOfColumns();
    QVector<KCheckableProxyModel *> newModels;
    newModels.resize(d->mCustomNumberOfColumns);
    d->mCustomColumnTitles.clear();
    d->mCustomColumnTitles.reserve(d->mCustomNumberOfColumns);
    for (int i = 0; i < d->mCustomNumberOfColumns; ++i) {
        newModels[i] = dlg->takeSelectionModel(i);
        d->mCustomColumnTitles.push_back(dlg->columnTitle(i));
    }
    d->mCollectionSelectionModels = newModels;
    d->mPendingChanges = true;
    recreateViews();
}

bool MultiAgendaView::customColumnSetupUsed() const
{
    return d->mCustomColumnSetupUsed;
}

int MultiAgendaView::customNumberOfColumns() const
{
    return d->mCustomNumberOfColumns;
}

QVector<KCheckableProxyModel *> MultiAgendaView::collectionSelectionModels() const
{
    return d->mCollectionSelectionModels;
}

QStringList MultiAgendaView::customColumnTitles() const
{
    return d->mCustomColumnTitles;
}

void MultiAgendaViewPrivate::ElidedLabel::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    QPainter p(this);
    QRect r = contentsRect();
    const QString elidedText = fontMetrics().elidedText(mText, Qt::ElideMiddle, r.width());
    p.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, elidedText);
}

QSize MultiAgendaViewPrivate::ElidedLabel::minimumSizeHint() const
{
    const QFontMetrics &fm = fontMetrics();
    return QSize(fm.boundingRect(QStringLiteral("...")).width(), fm.height());
}

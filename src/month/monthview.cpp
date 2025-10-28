/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Bertjan Broeksema <broeksema@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "monthview.h"
#include "monthgraphicsitems.h"
#include "monthitem.h"
#include "monthscene.h"
#include "prefs.h"

#include <Akonadi/CalendarBase>
#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include "calendarview_debug.h"
#include <KCalendarCore/OccurrenceIterator>
#include <KCheckableProxyModel>
#include <KLocalizedString>
#include <QIcon>

#include <QHBoxLayout>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>

using namespace EventViews;

namespace EventViews
{
class MonthViewPrivate : public KCalendarCore::Calendar::CalendarObserver
{
    MonthView *const q;

public: /// Methods
    explicit MonthViewPrivate(MonthView *qq);

    MonthItem *loadCalendarIncidences(const Akonadi::CollectionCalendar::Ptr &calendar, const QDateTime &startDt, const QDateTime &endDt);

    void addIncidence(const Akonadi::Item &incidence);
    void moveStartDate(int weeks, int months);
    // void setUpModels();
    void triggerDelayedReload(EventView::Change reason);

    /// Members
    QTimer reloadTimer;
    MonthScene *scene = nullptr;
    QDate selectedItemDate;
    Akonadi::Item::Id selectedItemId{-1};
    MonthGraphicsView *view = nullptr;
    QToolButton *fullView = nullptr;

    // List of uids for QDate
    QMap<QDate, QStringList> mBusyDays;

    // If the Month Year header is enabled
    bool enableMonthYearHeader = true;

protected:
    /* reimplemented from KCalendarCore::Calendar::CalendarObserver */
    void calendarIncidenceAdded(const KCalendarCore::Incidence::Ptr &incidence) override;
    void calendarIncidenceChanged(const KCalendarCore::Incidence::Ptr &incidence) override;
    void calendarIncidenceDeleted(const KCalendarCore::Incidence::Ptr &incidence, const KCalendarCore::Calendar *calendar) override;

private:
    // quiet --overloaded-virtual warning
    using KCalendarCore::Calendar::CalendarObserver::calendarIncidenceDeleted;
};
}

MonthViewPrivate::MonthViewPrivate(MonthView *qq)
    : q(qq)
    , scene(new MonthScene(qq))
    , view(new MonthGraphicsView(qq))

{
    reloadTimer.setSingleShot(true);
    view->setScene(scene);
}

MonthItem *MonthViewPrivate::loadCalendarIncidences(const Akonadi::CollectionCalendar::Ptr &calendar, const QDateTime &startDt, const QDateTime &endDt)
{
    MonthItem *itemToReselect = nullptr; // NOLINT(misc-const-correctness)

    const bool colorMonthBusyDays = q->preferences()->colorMonthBusyDays();

    KCalendarCore::OccurrenceIterator occurIter(*calendar, startDt, endDt);
    while (occurIter.hasNext()) {
        occurIter.next();

        // Remove the two checks when filtering is done through a proxyModel, when using calendar search
        if (!q->preferences()->showTodosMonthView() && occurIter.incidence()->type() == KCalendarCore::Incidence::TypeTodo) {
            continue;
        }
        if (!q->preferences()->showJournalsMonthView() && occurIter.incidence()->type() == KCalendarCore::Incidence::TypeJournal) {
            continue;
        }

        const bool busyDay = colorMonthBusyDays && q->makesWholeDayBusy(occurIter.incidence());
        if (busyDay) {
            QStringList &list = mBusyDays[occurIter.occurrenceStartDate().date()];
            list.append(occurIter.incidence()->uid());
        }

        const Akonadi::Item item = calendar->item(occurIter.incidence());
        if (!item.isValid()) {
            continue;
        }
        Q_ASSERT(item.isValid());
        Q_ASSERT(item.hasPayload());
        MonthItem *manager = new IncidenceMonthItem(scene, calendar, item, occurIter.incidence(), occurIter.occurrenceStartDate().toLocalTime().date());
        scene->mManagerList.push_back(manager);
        if (selectedItemId == item.id() && manager->realStartDate() == selectedItemDate) {
            // only select it outside the loop because we are still creating items
            itemToReselect = manager;
        }
    }

    return itemToReselect;
}

void MonthViewPrivate::addIncidence(const Akonadi::Item &incidence)
{
    Q_UNUSED(incidence)
    // TODO: add some more intelligence here...
    q->setChanges(q->changes() | EventView::IncidencesAdded);
    reloadTimer.start(50);
}

void MonthViewPrivate::moveStartDate(int weeks, int months)
{
    auto start = q->startDateTime();
    auto end = q->endDateTime();
    start = start.addDays(weeks * 7);
    end = end.addDays(weeks * 7);
    start = start.addMonths(months);
    end = end.addMonths(months);

    KCalendarCore::DateList dateList;
    QDate d = start.date();
    const QDate e = end.date();
    dateList.reserve(d.daysTo(e) + 1);
    while (d <= e) {
        dateList.append(d);
        d = d.addDays(1);
    }

    /**
     * If we call q->setDateRange( start, end ); directly,
     * it will change the selected dates in month view,
     * but the application won't know about it.
     * The correct way is to Q_EMIT datesSelected()
     * #250256
     * */
    Q_EMIT q->datesSelected(dateList);
}

void MonthViewPrivate::triggerDelayedReload(EventView::Change reason)
{
    q->setChanges(q->changes() | reason);
    if (!reloadTimer.isActive()) {
        reloadTimer.start(50);
    }
}

void MonthViewPrivate::calendarIncidenceAdded(const KCalendarCore::Incidence::Ptr &)
{
    triggerDelayedReload(MonthView::IncidencesAdded);
}

void MonthViewPrivate::calendarIncidenceChanged(const KCalendarCore::Incidence::Ptr &)
{
    triggerDelayedReload(MonthView::IncidencesEdited);
}

void MonthViewPrivate::calendarIncidenceDeleted(const KCalendarCore::Incidence::Ptr &incidence, const KCalendarCore::Calendar *calendar)
{
    Q_UNUSED(calendar)
    Q_ASSERT(!incidence->uid().isEmpty());
    scene->removeIncidence(incidence->uid());
}

/// MonthView

MonthView::MonthView(NavButtonsVisibility visibility, QWidget *parent)
    : EventView(parent)
    , d(new MonthViewPrivate(this))
{
    auto topLayout = new QHBoxLayout(this);
    topLayout->addWidget(d->view);
    topLayout->setContentsMargins({});

    if (visibility == Visible) {
        auto rightLayout = new QVBoxLayout();
        rightLayout->setSpacing(0);
        rightLayout->setContentsMargins({});

        // push buttons to the bottom
        rightLayout->addStretch(1);

        d->fullView = new QToolButton(this);
        d->fullView->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
        d->fullView->setAutoRaise(true);
        d->fullView->setCheckable(true);
        d->fullView->setChecked(preferences()->fullViewMonth());
        d->fullView->isChecked() ? d->fullView->setToolTip(i18nc("@info:tooltip", "Display calendar in a normal size"))
                                 : d->fullView->setToolTip(i18nc("@info:tooltip", "Display calendar in a full window"));
        d->fullView->setWhatsThis(i18nc("@info:whatsthis",
                                        "Click this button and the month view will be enlarged to fill the "
                                        "maximum available window space / or shrunk back to its normal size."));
        connect(d->fullView, &QAbstractButton::clicked, this, &MonthView::changeFullView);

        auto minusMonth = new QToolButton(this);
        minusMonth->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up-double")));
        minusMonth->setAutoRaise(true);
        minusMonth->setToolTip(i18nc("@info:tooltip", "Go back one month"));
        minusMonth->setWhatsThis(i18nc("@info:whatsthis", "Click this button and the view will be scrolled back in time by 1 month."));
        connect(minusMonth, &QAbstractButton::clicked, this, &MonthView::moveBackMonth);

        auto minusWeek = new QToolButton(this);
        minusWeek->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
        minusWeek->setAutoRaise(true);
        minusWeek->setToolTip(i18nc("@info:tooltip", "Go back one week"));
        minusWeek->setWhatsThis(i18nc("@info:whatsthis", "Click this button and the view will be scrolled back in time by 1 week."));
        connect(minusWeek, &QAbstractButton::clicked, this, &MonthView::moveBackWeek);

        auto plusWeek = new QToolButton(this);
        plusWeek->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
        plusWeek->setAutoRaise(true);
        plusWeek->setToolTip(i18nc("@info:tooltip", "Go forward one week"));
        plusWeek->setWhatsThis(i18nc("@info:whatsthis", "Click this button and the view will be scrolled forward in time by 1 week."));
        connect(plusWeek, &QAbstractButton::clicked, this, &MonthView::moveFwdWeek);

        auto plusMonth = new QToolButton(this);
        plusMonth->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down-double")));
        plusMonth->setAutoRaise(true);
        plusMonth->setToolTip(i18nc("@info:tooltip", "Go forward one month"));
        plusMonth->setWhatsThis(i18nc("@info:whatsthis", "Click this button and the view will be scrolled forward in time by 1 month."));
        connect(plusMonth, &QAbstractButton::clicked, this, &MonthView::moveFwdMonth);

        rightLayout->addWidget(d->fullView);
        rightLayout->addWidget(minusMonth);
        rightLayout->addWidget(minusWeek);
        rightLayout->addWidget(plusWeek);
        rightLayout->addWidget(plusMonth);

        topLayout->addLayout(rightLayout);
    } else {
        d->view->setFrameStyle(QFrame::NoFrame);
    }

    connect(d->scene, &MonthScene::showIncidencePopupSignal, this, &MonthView::showIncidencePopupSignal);

    connect(d->scene, &MonthScene::incidenceSelected, this, &EventView::incidenceSelected);

    connect(d->scene, qOverload<>(&MonthScene::newEventSignal), this, qOverload<>(&EventView::newEventSignal));
    connect(d->scene, qOverload<const QDate &>(&MonthScene::newEventSignal), this, qOverload<const QDate &>(&EventView::newEventSignal));

    connect(d->scene, &MonthScene::showNewEventPopupSignal, this, &MonthView::showNewEventPopupSignal);

    connect(&d->reloadTimer, &QTimer::timeout, this, &MonthView::reloadIncidences);
    updateConfig();

    // d->setUpModels();
    d->reloadTimer.start(50);
}

MonthView::~MonthView()
{
    for (auto &calendar : calendars()) {
        calendar->unregisterObserver(d.get());
    }
}

void MonthView::addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    if (calendar && calendar->collection().isValid()) {
        EventView::addCalendar(calendar);
        calendar->registerObserver(d.get());
        setChanges(changes() | ResourcesChanged);
        d->reloadTimer.start(50);
    }
}

void MonthView::removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    if (calendar && calendar->collection().isValid()) {
        EventView::removeCalendar(calendar);
        calendar->unregisterObserver(d.get());
        setChanges(changes() | ResourcesChanged);
        d->reloadTimer.start(50);
    }
}

void MonthView::updateConfig()
{
    d->scene->update();
    setChanges(changes() | ConfigChanged);
    d->reloadTimer.start(50);
}

int MonthView::currentDateCount() const
{
    return actualStartDateTime().date().daysTo(actualEndDateTime().date());
}

KCalendarCore::DateList MonthView::selectedIncidenceDates() const
{
    KCalendarCore::DateList list;
    if (d->scene->selectedItem()) {
        auto tmp = qobject_cast<IncidenceMonthItem *>(d->scene->selectedItem());
        if (tmp) {
            QDate const selectedItemDate = tmp->realStartDate();
            if (selectedItemDate.isValid()) {
                list << selectedItemDate;
            }
        }
    } else if (d->scene->selectedCell()) {
        list << d->scene->selectedCell()->date();
    }

    return list;
}

QDateTime MonthView::selectionStart() const
{
    if (d->scene->selectedCell()) {
        return QDateTime(d->scene->selectedCell()->date().startOfDay());
    } else {
        return {};
    }
}

QDateTime MonthView::selectionEnd() const
{
    // Only one cell can be selected (for now)
    return selectionStart();
}

void MonthView::setDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth)
{
    EventView::setDateRange(start, end, preferredMonth);
    setChanges(changes() | DatesChanged);
    d->reloadTimer.start(50);
}

static QTime nextQuarterHour(const QTime &time)
{
    if (time.second() % 900) {
        return time.addSecs(900 - (time.minute() * 60 + time.second()) % 900);
    }
    return time; // roundup not needed
}

static QTime adjustedDefaultStartTime(const QDate &startDt)
{
    QTime prefTime;
    if (CalendarSupport::KCalPrefs::instance()->startTime().isValid()) {
        prefTime = CalendarSupport::KCalPrefs::instance()->startTime().time();
    }

    const QDateTime currentDateTime = QDateTime::currentDateTime();
    if (startDt == currentDateTime.date()) {
        // If today and the current time is already past the default time
        // use the next quarter hour after the current time.
        // but don't spill over into tomorrow.
        const QTime currentTime = currentDateTime.time();
        if (!prefTime.isValid() || (currentTime > prefTime && currentTime < QTime(23, 45))) {
            prefTime = nextQuarterHour(currentTime);
        }
    }
    return prefTime;
}

bool MonthView::eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const
{
    Q_UNUSED(allDay);
    auto defaultDuration = CalendarSupport::KCalPrefs::instance()->defaultDuration().time();
    auto duration = (defaultDuration.hour() * 3600) + (defaultDuration.minute() * 60);
    if (startDt.isValid()) {
        if (endDt.isValid()) {
            if (startDt >= endDt) {
                endDt.setTime(startDt.time().addSecs(duration));
            }
        } else {
            endDt.setDate(startDt.date());
            endDt.setTime(startDt.time().addSecs(duration));
        }
        return true;
    }

    if (d->scene->selectedCell()) {
        startDt.setDate(d->scene->selectedCell()->date());
        startDt.setTime(adjustedDefaultStartTime(startDt.date()));

        endDt.setDate(d->scene->selectedCell()->date());
        endDt.setTime(startDt.time().addSecs(duration));
        return true;
    }

    return false;
}

void MonthView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    Q_UNUSED(incidenceList)
    Q_UNUSED(date)
}

void MonthView::changeIncidenceDisplay(const Akonadi::Item &incidence, int action)
{
    Q_UNUSED(incidence)
    Q_UNUSED(action)

    // TODO: add some more intelligence here...

    // don't call reloadIncidences() directly. It would delete all
    // MonthItems, but this changeIncidenceDisplay()-method was probably
    // called by one of the MonthItem objects. So only schedule a reload
    // as event
    setChanges(changes() | IncidencesEdited);
    d->reloadTimer.start(50);
}

void MonthView::updateView()
{
    d->view->update();
}

#ifndef QT_NO_WHEELEVENT
void MonthView::wheelEvent(QWheelEvent *event)
{
    // invert direction to get scroll-like behaviour
    if (event->angleDelta().y() > 0) {
        d->moveStartDate(-1, 0);
    } else if (event->angleDelta().y() < 0) {
        d->moveStartDate(1, 0);
    }

    // call accept in every case, we do not want anybody else to react
    event->accept();
}

#endif

void MonthView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_PageUp) {
        d->moveStartDate(0, -1);
        event->accept();
    } else if (event->key() == Qt::Key_PageDown) {
        d->moveStartDate(0, 1);
        event->accept();
    } else if (processKeyEvent(event)) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MonthView::keyReleaseEvent(QKeyEvent *event)
{
    if (processKeyEvent(event)) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MonthView::changeFullView()
{
    bool const fullView = d->fullView->isChecked();

    if (fullView) {
        d->fullView->setIcon(QIcon::fromTheme(QStringLiteral("view-restore")));
        d->fullView->setToolTip(i18nc("@info:tooltip", "Display calendar in a normal size"));
    } else {
        d->fullView->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
        d->fullView->setToolTip(i18nc("@info:tooltip", "Display calendar in a full window"));
    }
    preferences()->setFullViewMonth(fullView);
    preferences()->writeConfig();

    Q_EMIT fullViewChanged(fullView);
}

void MonthView::moveBackMonth()
{
    d->moveStartDate(0, -1);
}

void MonthView::moveBackWeek()
{
    d->moveStartDate(-1, 0);
}

void MonthView::moveFwdWeek()
{
    d->moveStartDate(1, 0);
}

void MonthView::moveFwdMonth()
{
    d->moveStartDate(0, 1);
}

void MonthView::showDates(const QDate &start, const QDate &end, const QDate &preferedMonth)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(preferedMonth)
    d->triggerDelayedReload(DatesChanged);
}

QPair<QDateTime, QDateTime> MonthView::actualDateRange(const QDateTime &start, const QDateTime &, const QDate &preferredMonth) const
{
    QDateTime dayOne = preferredMonth.isValid() ? QDateTime(preferredMonth.startOfDay()) : start;

    dayOne.setDate(QDate(dayOne.date().year(), dayOne.date().month(), 1));
    const int weekdayCol = (dayOne.date().dayOfWeek() + 7 - preferences()->firstDayOfWeek()) % 7;
    QDateTime actualStart = dayOne.addDays(-weekdayCol);
    actualStart.setTime(QTime(0, 0, 0, 0));
    QDateTime actualEnd = actualStart.addDays(6 * 7 - 1);
    actualEnd.setTime(QTime(23, 59, 59, 99));
    return qMakePair(actualStart, actualEnd);
}

Akonadi::Item::List MonthView::selectedIncidences() const
{
    Akonadi::Item::List selected;
    if (d->scene->selectedItem()) {
        auto tmp = qobject_cast<IncidenceMonthItem *>(d->scene->selectedItem());
        if (tmp) {
            Akonadi::Item const incidenceSelected = tmp->akonadiItem();
            if (incidenceSelected.isValid()) {
                selected.append(incidenceSelected);
            }
        }
    }
    return selected;
}

KHolidays::Holiday::List MonthView::holidays(QDate startDate, QDate endDate)
{
    KHolidays::Holiday::List holidays;
    auto const regions = CalendarSupport::KCalPrefs::instance()->mHolidays;
    for (auto const &r : regions) {
        KHolidays::HolidayRegion const region(r);
        if (region.isValid()) {
            holidays += region.rawHolidaysWithAstroSeasons(startDate, endDate);
        }
    }
    return holidays;
}

void MonthView::reloadIncidences()
{
    if (changes() == NothingChanged) {
        return;
    }
    // keep selection if it exists
    Akonadi::Item const incidenceSelected;

    MonthItem *itemToReselect = nullptr;

    if (auto tmp = qobject_cast<IncidenceMonthItem *>(d->scene->selectedItem())) {
        d->selectedItemId = tmp->akonadiItem().id();
        d->selectedItemDate = tmp->realStartDate();
        if (!d->selectedItemDate.isValid()) {
            return;
        }
    }

    d->scene->resetAll();
    d->mBusyDays.clear();
    // build monthcells hash
    int i = 0;
    for (QDate date = actualStartDateTime().date(); date <= actualEndDateTime().date(); date = date.addDays(1)) {
        d->scene->mMonthCellMap[date] = new MonthCell(i, date, d->scene);
        i++;
    }

    // build global event list
    const auto cals = calendars();
    for (const auto &calendar : cals) {
        auto *newItemToReselect = d->loadCalendarIncidences(calendar, actualStartDateTime(), actualEndDateTime());
        if (itemToReselect == nullptr) {
            itemToReselect = newItemToReselect;
        }
    }

    if (itemToReselect) {
        d->scene->selectItem(itemToReselect);
    }

    // add holidays
    auto const hols = holidays(actualStartDateTime().date(), actualEndDateTime().date());
    for (auto const &h : hols) {
        if (h.dayType() == KHolidays::Holiday::NonWorkday) {
            /* cppcheck-suppress constVariablePointer */
            MonthItem *holidayItem = new HolidayMonthItem(d->scene, h.observedStartDate(), h.observedEndDate(), h.name());
            d->scene->mManagerList << holidayItem;
        }
    }

    // sort it
    std::sort(d->scene->mManagerList.begin(), d->scene->mManagerList.end(), MonthItem::greaterThan);

    // build each month's cell event list
    /* cppcheck-suppress constVariablePointer */
    for (MonthItem *manager : std::as_const(d->scene->mManagerList)) {
        for (QDate date = manager->startDate(); date <= manager->endDate(); date = date.addDays(1)) {
            MonthCell *cell = d->scene->mMonthCellMap.value(date);
            if (cell) {
                cell->mMonthItemList << manager;
            }
        }
    }

    for (MonthItem *manager : std::as_const(d->scene->mManagerList)) {
        manager->updateMonthGraphicsItems();
        manager->updatePosition();
    }

    for (MonthItem *manager : std::as_const(d->scene->mManagerList)) {
        manager->updateGeometry();
    }

    d->scene->setInitialized(true);
    d->view->update();
    d->scene->update();
}

void MonthView::calendarReset()
{
    qCDebug(CALENDARVIEW_LOG);
    d->triggerDelayedReload(ResourcesChanged);
}

QDate MonthView::averageDate() const
{
    return actualStartDateTime().date().addDays(actualStartDateTime().date().daysTo(actualEndDateTime().date()) / 2);
}

int MonthView::currentMonth() const
{
    return averageDate().month();
}

bool MonthView::usesFullWindow()
{
    return preferences()->fullViewMonth();
}

void MonthView::enableMonthYearHeader(bool enable)
{
    d->enableMonthYearHeader = enable;
}

bool MonthView::hasEnabledMonthYearHeader()
{
    return d->enableMonthYearHeader;
}

void MonthView::showFullWindowButton(bool show)
{
    d->fullView->setVisible(show);
    preferences()->setFullViewMonth(show);
    preferences()->writeConfig();
}

bool MonthView::isBusyDay(QDate day) const
{
    return !d->mBusyDays[day].isEmpty();
}

#include "moc_monthview.cpp"

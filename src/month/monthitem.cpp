/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "monthitem.h"
#include "helper.h"
#include "monthgraphicsitems.h"
#include "monthscene.h"
#include "monthview.h"
#include "prefs.h"
#include "prefs_base.h" // Ugly, but needed for the Enums

#include <Akonadi/CalendarUtils>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/IncidenceChanger>
#include <Akonadi/TagCache>
#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include <KCalUtils/IncidenceFormatter>
#include <KCalUtils/RecurrenceActions>

#include "calendarview_debug.h"
#include <KMessageBox>

#include <QDrag>

using namespace EventViews;
using namespace KCalendarCore;

MonthItem::MonthItem(MonthScene *monthScene)
    : mMonthScene(monthScene)
{
}

MonthItem::~MonthItem()
{
    deleteAll();
}

void MonthItem::deleteAll()
{
    qDeleteAll(mMonthGraphicsItemList);
    mMonthGraphicsItemList.clear();
}

QWidget *MonthItem::parentWidget() const
{
    return mMonthScene ? mMonthScene->monthView() : nullptr;
}

void MonthItem::updateMonthGraphicsItems()
{
    // Remove all items
    qDeleteAll(mMonthGraphicsItemList);
    mMonthGraphicsItemList.clear();

    const QDate monthStartDate = startDate();
    const QDate monthEndDate = endDate();

    // For each row of the month view, create an item to build the whole
    // MonthItem's MonthGraphicsItems.
    for (QDate d = mMonthScene->mMonthView->actualStartDateTime().date(); d < mMonthScene->mMonthView->actualEndDateTime().date(); d = d.addDays(7)) {
        QDate const end = d.addDays(6);

        int span;
        QDate start;
        if (monthStartDate <= d && monthEndDate >= end) { // MonthItem takes the whole line
            span = 6;
            start = d;
        } else if (monthStartDate >= d && monthEndDate <= end) { // starts and ends on this line
            start = monthStartDate;
            span = daySpan();
        } else if (d <= monthEndDate && monthEndDate <= end) { // MonthItem ends on this line
            span = mMonthScene->getLeftSpan(monthEndDate);
            start = d;
        } else if (d <= monthStartDate && monthStartDate <= end) { // MonthItem begins on this line
            span = mMonthScene->getRightSpan(monthStartDate);
            start = monthStartDate;
        } else { // MonthItem is not on the line
            continue;
        }

        // A new item needs to be created
        auto newItem = new MonthGraphicsItem(this);
        mMonthGraphicsItemList << newItem;
        newItem->setStartDate(start);
        newItem->setDaySpan(span);
    }

    if (isMoving() || isResizing()) {
        setZValue(100);
    } else {
        setZValue(0);
    }
}

void MonthItem::beginResize()
{
    mOverrideDaySpan = daySpan();
    mOverrideStartDate = startDate();
    mResizing = true;
    setZValue(100);
}

void MonthItem::endResize()
{
    setZValue(0);
    mResizing = false; // startDate() and daySpan() return real values again

    if (mOverrideStartDate != startDate() || mOverrideDaySpan != daySpan()) {
        finalizeResize(mOverrideStartDate, mOverrideStartDate.addDays(mOverrideDaySpan));
    }
}

void MonthItem::beginMove()
{
    mOverrideDaySpan = daySpan();
    mOverrideStartDate = startDate();
    mMoving = true;
    setZValue(100);
}

void MonthItem::endMove()
{
    setZValue(0);
    mMoving = false; // startDate() and daySpan() return real values again

    if (mOverrideStartDate != startDate()) {
        finalizeMove(mOverrideStartDate);
    }
}

bool MonthItem::resizeBy(int offsetToPreviousDate)
{
    bool ret = false;
    if (mMonthScene->resizeType() == MonthScene::ResizeLeft) {
        if (mOverrideDaySpan - offsetToPreviousDate >= 0) {
            mOverrideStartDate = mOverrideStartDate.addDays(offsetToPreviousDate);
            mOverrideDaySpan = mOverrideDaySpan - offsetToPreviousDate;
            ret = true;
        }
    } else if (mMonthScene->resizeType() == MonthScene::ResizeRight) {
        if (mOverrideDaySpan + offsetToPreviousDate >= 0) {
            mOverrideDaySpan = mOverrideDaySpan + offsetToPreviousDate;
            ret = true;
        }
    }

    if (ret) {
        updateMonthGraphicsItems();
    }
    return ret;
}

void MonthItem::moveBy(int offsetToPreviousDate)
{
    mOverrideStartDate = mOverrideStartDate.addDays(offsetToPreviousDate);
    updateMonthGraphicsItems();
}

void MonthItem::moveTo(QDate date)
{
    mOverrideStartDate = date;
    updateMonthGraphicsItems();
}

void MonthItem::updateGeometry()
{
    for (MonthGraphicsItem *item : std::as_const(mMonthGraphicsItemList)) {
        item->updateGeometry();
    }
}

void MonthItem::setZValue(qreal z)
{
    for (MonthGraphicsItem *item : std::as_const(mMonthGraphicsItemList)) {
        item->setZValue(z);
    }
}

QDate MonthItem::startDate() const
{
    if ((isMoving() || isResizing()) && mOverrideStartDate.isValid()) {
        return mOverrideStartDate;
    }

    return realStartDate();
}

QDate MonthItem::endDate() const
{
    if ((isMoving() || isResizing()) && mOverrideStartDate.isValid()) {
        return mOverrideStartDate.addDays(mOverrideDaySpan);
    }

    return realEndDate();
}

int MonthItem::daySpan() const
{
    if (isMoving() || isResizing()) {
        return mOverrideDaySpan;
    }
    QDateTime const start(startDate().startOfDay());
    QDateTime const end(endDate().startOfDay());

    if (start.isValid() && end.isValid()) {
        return start.daysTo(end);
    }

    return 0;
}

bool MonthItem::greaterThan(const MonthItem *e1, const MonthItem *e2)
{
    const int leftDaySpan = e1->daySpan();
    const int rightDaySpan = e2->daySpan();
    if (leftDaySpan == rightDaySpan) {
        const QDate leftStartDate = e1->startDate();
        const QDate rightStartDate = e2->startDate();
        if (!leftStartDate.isValid() || !rightStartDate.isValid()) {
            return false;
        }
        if (leftStartDate == rightStartDate) {
            if (e1->allDay() && !e2->allDay()) {
                return true;
            }
            if (!e1->allDay() && e2->allDay()) {
                return false;
            }
            return e1->greaterThanFallback(e2);
        } else {
            return leftStartDate > rightStartDate;
        }
    }
    return leftDaySpan > rightDaySpan;
}

bool MonthItem::greaterThanFallback(const MonthItem *other) const
{
    const auto h = qobject_cast<const HolidayMonthItem *>(other);

    // If "other" is a holiday, display it first.
    return !h;
}

void MonthItem::updatePosition()
{
    if (!startDate().isValid() || !endDate().isValid()) {
        return;
    }

    int firstFreeSpace = 0;
    for (QDate d = startDate(); d <= endDate(); d = d.addDays(1)) {
        MonthCell *cell = mMonthScene->mMonthCellMap.value(d);
        if (!cell) {
            continue; // cell can be null if the item begins outside the month
        }
        int const firstFreeSpaceTmp = cell->firstFreeSpace();
        if (firstFreeSpaceTmp > firstFreeSpace) {
            firstFreeSpace = firstFreeSpaceTmp;
        }
    }

    for (QDate d = startDate(); d <= endDate(); d = d.addDays(1)) {
        MonthCell *cell = mMonthScene->mMonthCellMap.value(d);
        if (!cell) {
            continue;
        }
        cell->addMonthItem(this, firstFreeSpace);
    }

    mPosition = firstFreeSpace;
}

QList<MonthGraphicsItem *> EventViews::MonthItem::monthGraphicsItems() const
{
    return mMonthGraphicsItemList;
}

//-----------------------------------------------------------------
// INCIDENCEMONTHITEM
IncidenceMonthItem::IncidenceMonthItem(MonthScene *monthScene,
                                       const Akonadi::CollectionCalendar::Ptr &calendar,
                                       const Akonadi::Item &aitem,
                                       const KCalendarCore::Incidence::Ptr &incidence,
                                       QDate recurStartDate)
    : MonthItem(monthScene)
    , mCalendar(calendar)
    , mIncidence(incidence)
    , mAkonadiItemId(aitem.id())
{
    mIsEvent = CalendarSupport::hasEvent(aitem);
    mIsJournal = CalendarSupport::hasJournal(aitem);
    mIsTodo = CalendarSupport::hasTodo(aitem);

    KCalendarCore::Incidence::Ptr inc = mIncidence;
    if (inc->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES") || inc->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES")) {
        const int years = EventViews::yearDiff(inc->dtStart().date(), recurStartDate);
        if (years > 0) {
            inc = KCalendarCore::Incidence::Ptr(inc->clone());
            inc->setReadOnly(false);
            if (inc->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES")) {
                inc->setDescription(i18ncp("@info/plain a person's age", "1 year old", "%1 years old", years));
            } else { // anniversary
                inc->setDescription(i18ncp("@info/plain number of years of marriage", "1 year", "%1 years", years));
            }
            inc->setReadOnly(true);
            mIncidence = inc;
        }
    }

    connect(monthScene, &MonthScene::incidenceSelected, this, &IncidenceMonthItem::updateSelection);

    // first set to 0, because it's used in startDate()
    mRecurDayOffset = 0;
    const auto incidenceStart = mIncidence->dtStart().toLocalTime().date();
    if ((mIncidence->recurs() || mIncidence->recurrenceId().isValid()) && incidenceStart.isValid() && recurStartDate.isValid()) {
        mRecurDayOffset = incidenceStart.daysTo(recurStartDate);
    }
}

IncidenceMonthItem::~IncidenceMonthItem() = default;

bool IncidenceMonthItem::greaterThanFallback(const MonthItem *other) const
{
    const auto o = qobject_cast<const IncidenceMonthItem *>(other);
    if (!o) {
        return MonthItem::greaterThanFallback(other);
    }

    if (allDay() != o->allDay()) {
        return allDay();
    }
    const KCalendarCore::Incidence::Ptr otherIncidence = o->mIncidence;

    if (mIncidence->dtStart().time() != otherIncidence->dtStart().time()) {
        return mIncidence->dtStart().time() < otherIncidence->dtStart().time();
    }

    // as a last resort, compare uids
    return mIncidence->uid() < otherIncidence->uid();
}

QDate IncidenceMonthItem::realStartDate() const
{
    if (!mIncidence) {
        return {};
    }

    const QDateTime dt = mIncidence->dateTime(Incidence::RoleDisplayStart);
    const QDate start = dt.toLocalTime().date();

    return start.addDays(mRecurDayOffset);
}

QDate IncidenceMonthItem::realEndDate() const
{
    if (!mIncidence) {
        return {};
    }

    QDateTime dt = mIncidence->dateTime(KCalendarCore::Incidence::RoleDisplayEnd);
    if (!mIncidence->allDay() && dt > mIncidence->dateTime(KCalendarCore::Incidence::RoleDisplayStart)) {
        // If dt's time portion is 00:00:00, the incidence ends on the previous day
        // unless it also starts at 00:00:00 (a duration of 0).
        dt = dt.addMSecs(-1);
    }
    const QDate end = dt.toLocalTime().date();

    return end.addDays(mRecurDayOffset);
}

bool IncidenceMonthItem::allDay() const
{
    return mIncidence->allDay();
}

bool IncidenceMonthItem::isMoveable() const
{
    return mCalendar->hasRight(Akonadi::Collection::CanChangeItem);
}

bool IncidenceMonthItem::isResizable() const
{
    return mCalendar->hasRight(Akonadi::Collection::CanChangeItem);
}

void IncidenceMonthItem::finalizeMove(const QDate &newStartDate)
{
    Q_ASSERT(isMoveable());

    if (startDate().isValid()) {
        if (newStartDate.isValid()) {
            updateDates(startDate().daysTo(newStartDate), startDate().daysTo(newStartDate));
        } else {
            if (QDrag *drag = CalendarSupport::createDrag(akonadiItem(), this)) {
                drag->exec();
            }
        }
    }
}

void IncidenceMonthItem::finalizeResize(const QDate &newStartDate, const QDate &newEndDate)
{
    Q_ASSERT(isResizable());

    if (startDate().isValid() && endDate().isValid() && newStartDate.isValid() && newEndDate.isValid()) {
        updateDates(startDate().daysTo(newStartDate), endDate().daysTo(newEndDate));
    }
}

void IncidenceMonthItem::updateDates(int startOffset, int endOffset)
{
    Akonadi::IncidenceChanger *changer = monthScene()->incidenceChanger();
    if (!changer || (startOffset == 0 && endOffset == 0)) {
        qCDebug(CALENDARVIEW_LOG) << changer << startOffset << endOffset;
        return;
    }

    Akonadi::Item item = akonadiItem();
    item.setPayload(mIncidence);
    if (mIncidence->recurs()) {
        const int res = monthScene()->mMonthView->showMoveRecurDialog(mIncidence, startDate());
        switch (res) {
        case KCalUtils::RecurrenceActions::AllOccurrences: {
            // All occurrences
            KCalendarCore::Incidence::Ptr const oldIncidence(mIncidence->clone());
            setNewDates(mIncidence, startOffset, endOffset);
            changer->modifyIncidence(item, oldIncidence);
            break;
        }
        case KCalUtils::RecurrenceActions::SelectedOccurrence: // Just this occurrence
        case KCalUtils::RecurrenceActions::FutureOccurrences: { // All future occurrences
            const bool thisAndFuture = (res == KCalUtils::RecurrenceActions::FutureOccurrences);
            QDateTime occurrenceDate(mIncidence->dtStart());
            occurrenceDate.setDate(startDate());
            KCalendarCore::Incidence::Ptr const newIncidence(KCalendarCore::Calendar::createException(mIncidence, occurrenceDate, thisAndFuture));
            if (newIncidence) {
                changer->startAtomicOperation(i18nc("@info/plain", "Move occurrence(s)"));
                setNewDates(newIncidence, startOffset, endOffset);
                changer->createIncidence(newIncidence, item.parentCollection(), parentWidget());
                changer->endAtomicOperation();
            } else {
                KMessageBox::error(parentWidget(),
                                   i18nc("@info",
                                         "Unable to add the exception item to the calendar. "
                                         "No change will be done."),
                                   i18nc("@title:window", "Error Occurred"));
            }
            break;
        }
        default:
            // nothing to do
            break;
        }
    } else { // Doesn't recur
        KCalendarCore::Incidence::Ptr const oldIncidence(mIncidence->clone());
        setNewDates(mIncidence, startOffset, endOffset);
        changer->modifyIncidence(item, oldIncidence);
    }
}

void IncidenceMonthItem::updateSelection(const Akonadi::Item &incidence, QDate date)
{
    Q_UNUSED(date)
    setSelected(incidence == akonadiItem());
}

QString IncidenceMonthItem::text(bool end) const
{
    QString ret = mIncidence->summary();
    if (!allDay() && !mIsJournal && monthScene()->monthView()->preferences()->showTimeInMonthView()) {
        // Prepend the time str to the text
        QString timeStr;
        if (mIsTodo) {
            KCalendarCore::Todo::Ptr const todo = mIncidence.staticCast<Todo>();
            timeStr = QLocale().toString(todo->dtDue().toLocalTime().time(), QLocale::ShortFormat);
        } else {
            if (!end) {
                QTime time;
                if (mIncidence->recurs()) {
                    const auto start = mIncidence->dtStart().addDays(mRecurDayOffset).addSecs(-1);
                    time = mIncidence->recurrence()->getNextDateTime(start).toLocalTime().time();
                } else {
                    time = mIncidence->dtStart().toLocalTime().time();
                }
                timeStr = QLocale().toString(time, QLocale::ShortFormat);
            } else {
                KCalendarCore::Event::Ptr const event = mIncidence.staticCast<Event>();
                timeStr = QLocale().toString(event->dtEnd().toLocalTime().time(), QLocale::ShortFormat);
            }
        }
        if (!timeStr.isEmpty()) {
            if (!end) {
                ret = timeStr + u' ' + ret;
            } else {
                ret = ret + u' ' + timeStr;
            }
        }
    }

    return ret;
}

QString IncidenceMonthItem::toolTipText(const QDate &date) const
{
    return KCalUtils::IncidenceFormatter::toolTipStr(Akonadi::CalendarUtils::displayName(mCalendar->model(), akonadiItem().parentCollection()),
                                                     mIncidence,
                                                     date,
                                                     true);
}

QList<QPixmap> IncidenceMonthItem::icons() const
{
    QList<QPixmap> ret;

    if (!mIncidence) {
        return ret;
    }

    bool specialEvent = false;
    Akonadi::Item const item = akonadiItem();

    const QSet<EventView::ItemIcon> icons = monthScene()->monthView()->preferences()->monthViewIcons();

    QString customIconName;
    if (icons.contains(EventViews::EventView::CalendarCustomIcon)) {
        const QString iconName = monthScene()->monthView()->iconForItem(item);
        if (!iconName.isEmpty() && iconName != QLatin1StringView("view-calendar") && iconName != QLatin1StringView("office-calendar")) {
            customIconName = iconName;
            ret << QPixmap(cachedSmallIcon(iconName));
        }
    }

    if (mIsEvent) {
        if (mIncidence->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES")) {
            specialEvent = true;
            ret << monthScene()->anniversaryPixmap();
        } else if (mIncidence->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES")) {
            specialEvent = true;
            // Disabling birthday icon because it's the birthday agent's icon
            // and we allow to display the agent's icon now.
            // ret << monthScene()->birthdayPixmap();
        }

        // smartins: Disabling the event Pixmap because:
        // 1. Save precious space so we can read the event's title better.
        // 2. We don't need a pixmap to tell us an item is an event we
        //    only need one to tell us it's not, as month view was designed for events.
        // 3. If only to-dos and journals have a pixmap they will be distinguished
        //    from event's much easier.

        // ret << monthScene()->eventPixmap();
    } else if ((mIsTodo || mIsJournal) && icons.contains(mIsTodo ? EventView::TaskIcon : EventView::JournalIcon)) {
        QDateTime const occurrenceDateTime = mIncidence->dateTime(Incidence::RoleRecurrenceStart).addDays(mRecurDayOffset);

        const QString incidenceIconName = mIncidence->iconName(occurrenceDateTime);
        if (customIconName != incidenceIconName) {
            ret << QPixmap(cachedSmallIcon(incidenceIconName));
        }
    }

    if (icons.contains(EventView::ReadOnlyIcon) && !mCalendar->hasRight(Akonadi::Collection::CanChangeItem) && !specialEvent) {
        ret << monthScene()->readonlyPixmap();
    }

    /* sorry, this looks too cluttered. disable until we can
       make something prettier; no idea at this time -- allen */
    if (icons.contains(EventView::ReminderIcon) && mIncidence->hasEnabledAlarms() && !specialEvent) {
        ret << monthScene()->alarmPixmap();
    }
    if (icons.contains(EventView::RecurringIcon) && mIncidence->recurs() && !specialEvent) {
        ret << monthScene()->recurPixmap();
    }
    // TODO: check what to do with Reply

    return ret;
}

QColor IncidenceMonthItem::catColor() const
{
    Q_ASSERT(mIncidence);
    const auto &prefs = monthScene()->monthView()->preferences();

    const auto &categories = mIncidence->categories();
    if (categories.isEmpty() || !Akonadi::TagCache::instance()->tagColor(categories.first()).isValid()) {
        const auto &colorPreference = prefs->monthViewColors();
        if (colorPreference == PrefsBase::CategoryOnly) {
            return CalendarSupport::KCalPrefs::instance()->unsetCategoryColor();
        }
        return EventViews::resourceColor(mCalendar->collection(), prefs);
    }
    return Akonadi::TagCache::instance()->tagColor(categories.first());
}

QColor IncidenceMonthItem::bgColor() const
{
    const auto &prefs = monthScene()->monthView()->preferences();

    if (!prefs->todosUseCategoryColors() && mIsTodo) {
        Todo::Ptr const todo = Akonadi::CalendarUtils::todo(akonadiItem());
        Q_ASSERT(todo);
        if (todo) {
            // this is dtDue if there's no dtRecurrence
            const auto dtRecurrence = todo->dtRecurrence().toLocalTime().date();
            const auto today = QDate::currentDate();
            if (startDate() >= dtRecurrence) {
                if (todo->isOverdue() && today > startDate()) {
                    return prefs->todoOverdueColor();
                }
                if (today == startDate() && !todo->isCompleted()) {
                    return prefs->todoDueTodayColor();
                }
            }
        }
    }

    const auto &colorPreference = prefs->monthViewColors();
    const auto bgDisplaysResource = colorPreference == PrefsBase::MonthItemResourceInsideCategoryOutside || colorPreference == PrefsBase::MonthItemResourceOnly;
    return bgDisplaysResource ? EventViews::resourceColor(mCalendar->collection(), prefs) : catColor();
}

QColor IncidenceMonthItem::frameColor() const
{
    const auto &prefs = monthScene()->monthView()->preferences();
    const auto frameDisplaysResource =
        (prefs->monthViewColors() == PrefsBase::MonthItemResourceOnly || prefs->monthViewColors() == PrefsBase::MonthItemCategoryInsideResourceOutside);
    const auto frameColor = frameDisplaysResource ? EventViews::resourceColor(mCalendar->collection(), prefs) : catColor();
    return EventView::itemFrameColor(frameColor, selected());
}

Akonadi::Item IncidenceMonthItem::akonadiItem() const
{
    if (mIncidence) {
        return mCalendar->item(mIncidence);
    } else {
        return {};
    }
}

KCalendarCore::Incidence::Ptr IncidenceMonthItem::incidence() const
{
    return mIncidence;
}

Akonadi::Item::Id IncidenceMonthItem::akonadiItemId() const
{
    return mAkonadiItemId;
}

Akonadi::CollectionCalendar::Ptr IncidenceMonthItem::calendar() const
{
    return mCalendar;
}

void IncidenceMonthItem::setNewDates(const KCalendarCore::Incidence::Ptr &incidence, int startOffset, int endOffset)
{
    if (mIsTodo) {
        // For to-dos endOffset is ignored because it will always be == to startOffset because we only
        // support moving to-dos, not resizing them. There are no multi-day to-dos.
        // Lets just call it offset to reduce confusion.
        const int offset = startOffset;

        KCalendarCore::Todo::Ptr const todo = incidence.staticCast<Todo>();
        QDateTime due = todo->dtDue();
        QDateTime start = todo->dtStart();
        if (due.isValid()) { // Due has priority over start.
            // We will only move the due date, unlike events where we move both.
            due = due.addDays(offset);
            todo->setDtDue(due);

            if (start.isValid() && start > due) {
                // Start can't be bigger than due.
                todo->setDtStart(due);
            }
        } else if (start.isValid()) {
            // So we're displaying a to-do that doesn't have due date, only start...
            start = start.addDays(offset);
            todo->setDtStart(start);
        } else {
            // This never happens
            qCWarning(CALENDARVIEW_LOG) << "Move what? uid:" << todo->uid() << "; summary=" << todo->summary();
        }
    } else {
        incidence->setDtStart(incidence->dtStart().addDays(startOffset));
        if (mIsEvent) {
            KCalendarCore::Event::Ptr const event = incidence.staticCast<Event>();
            event->setDtEnd(event->dtEnd().addDays(endOffset));
        }
    }
}

//-----------------------------------------------------------------
// HOLIDAYMONTHITEM
HolidayMonthItem::HolidayMonthItem(MonthScene *monthScene, QDate date, const QString &name)
    : HolidayMonthItem(monthScene, date, date, name)
{
}

HolidayMonthItem::HolidayMonthItem(MonthScene *monthScene, QDate startDate, QDate endDate, const QString &name)
    : MonthItem(monthScene)
    , mStartDate(startDate)
    , mEndDate(endDate)
    , mName(name)
{
}

HolidayMonthItem::~HolidayMonthItem() = default;

bool HolidayMonthItem::greaterThanFallback(const MonthItem *other) const
{
    const auto o = qobject_cast<const HolidayMonthItem *>(other);
    // always put holidays on top
    return !o;
}

void HolidayMonthItem::finalizeMove(const QDate &newStartDate)
{
    Q_UNUSED(newStartDate)
    Q_ASSERT(false);
}

void HolidayMonthItem::finalizeResize(const QDate &newStartDate, const QDate &newEndDate)
{
    Q_UNUSED(newStartDate)
    Q_UNUSED(newEndDate)
    Q_ASSERT(false);
}

QList<QPixmap> HolidayMonthItem::icons() const
{
    QList<QPixmap> ret;
    ret << monthScene()->holidayPixmap();

    return ret;
}

QColor HolidayMonthItem::bgColor() const
{
    // FIXME: Currently, only this value is settable in the options.
    // There is a monthHolidaysBackgroundColor() option too. Maybe it would be
    // wise to merge those two.
    return monthScene()->monthView()->preferences()->agendaHolidaysBackgroundColor();
}

QColor HolidayMonthItem::frameColor() const
{
    return Qt::black;
}

#include "moc_monthitem.cpp"

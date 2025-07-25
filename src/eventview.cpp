/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "eventview_p.h"
using namespace Qt::Literals::StringLiterals;

#include "prefs.h"

#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/KCalPrefs>

#include <Akonadi/CalendarUtils>
#include <Akonadi/ETMViewStateSaver>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityTreeModel>

#include <KCalendarCore/CalFilter>

#include <KCalUtils/RecurrenceActions>

#include <KHolidays/HolidayRegion>

#include "calendarview_debug.h"
#include <KCheckableProxyModel>
#include <KGuiItem>
#include <KLocalizedString>
#include <KRandom>
#include <KRearrangeColumnsProxyModel>
#include <KViewStateMaintainer>

#include <QApplication>
#include <QKeyEvent>
#include <QSortFilterProxyModel>

using namespace KCalendarCore;
using namespace EventViews;
using namespace Akonadi;

CalendarSupport::CollectionSelection *EventViewPrivate::sGlobalCollectionSelection = nullptr;

/* static */
void EventView::setGlobalCollectionSelection(CalendarSupport::CollectionSelection *s)
{
    EventViewPrivate::sGlobalCollectionSelection = s;
}

EventView::EventView(QWidget *parent)
    : QWidget(parent)
    , d_ptr(new EventViewPrivate(this))
{
    QByteArray cname = metaObject()->className();
    cname.replace(':', '_');
    d_ptr->identifier = cname + '_' + KRandom::randomString(8).toLatin1();

    // AKONADI_PORT review: the FocusLineEdit in the editor emits focusReceivedSignal(),
    // which triggered finishTypeAhead.  But the global focus widget in QApplication is
    // changed later, thus subsequent keyevents still went to this view, triggering another
    // editor, for each keypress.
    // Thus, listen to the global focusChanged() signal (seen in Qt 4.6-stable-patched 20091112 -Frank)
    connect(qobject_cast<QApplication *>(QApplication::instance()), &QApplication::focusChanged, this, &EventView::focusChanged);

    d_ptr->setUpModels();
}

EventView::~EventView() = default;

void EventView::defaultAction(const Akonadi::Item &item)
{
    qCDebug(CALENDARVIEW_LOG);
    const Incidence::Ptr incidence = Akonadi::CalendarUtils::incidence(item);
    if (!incidence) {
        return;
    }

    qCDebug(CALENDARVIEW_LOG) << "  type:" << int(incidence->type());

    if (incidence->isReadOnly()) {
        Q_EMIT showIncidenceSignal(item);
    } else {
        Q_EMIT editIncidenceSignal(item);
    }
}

void EventView::setHolidayRegions(const QStringList &regions)
{
    Q_D(EventView);
    d->mHolidayRegions.clear();
    for (const QString &regionStr : regions) {
        auto region = std::make_unique<KHolidays::HolidayRegion>(regionStr);
        if (region->isValid()) {
            d->mHolidayRegions.push_back(std::move(region));
        }
    }
}

int EventView::showMoveRecurDialog(const Incidence::Ptr &inc, QDate date)
{
    QDateTime const dateTime(date, {}, QTimeZone::LocalTime);

    int const availableOccurrences = KCalUtils::RecurrenceActions::availableOccurrences(inc, dateTime);

    const QString caption = i18nc("@title:window", "Changing Recurring Item");
    KGuiItem const itemFuture(i18n("Also &Future Items"));
    KGuiItem const itemSelected(i18n("Only &This Item"));
    KGuiItem const itemAll(i18n("&All Occurrences"));

    switch (availableOccurrences) {
    case KCalUtils::RecurrenceActions::NoOccurrence:
        return KCalUtils::RecurrenceActions::NoOccurrence;

    case KCalUtils::RecurrenceActions::SelectedOccurrence:
        return KCalUtils::RecurrenceActions::SelectedOccurrence;

    case KCalUtils::RecurrenceActions::AllOccurrences:
        Q_ASSERT(availableOccurrences & KCalUtils::RecurrenceActions::SelectedOccurrence);

        // if there are all kinds of ooccurrences (i.e. past present and future) the user might
        // want the option only apply to current and future occurrences, leaving the past ones
        // provide a third choice for that ("Also future")
        if (availableOccurrences == KCalUtils::RecurrenceActions::AllOccurrences) {
            const QString message = i18n(
                "The item you are trying to change is a recurring item. "
                "Should the changes be applied only to this single occurrence, "
                "also to future items, or to all items in the recurrence?");
            return KCalUtils::RecurrenceActions::questionSelectedFutureAllCancel(message, caption, itemSelected, itemFuture, itemAll, this);
        }
        [[fallthrough]];

    default:
        Q_ASSERT(availableOccurrences & KCalUtils::RecurrenceActions::SelectedOccurrence);
        // selected occurrence and either past or future occurrences
        const QString message = i18n(
            "The item you are trying to change is a recurring item. "
            "Should the changes be applied only to this single occurrence "
            "or to all items in the recurrence?");
        return KCalUtils::RecurrenceActions::questionSelectedAllCancel(message, caption, itemSelected, itemAll, this);
        break;
    }

    return KCalUtils::RecurrenceActions::NoOccurrence;
}

void EventView::addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    Q_D(EventView);
    d->mCalendars.push_back(calendar);
}

void EventView::removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    Q_D(EventView);
    d->mCalendars.removeOne(calendar);
}

void EventView::setModel(QAbstractItemModel *model)
{
    Q_D(EventView);
    if (d->model != model) {
        d->model = model;
        if (d->model) {
            if (d->collectionSelectionModel) {
                d->collectionSelectionModel->setSourceModel(d->model);
            }

            d->setEtm(d->model);
            d->setUpModels();

            connect(d->model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                Q_D(EventView);

                for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
                    const auto index = topLeft.siblingAtRow(i);
                    const auto col = d->model->data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
                    if (col.isValid()) {
                        // TODO: we have no way of knowing what has actually changed in the model :(
                        onCollectionChanged(col, {"AccessRights"});
                    }
                }
            });
        }
    }
}

QAbstractItemModel *EventView::model() const
{
    Q_D(const EventView);
    return d->model;
}

Akonadi::EntityTreeModel *EventView::entityTreeModel() const
{
    Q_D(const EventView);
    return d->etm;
}

void EventView::setPreferences(const PrefsPtr &preferences)
{
    Q_D(EventView);
    if (d->mPrefs != preferences) {
        if (preferences) {
            d->mPrefs = preferences;
        } else {
            d->mPrefs = PrefsPtr(new Prefs());
        }
        updateConfig();
    }
}

void EventView::setKCalPreferences(const KCalPrefsPtr &preferences)
{
    Q_D(EventView);
    if (d->mKCalPrefs != preferences) {
        if (preferences) {
            d->mKCalPrefs = preferences;
        } else {
            d->mKCalPrefs = KCalPrefsPtr(new CalendarSupport::KCalPrefs());
        }
        updateConfig();
    }
}

PrefsPtr EventView::preferences() const
{
    Q_D(const EventView);
    return d->mPrefs;
}

KCalPrefsPtr EventView::kcalPreferences() const
{
    Q_D(const EventView);
    return d->mKCalPrefs;
}

void EventView::dayPassed(const QDate &)
{
    updateView();
}

void EventView::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    Q_D(EventView);
    d->mChanger = changer;
}

void EventView::flushView()
{
}

EventView *EventView::viewAt(const QPoint &)
{
    return this;
}

void EventView::updateConfig()
{
}

QDateTime EventView::selectionStart() const
{
    return {};
}

QDateTime EventView::selectionEnd() const
{
    return {};
}

bool EventView::dateRangeSelectionEnabled() const
{
    Q_D(const EventView);
    return d->mDateRangeSelectionEnabled;
}

void EventView::setDateRangeSelectionEnabled(bool enable)
{
    Q_D(EventView);
    d->mDateRangeSelectionEnabled = enable;
}

bool EventView::supportsZoom() const
{
    return false;
}

bool EventView::hasConfigurationDialog() const
{
    return false;
}

void EventView::setDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth)
{
    Q_D(EventView);

    d->startDateTime = start;
    d->endDateTime = end;
    showDates(start.date(), end.date(), preferredMonth);
    const QPair<QDateTime, QDateTime> adjusted = actualDateRange(start, end, preferredMonth);
    d->actualStartDateTime = adjusted.first;
    d->actualEndDateTime = adjusted.second;
}

QDateTime EventView::startDateTime() const
{
    Q_D(const EventView);
    return d->startDateTime;
}

QDateTime EventView::endDateTime() const
{
    Q_D(const EventView);
    return d->endDateTime;
}

QDateTime EventView::actualStartDateTime() const
{
    Q_D(const EventView);
    return d->actualStartDateTime;
}

QDateTime EventView::actualEndDateTime() const
{
    Q_D(const EventView);
    return d->actualEndDateTime;
}

void EventView::showConfigurationDialog(QWidget *)
{
}

bool EventView::processKeyEvent(QKeyEvent *ke)
{
    Q_D(EventView);
    // If Return is pressed bring up an editor for the current selected time span.
    if (ke->key() == Qt::Key_Return) {
        if (ke->type() == QEvent::KeyPress) {
            d->mReturnPressed = true;
        } else if (ke->type() == QEvent::KeyRelease) {
            if (d->mReturnPressed) {
                if (d->startDateTime.isValid()) {
                    Q_EMIT newEventSignal(d->startDateTime);
                } else {
                    Q_EMIT newEventSignal();
                }
                d->mReturnPressed = false;
                return true;
            } else {
                d->mReturnPressed = false;
            }
        }
    }

    // Ignore all input that does not produce any output
    if (ke->text().isEmpty() || (ke->modifiers() & Qt::ControlModifier)) {
        return false;
    }

    if (ke->type() == QEvent::KeyPress) {
        switch (ke->key()) {
        case Qt::Key_Escape:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Control:
        case Qt::Key_Meta:
        case Qt::Key_Alt:
            break;
        default:
            d->mTypeAheadEvents.append(new QKeyEvent(ke->type(), ke->key(), ke->modifiers(), ke->text(), ke->isAutoRepeat(), static_cast<ushort>(ke->count())));
            if (!d->mTypeAhead) {
                d->mTypeAhead = true;
                if (d->startDateTime.isValid()) {
                    Q_EMIT newEventSignal(d->startDateTime);
                } else {
                    Q_EMIT newEventSignal();
                }
            }
            return true;
        }
    }
    return false;
}

void EventView::setTypeAheadReceiver(QObject *o)
{
    Q_D(EventView);
    d->mTypeAheadReceiver = o;
}

void EventView::focusChanged(QWidget *, QWidget *now)
{
    Q_D(EventView);
    if (d->mTypeAhead && now && now == d->mTypeAheadReceiver) {
        d->finishTypeAhead();
    }
}

CalendarSupport::CollectionSelection *EventView::collectionSelection() const
{
    Q_D(const EventView);
    return d->customCollectionSelection ? d->customCollectionSelection.get() : globalCollectionSelection();
}

void EventView::setCustomCollectionSelectionProxyModel(KCheckableProxyModel *model)
{
    Q_D(EventView);
    if (d->collectionSelectionModel == model) {
        return;
    }

    delete d->collectionSelectionModel;
    d->collectionSelectionModel = model;
    d->setUpModels();
}

KCheckableProxyModel *EventView::customCollectionSelectionProxyModel() const
{
    Q_D(const EventView);
    return d->collectionSelectionModel;
}

KCheckableProxyModel *EventView::takeCustomCollectionSelectionProxyModel()
{
    Q_D(EventView);
    KCheckableProxyModel *m = d->collectionSelectionModel;
    d->collectionSelectionModel = nullptr;
    d->setUpModels();
    return m;
}

CalendarSupport::CollectionSelection *EventView::customCollectionSelection() const
{
    Q_D(const EventView);
    return d->customCollectionSelection.get();
}

void EventView::clearSelection()
{
}

bool EventView::eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const
{
    Q_UNUSED(startDt)
    Q_UNUSED(endDt)
    Q_UNUSED(allDay)
    return false;
}

Akonadi::IncidenceChanger *EventView::changer() const
{
    Q_D(const EventView);
    return d->mChanger;
}

void EventView::doRestoreConfig(const KConfigGroup &)
{
}

void EventView::doSaveConfig(KConfigGroup &)
{
}

QPair<QDateTime, QDateTime> EventView::actualDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth) const
{
    Q_UNUSED(preferredMonth)
    return qMakePair(start, end);
}

/*
void EventView::incidencesAdded( const Akonadi::Item::List & )
{
}

void EventView::incidencesAboutToBeRemoved( const Akonadi::Item::List & )
{
}

void EventView::incidencesChanged( const Akonadi::Item::List & )
{
}
*/

void EventView::handleBackendError(const QString &errorString)
{
    qCCritical(CALENDARVIEW_LOG) << errorString;
}

void EventView::calendarReset()
{
}

CalendarSupport::CollectionSelection *EventView::globalCollectionSelection()
{
    return EventViewPrivate::sGlobalCollectionSelection;
}

QByteArray EventView::identifier() const
{
    Q_D(const EventView);
    return d->identifier;
}

void EventView::setIdentifier(const QByteArray &identifier)
{
    Q_D(EventView);
    d->identifier = identifier;
}

void EventView::setChanges(Changes changes)
{
    Q_D(EventView);
    if (d->mChanges == NothingChanged) {
        QMetaObject::invokeMethod(this, &EventView::updateView, Qt::QueuedConnection);
    }

    d->mChanges = changes;
}

EventView::Changes EventView::changes() const
{
    Q_D(const EventView);
    return d->mChanges;
}

void EventView::restoreConfig(const KConfigGroup &configGroup)
{
    Q_D(EventView);
    const bool useCustom = configGroup.readEntry("UseCustomCollectionSelection", false);
    if (!d->collectionSelectionModel && !useCustom) {
        delete d->collectionSelectionModel;
        d->collectionSelectionModel = nullptr;
        d->setUpModels();
    } else if (useCustom) {
        if (!d->collectionSelectionModel) {
            // Sort the calendar model on calendar name
            auto sortProxy = new QSortFilterProxyModel(this);
            sortProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
            sortProxy->setSourceModel(d->model);

            // Only show the first column.
            auto columnFilterProxy = new KRearrangeColumnsProxyModel(this);
            columnFilterProxy->setSourceColumns(QList<int>() << 0);
            columnFilterProxy->setSourceModel(sortProxy);

            // Make the calendar model checkable.
            d->collectionSelectionModel = new KCheckableProxyModel(this);
            d->collectionSelectionModel->setSourceModel(columnFilterProxy);

            d->setUpModels();
        }
        const KConfigGroup selectionGroup = configGroup.config()->group(configGroup.name() + "_selectionSetup"_L1);

        KViewStateMaintainer<ETMViewStateSaver> maintainer(selectionGroup);
        maintainer.setSelectionModel(d->collectionSelectionModel->selectionModel());
        maintainer.restoreState();
    }

    doRestoreConfig(configGroup);
}

void EventView::saveConfig(KConfigGroup &configGroup)
{
    Q_D(EventView);
    configGroup.writeEntry("UseCustomCollectionSelection", d->collectionSelectionModel != nullptr);

    if (d->collectionSelectionModel) {
        KConfigGroup const selectionGroup = configGroup.config()->group(configGroup.name() + "_selectionSetup"_L1);

        KViewStateMaintainer<ETMViewStateSaver> maintainer(selectionGroup);
        maintainer.setSelectionModel(d->collectionSelectionModel->selectionModel());
        maintainer.saveState();
    }

    doSaveConfig(configGroup);
}

bool EventView::makesWholeDayBusy(const KCalendarCore::Incidence::Ptr &incidence) const
{
    // Must be event
    // Must be all day
    // Must be marked busy (TRANSP: OPAQUE)
    // You must be attendee or organizer

    if (incidence->type() != KCalendarCore::Incidence::TypeEvent || !incidence->allDay()) {
        return false;
    }

    KCalendarCore::Event::Ptr const ev = incidence.staticCast<KCalendarCore::Event>();

    if (ev->transparency() != KCalendarCore::Event::Opaque) {
        return false;
    }

    // Last check: must be organizer or attendee:

    if (kcalPreferences()->thatIsMe(ev->organizer().email())) {
        return true;
    }

    KCalendarCore::Attendee::List const attendees = ev->attendees();
    KCalendarCore::Attendee::List::ConstIterator it;
    for (it = attendees.constBegin(); it != attendees.constEnd(); ++it) {
        if (kcalPreferences()->thatIsMe((*it).email())) {
            return true;
        }
    }

    return false;
}

/*static*/
QColor EventView::itemFrameColor(const QColor &color, bool selected)
{
    if (color.isValid()) {
        return selected ? QColor(85 + color.red() * 2.0 / 3, 85 + color.green() * 2.0 / 3, 85 + color.blue() * 2.0 / 3) : color.darker(115);
    } else {
        return Qt::black;
    }
}

QString EventView::iconForItem(const Akonadi::Item &item)
{
    Q_D(EventView);
    QString iconName;
    Akonadi::Collection collection = Akonadi::EntityTreeModel::updatedCollection(d->model, item.storageCollectionId());
    if (collection.isValid() && collection.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
        iconName = collection.attribute<Akonadi::EntityDisplayAttribute>()->iconName();
    }
    // storageCollection typically returns a generic fallback icon, which we ignore for this purpose
    if (iconName.isEmpty() || iconName.startsWith("view-calendar"_L1) || iconName.startsWith("office-calendar"_L1) || iconName.startsWith("view-pim"_L1)) {
        collection = item.parentCollection();
        while (collection.parentCollection().isValid() && collection.parentCollection() != Akonadi::Collection::root()) {
            collection = Akonadi::EntityTreeModel::updatedCollection(d->model, collection.parentCollection());
        }

        if (collection.isValid() && collection.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
            iconName = collection.attribute<Akonadi::EntityDisplayAttribute>()->iconName();
        }
    }

    return iconName;
}

void EventView::onCollectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    Q_UNUSED(collection)
    if (changedAttributes.contains("AccessRights")) {
        setChanges(changes() | EventViews::EventView::ResourcesChanged);
        updateView();
    }
}

QList<Akonadi::CollectionCalendar::Ptr> EventView::calendars() const
{
    Q_D(const EventView);
    return d->mCalendars;
}

Akonadi::CollectionCalendar::Ptr EventView::calendar3(const Akonadi::Item &item) const
{
    return calendarForCollection(item.storageCollectionId());
}

Akonadi::CollectionCalendar::Ptr EventView::calendar3(const KCalendarCore::Incidence::Ptr &incidence) const
{
    const auto collectionId = incidence->customProperty("VOLATILE", "COLLECTION-ID").toLongLong();
    return calendarForCollection(collectionId);
}

Akonadi::CollectionCalendar::Ptr EventView::calendarForCollection(Akonadi::Collection::Id collectionId) const
{
    const auto hasCollectionId = [collectionId](const Akonadi::CollectionCalendar::Ptr &calendar) {
        return calendar->collection().id() == collectionId;
    };

    Q_D(const EventView);
    const auto cal = std::find_if(d->mCalendars.cbegin(), d->mCalendars.cend(), hasCollectionId);
    return cal != d->mCalendars.cend() ? *cal : Akonadi::CollectionCalendar::Ptr{};
}

Akonadi::CollectionCalendar::Ptr EventView::calendarForCollection(const Akonadi::Collection &collection) const
{
    return calendarForCollection(collection.id());
}

#include "moc_eventview.cpp"

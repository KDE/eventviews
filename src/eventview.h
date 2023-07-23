/*
  SPDX-FileCopyrightText: 1999 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventviews_export.h"

#include <Akonadi/Collection>
#include <Akonadi/CollectionCalendar>
#include <Akonadi/Item>

#include <KCalendarCore/Incidence>
#include <KCalendarCore/Todo>

#include <QByteArray>
#include <QDate>
#include <QSet>
#include <QWidget>

#include <memory>

class QAbstractItemModel;

namespace CalendarSupport
{
class CollectionSelection;
class KCalPrefs;
}

namespace Akonadi
{
class IncidenceChanger;
}

class KCheckableProxyModel;
class KConfigGroup;

namespace EventViews
{
enum { BUSY_BACKGROUND_ALPHA = 70 };

class EventViewPrivate;
class Prefs;
using PrefsPtr = QSharedPointer<Prefs>;
using KCalPrefsPtr = QSharedPointer<CalendarSupport::KCalPrefs>;

/**
  EventView is the abstract base class from which all other calendar views
  for event data are derived.  It provides methods for displaying
  appointments and events on one or more days.  The actual number of
  days that a view actually supports is not defined by this abstract class;
  that is up to the classes that inherit from it.  It also provides
  methods for updating the display, retrieving the currently selected
  event (or events), and the like.

  @short Abstract class from which all event views are derived.
  @author Preston Brown <pbrown@kde.org>
  @see KOListView, AgendaView, KOMonthView
*/
class EVENTVIEWS_EXPORT EventView : public QWidget
{
    Q_OBJECT
public:
    enum {
        // This value is passed to QColor's lighter(int factor) for selected events
        BRIGHTNESS_FACTOR = 110
    };

    enum ItemIcon {
        CalendarCustomIcon = 0,
        TaskIcon,
        JournalIcon,
        RecurringIcon,
        ReminderIcon,
        ReadOnlyIcon,
        ReplyIcon,
        AttendingIcon,
        TentativeIcon,
        OrganizerIcon,
        IconCount = 10 // Always keep at the end
    };

    enum Change {
        NothingChanged = 0,
        IncidencesAdded = 1,
        IncidencesEdited = 2,
        IncidencesDeleted = 4,
        DatesChanged = 8,
        FilterChanged = 16,
        ResourcesChanged = 32,
        ZoomChanged = 64,
        ConfigChanged = 128
    };
    Q_DECLARE_FLAGS(Changes, Change)

    /**
     * Constructs a view.
     * @param cal is a pointer to the calendar object from which events
     *        will be retrieved for display.
     * @param parent is the parent QWidget.
     */
    explicit EventView(QWidget *parent = nullptr);

    /**
     * Destructor. Views will do view-specific cleanups here.
     */
    ~EventView() override;

    virtual void addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar);
    virtual void removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar);

    virtual void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;
    Akonadi::EntityTreeModel *entityTreeModel() const;

    /*
      update config is called after prefs are set.
    */
    virtual void setPreferences(const PrefsPtr &preferences);
    Q_REQUIRED_RESULT PrefsPtr preferences() const;

    virtual void setKCalPreferences(const KCalPrefsPtr &preferences);
    Q_REQUIRED_RESULT KCalPrefsPtr kcalPreferences() const;

    /**
      @return a list of selected events. Most views can probably only
      select a single event at a time, but some may be able to select
      more than one.
    */
    virtual Akonadi::Item::List selectedIncidences() const = 0;

    /**
      Returns a list of the dates of selected events. Most views can
      probably only select a single event at a time, but some may be able
      to select more than one.
    */
    virtual KCalendarCore::DateList selectedIncidenceDates() const = 0;

    /**
       Returns the start of the selection, or an invalid QDateTime if there is no selection
       or the view doesn't support selecting cells.
     */
    virtual QDateTime selectionStart() const;

    /**
      Returns the end of the selection, or an invalid QDateTime if there is no selection
      or the view doesn't support selecting cells.
     */
    virtual QDateTime selectionEnd() const;

    /**
      Sets the default start/end date/time for new events.
      Return true if anything was changed
    */
    virtual bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const;

    /**
      Returns whether or not date range selection is enabled. This setting only
      applies to views that actually supports selecting cells.
      @see selectionStart()
      @see selectionEnd()
     */
    Q_REQUIRED_RESULT bool dateRangeSelectionEnabled() const;

    /**
      Enable or disable date range selection.
      @see dateRangeSelectionEnabled()
     */
    void setDateRangeSelectionEnabled(bool enable);

    /**
      Returns the number of currently shown dates.
      A return value of 0 means no idea.
    */
    virtual int currentDateCount() const = 0;

    /**
     * returns whether this view supports zoom.
     * Base implementation returns false.
     */
    virtual bool supportsZoom() const;

    virtual bool hasConfigurationDialog() const;

    virtual void showConfigurationDialog(QWidget *parent);

    Q_REQUIRED_RESULT QByteArray identifier() const;
    void setIdentifier(const QByteArray &identifier);

    /**
     * reads the view configuration. View-specific configuration can be
     * restored via doRestoreConfig()
     *
     * @param configGroup the group to read settings from
     * @see doRestoreConfig()
     */
    void restoreConfig(const KConfigGroup &configGroup);

    /**
     * writes out the view configuration. View-specific configuration can be
     * saved via doSaveConfig()
     *
     * @param configGroup the group to store settings in
     * @see doSaveConfig()
     */
    void saveConfig(KConfigGroup &configGroup);

    //----------------------------------------------------------------------------
    KCheckableProxyModel *takeCustomCollectionSelectionProxyModel();
    KCheckableProxyModel *customCollectionSelectionProxyModel() const;
    void setCustomCollectionSelectionProxyModel(KCheckableProxyModel *model);

    CalendarSupport::CollectionSelection *customCollectionSelection() const;

    static CalendarSupport::CollectionSelection *globalCollectionSelection();
    static void setGlobalCollectionSelection(CalendarSupport::CollectionSelection *selection);
    //----------------------------------------------------------------------------

    /**
     * returns the view at the given widget coordinate. This is usually the view
     * itself, except for composite views, where a subview will be returned.
     * The default implementation returns @p this .
     */
    virtual EventView *viewAt(const QPoint &p);

    /**
     * @param preferredMonth Used by month orientated views.  Contains the
     * month to show when the week crosses months.  It's a QDate instead
     * of uint so it can be easily fed to KCalendarSystem's functions.
     */
    virtual void setDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth = QDate());

    Q_REQUIRED_RESULT QDateTime startDateTime() const;
    Q_REQUIRED_RESULT QDateTime endDateTime() const;

    Q_REQUIRED_RESULT QDateTime actualStartDateTime() const;
    Q_REQUIRED_RESULT QDateTime actualEndDateTime() const;

    Q_REQUIRED_RESULT int showMoveRecurDialog(const KCalendarCore::Incidence::Ptr &incidence, QDate date);

    /**
      Handles key events, opens the new event dialog when enter is pressed, activates type ahead.
    */
    Q_REQUIRED_RESULT bool processKeyEvent(QKeyEvent *);

    /*
     * Sets the QObject that will receive key events that were made
     * while the new event dialog was still being created.
     */
    void setTypeAheadReceiver(QObject *o);

    /**
      Returns the selection of collection to be used by this view
      (custom if set, or global otherwise).
    */
    CalendarSupport::CollectionSelection *collectionSelection() const;

    /**
      Notifies the view that there are pending changes so a redraw is needed.
      @param needed if the update is needed or not.
    */
    virtual void setChanges(Changes changes);

    /**
      Returns if there are pending changes and a redraw is needed.
    */
    Q_REQUIRED_RESULT Changes changes() const;

    /**
     * Returns a variation of @p color that will be used for the border
     * of an agenda or month item.
     */
    Q_REQUIRED_RESULT static QColor itemFrameColor(const QColor &color, bool selected);

    Q_REQUIRED_RESULT QString iconForItem(const Akonadi::Item &);

public Q_SLOTS:
    /**
      Shows given incidences. Depending on the actual view it might not
      be possible to show all given events.

      @param incidenceList a list of incidences to show.
      @param date is the QDate on which the incidences are being shown.
    */
    virtual void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) = 0;

    /**
      Updates the current display to reflect changes that may have happened
      in the calendar since the last display refresh.
    */
    virtual void updateView() = 0;
    virtual void dayPassed(const QDate &);

    /**
      Assign a new incidence change helper object.
     */
    virtual void setIncidenceChanger(Akonadi::IncidenceChanger *changer);

    /**
      Write all unsaved data back to calendar store.
    */
    virtual void flushView();

    /**
      Re-reads the configuration and picks up relevant
      changes which are applicable to the view.
    */
    virtual void updateConfig();

    /**
      Clear selection. The incidenceSelected signal is not emitted.
    */
    virtual void clearSelection();

    void focusChanged(QWidget *, QWidget *);

    /**
     Perform the default action for an incidence, e.g. open the event editor,
     when double-clicking an event in the agenda view.
    */
    void defaultAction(const Akonadi::Item &incidence);

    /**
       Set which holiday regions the user wants to use.
       @param regions a list of Holiday Regions strings.
    */
    void setHolidayRegions(const QStringList &regions);

Q_SIGNALS:
    /**
     * when the view changes the dates that are selected in one way or
     * another, this signal is emitted.  It should be connected back to
     * the KDateNavigator object so that it changes appropriately,
     * and any other objects that need to be aware that the list of
     * selected dates has changed.
     *   @param datelist the new list of selected dates
     */
    void datesSelected(const KCalendarCore::DateList &datelist);

    /**
     * Emitted when an event is moved using the mouse in an agenda
     * view (week / month).
     */
    void shiftedEvent(const QDate &olddate, const QDate &newdate);

    void incidenceSelected(const Akonadi::Item &, const QDate);

    /**
     * instructs the receiver to show the incidence in read-only mode.
     */
    void showIncidenceSignal(const Akonadi::Item &);

    /**
     * instructs the receiver to begin editing the incidence specified in
     * some manner.  Doesn't make sense to connect to more than one
     * receiver.
     */
    void editIncidenceSignal(const Akonadi::Item &);

    /**
     * instructs the receiver to delete the Incidence in some manner; some
     * possibilities include automatically, with a confirmation dialog
     * box, etc.  Doesn't make sense to connect to more than one receiver.
     */
    void deleteIncidenceSignal(const Akonadi::Item &);

    /**
     * instructs the receiver to cut the Incidence
     */
    void cutIncidenceSignal(const Akonadi::Item &);

    /**
     * instructs the receiver to copy the incidence
     */
    void copyIncidenceSignal(const Akonadi::Item &);

    /**
     * instructs the receiver to paste the incidence
     */
    void pasteIncidenceSignal();

    /**
     * instructs the receiver to toggle the alarms of the Incidence.
     */
    void toggleAlarmSignal(const Akonadi::Item &);

    /**
     * instructs the receiver to toggle the completion state of the Incidence
     * (which must be a Todo type).
     */
    void toggleTodoCompletedSignal(const Akonadi::Item &);

    /**
     * Copy the incidence to the specified resource.
     */
    void copyIncidenceToResourceSignal(const Akonadi::Item &, const Akonadi::Collection &);

    /**
     * Move the incidence to the specified resource.
     */
    void moveIncidenceToResourceSignal(const Akonadi::Item &, const Akonadi::Collection &);

    /** Dissociate from a recurring incidence the occurrence on the given
     *  date to a new incidence or dissociate all occurrences from the
     *  given date onwards.
     */
    void dissociateOccurrencesSignal(const Akonadi::Item &, const QDate &);

    /**
     * instructs the receiver to create a new event in given collection. Doesn't make
     * sense to connect to more than one receiver.
     */
    void newEventSignal();
    /**
     * instructs the receiver to create a new event with the specified beginning
     * time. Doesn't make sense to connect to more than one receiver.
     */
    void newEventSignal(const QDate &);
    /**
     * instructs the receiver to create a new event with the specified beginning
     * time. Doesn't make sense to connect to more than one receiver.
     */
    void newEventSignal(const QDateTime &);
    /**
     * instructs the receiver to create a new event, with the specified
     * beginning end ending times.  Doesn't make sense to connect to more
     * than one receiver.
     */
    void newEventSignal(const QDateTime &, const QDateTime &);

    void newTodoSignal(const QDate &);
    void newSubTodoSignal(const Akonadi::Item &);

    void newJournalSignal(const QDate &);

protected Q_SLOTS:
    virtual void calendarReset();

private:
    EVENTVIEWS_NO_EXPORT void onCollectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &);

protected:
    QVector<Akonadi::CollectionCalendar::Ptr> calendars() const;
    Akonadi::CollectionCalendar::Ptr calendar3(const Akonadi::Item &item) const;
    Akonadi::CollectionCalendar::Ptr calendar3(const KCalendarCore::Incidence::Ptr &incidence) const;
    Akonadi::CollectionCalendar::Ptr calendarForCollection(const Akonadi::Collection &collection) const;
    Akonadi::CollectionCalendar::Ptr calendarForCollection(Akonadi::Collection::Id collectionId) const;

    bool makesWholeDayBusy(const KCalendarCore::Incidence::Ptr &incidence) const;
    Akonadi::IncidenceChanger *changer() const;

    /**
     * reimplement to read view-specific settings.
     */
    virtual void doRestoreConfig(const KConfigGroup &configGroup);

    /**
     * reimplement to write view-specific settings.
     */
    virtual void doSaveConfig(KConfigGroup &configGroup);

    /**
      @deprecated
     */
    virtual void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth = QDate()) = 0;

    /**
     * from the requested date range (passed via setDateRange()), calculates the
     * adjusted date range actually displayed by the view, depending on the
     * view's supported range (e.g., a month view always displays one month)
     * The default implementation returns the range unmodified
     *
     * @param preferredMonth Used by month orientated views. Contains the
     * month to show when the week crosses months.  It's a QDate instead of
     * uint so it can be easily fed to KCalendarSystem's functions.
     */
    virtual QPair<QDateTime, QDateTime> actualDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth = QDate()) const;
    /*
    virtual void incidencesAdded( const Akonadi::Item::List &incidences );
    virtual void incidencesAboutToBeRemoved( const Akonadi::Item::List &incidences );
    virtual void incidencesChanged( const Akonadi::Item::List &incidences );
    */
    virtual void handleBackendError(const QString &error);

private:
    std::unique_ptr<EventViewPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(EventView)
};
}

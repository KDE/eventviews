/*
  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"
#include "eventviews_export.h"
#include "viewcalendar.h"

#include <KCalendarCore/Todo>

#include <memory>

class KConfig;

class QSplitter;

namespace EventViews
{
namespace CalendarDecoration
{
class Decoration;
}

class TimeLabels;
class TimeLabelsZone;

class Agenda;
class AgendaItem;
class AgendaView;

class EventIndicatorPrivate;

class EventIndicator : public QWidget
{
    Q_OBJECT
public:
    enum Location { Top, Bottom };
    explicit EventIndicator(Location loc = Top, QWidget *parent = nullptr);
    ~EventIndicator() override;

    void changeColumns(int columns);

    void enableColumn(int column, bool enable);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *, QEvent *) override;

private:
    std::unique_ptr<EventIndicatorPrivate> const d;
};

class AgendaViewPrivate;

/**
  AgendaView is the agenda-like view that displays events in a single
  or multi-day view.
*/
class EVENTVIEWS_EXPORT AgendaView : public EventView
{
    Q_OBJECT
public:
    explicit AgendaView(const PrefsPtr &preferences, QDate start, QDate end, bool isInteractive, bool isSideBySide = false, QWidget *parent = nullptr);

    explicit AgendaView(QDate start, QDate end, bool isInteractive, bool isSideBySide = false, QWidget *parent = nullptr);

    ~AgendaView() override;

    enum {
        MAX_DAY_COUNT = 42 // (6 * 7)
    };

    void addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;
    void removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;

    /** Returns number of currently shown dates. */
    Q_REQUIRED_RESULT int currentDateCount() const override;

    /** returns the currently selected events */
    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;

    /** returns the currently selected incidence's dates */
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override;

    /** return the default start/end date/time for new events   */
    bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const override;

    /** start-datetime of selection */
    Q_REQUIRED_RESULT QDateTime selectionStart() const override;

    /** end-datetime of selection */
    Q_REQUIRED_RESULT QDateTime selectionEnd() const override;

    /** returns true if selection is for whole day */
    Q_REQUIRED_RESULT bool selectedIsAllDay() const;

    /** make selected start/end invalid */
    void deleteSelectedDateTime();

    /** returns if only a single cell is selected, or a range of cells */
    Q_REQUIRED_RESULT bool selectedIsSingleCell() const;

    /* reimp from EventView */
    virtual void addCalendar(const ViewCalendar::Ptr &cal);

    QSplitter *splitter() const;

    // FIXME: we already have startDateTime() and endDateTime() in the base class

    /** First shown day */
    Q_REQUIRED_RESULT QDate startDate() const;
    /** Last shown day */
    Q_REQUIRED_RESULT QDate endDate() const;

    /** Update event belonging to agenda item
        If the incidence is multi-day, item is the first one
    */
    void updateEventDates(AgendaItem *item, bool addIncidence, Akonadi::Collection::Id collectionId);

    Q_REQUIRED_RESULT QList<bool> busyDayMask() const;

    /**
     * Return calendar object for a concrete incidence.
     * this function is able to use multiple calendars
     * TODO: replace EventsView::calendar()
     */
    virtual KCalendarCore::Calendar::Ptr calendar2(const KCalendarCore::Incidence::Ptr &incidence) const;
    virtual KCalendarCore::Calendar::Ptr calendar2(const QString &incidenceIdentifier) const;

    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth = QDate()) override;

    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;

    void clearSelection() override;

    void startDrag(const Akonadi::Item &);

    void readSettings();
    void readSettings(const KConfig *);
    void writeSettings(KConfig *);

    void enableAgendaUpdate(bool enable);
    void setIncidenceChanger(Akonadi::IncidenceChanger *changer) override;

    void zoomInHorizontally(QDate date = QDate());
    void zoomOutHorizontally(QDate date = QDate());

    void zoomInVertically();
    void zoomOutVertically();

    void zoomView(const int delta, QPoint pos, const Qt::Orientation orient = Qt::Horizontal);

    void clearTimeSpanSelection();

    // Used by the timelabelszone
    void updateTimeBarWidth();
    /** Create labels for the selected dates. */
    void createDayLabels(bool force);

    void createTimeBarHeaders();

    void setChanges(EventView::Changes) override;

Q_SIGNALS:
    void showNewEventPopupSignal();
    void showIncidencePopupSignal(const Akonadi::CollectionCalendar::Ptr &, const Akonadi::Item &, const QDate &);
    void zoomViewHorizontally(const QDate &, int count);

    void timeSpanSelectionChanged();

protected:
    /** Fill agenda using the current set value for the start date */
    void fillAgenda();

    void connectAgenda(Agenda *agenda, Agenda *otherAgenda);

    /**
      Set the masks on the agenda widgets indicating, which days are holidays.
    */
    void setHolidayMasks();

    void removeIncidence(const KCalendarCore::Incidence::Ptr &inc);

public Q_SLOTS:
    void updateView() override;
    void updateConfig() override;
    /** reschedule the todo  to the given x- and y- coordinates.
        Third parameter determines all-day (no time specified) */
    void slotIncidencesDropped(const KCalendarCore::Incidence::List &incidences, const QPoint &, bool);
    void slotIncidencesDropped(const QList<QUrl> &incidences, const QPoint &, bool);
    void startDrag(const KCalendarCore::Incidence::Ptr &);

protected Q_SLOTS:
    void updateEventIndicatorTop(int newY);
    void updateEventIndicatorBottom(int newY);

    /** Updates data for selected timespan */
    void newTimeSpanSelected(const QPoint &start, const QPoint &end);
    /** Updates data for selected timespan for all day event*/
    void newTimeSpanSelectedAllDay(const QPoint &start, const QPoint &end);
    /**
      Updates the event indicators after a certain incidence was modified or
      removed.
    */
    void updateEventIndicators();
    void scheduleUpdateEventIndicators();

    void alignAgendas();

protected:
    void showEvent(QShowEvent *showEvent) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private Q_SLOTS:
    void slotIncidenceSelected(const KCalendarCore::Incidence::Ptr &incidence, QDate date);
    void slotShowIncidencePopup(const KCalendarCore::Incidence::Ptr &incidence, QDate date);
    void slotEditIncidence(const KCalendarCore::Incidence::Ptr &incidence);
    void slotShowIncidence(const KCalendarCore::Incidence::Ptr &incidence);
    void slotDeleteIncidence(const KCalendarCore::Incidence::Ptr &incidence);

private:
    void init(QDate start, QDate end);
    bool filterByCollectionSelection(const KCalendarCore::Incidence::Ptr &incidence);
    void setupTimeLabel(TimeLabels *timeLabel);
    bool displayIncidence(const KCalendarCore::Incidence::Ptr &incidence, bool createSelected);

    friend class TimeLabelsZone;
    friend class MultiAgendaViewPrivate;
    friend class MultiAgendaView;
    Agenda *agenda() const;
    Agenda *allDayAgenda() const;

private:
    friend class AgendaViewPrivate;
    std::unique_ptr<AgendaViewPrivate> const d;
};
}

/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Bertjan Broeksema <broeksema@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "eventview.h"

#include <KHolidays/HolidayRegion>

#include <memory>

namespace EventViews
{
class MonthViewPrivate;

/**
  New month view.
*/
class EVENTVIEWS_EXPORT MonthView : public EventView
{
    Q_OBJECT
public:
    enum NavButtonsVisibility {
        Visible,
        Hidden
    };

    explicit MonthView(NavButtonsVisibility visibility = Visible, QWidget *parent = nullptr);
    ~MonthView() override;

    void addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;
    void removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;

    [[nodiscard]] int currentDateCount() const override;
    [[nodiscard]] int currentMonth() const;

    [[nodiscard]] Akonadi::Item::List selectedIncidences() const override;

    /** Returns dates of the currently selected events */
    [[nodiscard]] KCalendarCore::DateList selectedIncidenceDates() const override;

    [[nodiscard]] QDateTime selectionStart() const override;

    [[nodiscard]] QDateTime selectionEnd() const override;

    void setDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth = QDate()) override;

    [[nodiscard]] bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const override;

    /**
     * Returns the average date in the view
     */
    [[nodiscard]] QDate averageDate() const;

    [[nodiscard]] bool usesFullWindow();

    void enableMonthYearHeader(bool enable);

    [[nodiscard]] bool hasEnabledMonthYearHeader();

    [[nodiscard]] bool supportsDateRangeSelection() const
    {
        return false;
    }

    [[nodiscard]] bool isBusyDay(QDate day) const;

Q_SIGNALS:
    void showIncidencePopupSignal(const Akonadi::CollectionCalendar::Ptr &calendar, const Akonadi::Item &item, const QDate &date);
    void showNewEventPopupSignal();
    void fullViewChanged(bool enabled);

public Q_SLOTS:
    void updateConfig() override;
    void updateView() override;
    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;

    void changeIncidenceDisplay(const Akonadi::Item &, int);
    void changeFullView(); /// Display in full window mode
    void moveBackMonth(); /// Shift the view one month back
    void moveBackWeek(); /// Shift the view one week back
    void moveFwdWeek(); /// Shift the view one week forward
    void moveFwdMonth(); /// Shift the view one month forward

protected Q_SLOTS:
    void calendarReset() override;

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) override;
#endif
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    QPair<QDateTime, QDateTime> actualDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth = QDate()) const override;

    KHolidays::Holiday::List holidays(QDate startDate, QDate endDate);

    // Compute and update the whole view
    void reloadIncidences();

protected:
    /**
     * @deprecated
     */
    void showDates(const QDate &start, const QDate &end, const QDate &preferedMonth = QDate()) override;

private:
    std::unique_ptr<MonthViewPrivate> const d;
    friend class MonthViewPrivate;
    friend class MonthScene;
};
}

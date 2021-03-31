/*
  SPDX-FileCopyrightText: 2008 Bruno Virlet <bruno.virlet@gmail.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Bertjan Broeksema <broeksema@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "eventview.h"

class QModelIndex;

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
    enum NavButtonsVisibility { Visible, Hidden };

    explicit MonthView(NavButtonsVisibility visibility = Visible, QWidget *parent = nullptr);
    ~MonthView() override;

    Q_REQUIRED_RESULT int currentDateCount() const override;
    Q_REQUIRED_RESULT int currentMonth() const;

    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;

    /** Returns dates of the currently selected events */
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override;

    Q_REQUIRED_RESULT QDateTime selectionStart() const override;

    Q_REQUIRED_RESULT QDateTime selectionEnd() const override;

    virtual void setDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth = QDate()) override;

    Q_REQUIRED_RESULT bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const override;

    /**
     * Returns the average date in the view
     */
    Q_REQUIRED_RESULT QDate averageDate() const;

    Q_REQUIRED_RESULT bool usesFullWindow();

    Q_REQUIRED_RESULT bool supportsDateRangeSelection() const
    {
        return false;
    }

    Q_REQUIRED_RESULT bool isBusyDay(const QDate &day) const;

    void setCalendar(const Akonadi::ETMCalendar::Ptr &cal) override;

Q_SIGNALS:
    void showIncidencePopupSignal(const Akonadi::Item &item, const QDate &date);
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

private Q_SLOTS:
    // void dataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );
    // void rowsInserted( const QModelIndex &parent, int start, int end );
    // void rowsAboutToBeRemoved( const QModelIndex &parent, int start, int end );

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) override;
#endif
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    QPair<QDateTime, QDateTime> actualDateRange(const QDateTime &start, const QDateTime &end, const QDate &preferredMonth = QDate()) const override;

    // Compute and update the whole view
    void reloadIncidences();

protected:
    /**
     * @deprecated
     */
    void showDates(const QDate &start, const QDate &end, const QDate &preferedMonth = QDate()) override;

private:
    MonthViewPrivate *const d;
    friend class MonthViewPrivate;
    friend class MonthScene;
};
}


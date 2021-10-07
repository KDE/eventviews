/*
  SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"

#include <Akonadi/Item>

#include <QDateTime>

#include <memory>

namespace EventViews
{
/**
  This class provides a view showing which blocks of time are occupied by events
  in the user's calendars.
*/
class EVENTVIEWS_EXPORT TimelineView : public EventView
{
    Q_OBJECT
public:

    /**
     * Create a TimelineView.
     * @param preferences Preferences object for user-configurable aspects of the view.
     */
    explicit TimelineView(const EventViews::PrefsPtr &preferences, QWidget *parent = nullptr);

    /**
     * @deprecated Use TimelineView(const EventViews::PrefsPtr &preferences, QWidget *parent = nullptr)
     */
    explicit TimelineView(QWidget *parent = nullptr);

    ~TimelineView() override;

    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override;
    Q_REQUIRED_RESULT int currentDateCount() const override;

    // ensure start and end are valid before calling this.
    void showDates(const QDate &, const QDate &, const QDate &preferredMonth = QDate()) override;

    // Unused.
    /** @deprecated Use EventView::startDateTime. */
    [[deprecated("Use EventView::startDateTime.")]]
    Q_REQUIRED_RESULT QDate startDate() const;

    // Unused.
    /** @deprecated Use EventView::endDateTime.   */
    [[deprecated("Use EventView::endDateTime.")]]
    Q_REQUIRED_RESULT QDate endDate() const;

    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;
    void updateView() override;
    virtual void changeIncidenceDisplay(const Akonadi::Item &incidence, int mode);
    Q_REQUIRED_RESULT bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const override;

Q_SIGNALS:
    void showNewEventPopupSignal();
    void showIncidencePopupSignal(const Akonadi::Item &, const QDate &);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> const d;
};
} // namespace EventViews


/*
  SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"

#include <Item>

#include <QDateTime>

namespace EventViews
{
/**
  This class provides a view ....
*/
class EVENTVIEWS_EXPORT TimelineView : public EventView
{
    Q_OBJECT
public:
    explicit TimelineView(QWidget *parent = nullptr);
    ~TimelineView() override;

    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override;
    Q_REQUIRED_RESULT int currentDateCount() const override;

    // ensure start and end are valid before calling this.
    void showDates(const QDate &, const QDate &, const QDate &preferredMonth = QDate()) override;

    // FIXME: we already have startDateTime() in the base class
    // why aren't we using it.
    Q_REQUIRED_RESULT QDate startDate() const;
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
    Private *const d;
};
} // namespace EventViews


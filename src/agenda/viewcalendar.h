/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#ifndef EVENTVIEWS_VIEWCALENDAR_H
#define EVENTVIEWS_VIEWCALENDAR_H

#include "eventviews_export.h"

#include <AkonadiCore/Item>
#include <Akonadi/Calendar/ETMCalendar>
#include <KCalendarCore/Incidence>

#include <QColor>
#include <QList>

namespace EventViews {
class AgendaView;

class EVENTVIEWS_EXPORT ViewCalendar
{
public:
    typedef QSharedPointer<ViewCalendar> Ptr;

    virtual ~ViewCalendar();
    virtual bool isValid(const KCalendarCore::Incidence::Ptr &incidence) const = 0;
    virtual bool isValid(const QString &incidenceIdentifier) const = 0;
    virtual QString displayName(const KCalendarCore::Incidence::Ptr &incidence) const = 0;

    virtual QColor resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const = 0;
    virtual QString iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const = 0;

    virtual KCalendarCore::Calendar::Ptr getCalendar() const = 0;
};

class AkonadiViewCalendar : public ViewCalendar
{
public:
    typedef QSharedPointer<AkonadiViewCalendar> Ptr;

    ~AkonadiViewCalendar() override;
    bool isValid(const KCalendarCore::Incidence::Ptr &incidence) const override;
    bool isValid(const QString &incidenceIdentifier) const override;
    QString displayName(const KCalendarCore::Incidence::Ptr &incidence) const override;

    QColor resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const override;
    QString iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const override;

    Akonadi::Item item(const KCalendarCore::Incidence::Ptr &incidence) const;

    KCalendarCore::Calendar::Ptr getCalendar() const override;

    Akonadi::ETMCalendar::Ptr mCalendar;
    AgendaView *mAgendaView = nullptr;
};

class MultiViewCalendar : public ViewCalendar
{
public:
    typedef QSharedPointer<MultiViewCalendar> Ptr;

    ~MultiViewCalendar() override;
    ViewCalendar::Ptr findCalendar(const KCalendarCore::Incidence::Ptr &incidence) const;
    ViewCalendar::Ptr findCalendar(const QString &incidenceIdentifier) const;
    Q_REQUIRED_RESULT bool isValid(const KCalendarCore::Incidence::Ptr &incidence) const override;
    Q_REQUIRED_RESULT bool isValid(const QString &incidenceIdentifier) const override;
    Q_REQUIRED_RESULT QString displayName(const KCalendarCore::Incidence::Ptr &incidence) const override;

    Q_REQUIRED_RESULT QColor resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const override;
    Q_REQUIRED_RESULT QString iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const override;

    Q_REQUIRED_RESULT Akonadi::Item item(const KCalendarCore::Incidence::Ptr &incidence) const;

    void addCalendar(const ViewCalendar::Ptr &calendar);
    void setETMCalendar(const Akonadi::ETMCalendar::Ptr &calendar);
    int calendars() const;
    Q_REQUIRED_RESULT KCalendarCore::Calendar::Ptr getCalendar() const override;
    Q_REQUIRED_RESULT KCalendarCore::Incidence::List incidences() const;

    AgendaView *mAgendaView = nullptr;
    AkonadiViewCalendar::Ptr mETMCalendar;
    QList<ViewCalendar::Ptr> mSubCalendars;
};
}

#endif

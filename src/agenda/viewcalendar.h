/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#pragma once

#include "eventviews_export.h"

#include <Akonadi/CollectionCalendar>
#include <Akonadi/Item>
#include <KCalendarCore/Incidence>

#include <QColor>
#include <QList>

namespace EventViews
{
class AgendaView;

class EVENTVIEWS_EXPORT ViewCalendar
{
public:
    using Ptr = QSharedPointer<ViewCalendar>;

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
    using Ptr = QSharedPointer<AkonadiViewCalendar>;

    ~AkonadiViewCalendar() override;
    bool isValid(const KCalendarCore::Incidence::Ptr &incidence) const override;
    bool isValid(const QString &incidenceIdentifier) const override;
    QString displayName(const KCalendarCore::Incidence::Ptr &incidence) const override;

    QColor resourceColor(const KCalendarCore::Incidence::Ptr &incidence) const override;
    QString iconForIncidence(const KCalendarCore::Incidence::Ptr &incidence) const override;

    Akonadi::Item item(const KCalendarCore::Incidence::Ptr &incidence) const;

    KCalendarCore::Calendar::Ptr getCalendar() const override;

    Akonadi::CollectionCalendar::Ptr mCalendar;
    AgendaView *mAgendaView = nullptr;
};

class MultiViewCalendar : public ViewCalendar
{
public:
    using Ptr = QSharedPointer<MultiViewCalendar>;

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
    void removeCalendar(const ViewCalendar::Ptr &calendar);
    Q_REQUIRED_RESULT int calendarCount() const;

    Q_REQUIRED_RESULT Akonadi::CollectionCalendar::Ptr calendarForCollection(const Akonadi::Collection &col) const;
    Q_REQUIRED_RESULT Akonadi::CollectionCalendar::Ptr calendarForCollection(Akonadi::Collection::Id id) const;

    Q_REQUIRED_RESULT KCalendarCore::Calendar::Ptr getCalendar() const override;
    Q_REQUIRED_RESULT KCalendarCore::Incidence::List incidences() const;

    AgendaView *mAgendaView = nullptr;
    QVector<ViewCalendar::Ptr> mSubCalendars;
};
}

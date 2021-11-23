/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"

#include <Akonadi/Calendar/IncidenceChanger>

#include <KIconLoader>

#include <QTextBrowser>
#include <QUrl>

namespace EventViews
{
class WhatsNextTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit WhatsNextTextBrowser(QWidget *parent)
        : QTextBrowser(parent)
    {
    }

    /** Reimplemented from QTextBrowser to handle links. */
    void setSource(const QUrl &name) override;

Q_SIGNALS:
    void showIncidence(const QString &uid);
};

/**
  This class provides a view of the next events and todos
*/
class EVENTVIEWS_EXPORT WhatsNextView : public EventViews::EventView
{
    Q_OBJECT
public:
    explicit WhatsNextView(QWidget *parent = nullptr);
    ~WhatsNextView() override;

    Q_REQUIRED_RESULT int currentDateCount() const override;
    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override
    {
        return {};
    }

    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override
    {
        return {};
    }

    Q_REQUIRED_RESULT bool supportsDateNavigation() const
    {
        return true;
    }

public Q_SLOTS:
    void updateView() override;
    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth) override;
    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;

    void changeIncidenceDisplay(const Akonadi::Item &, Akonadi::IncidenceChanger::ChangeType);

protected:
    void appendEvent(const KCalendarCore::Incidence::Ptr &, const QDateTime &start = QDateTime(), const QDateTime &end = QDateTime());
    void appendTodo(const KCalendarCore::Incidence::Ptr &);

private Q_SLOTS:
    void showIncidence(const QString &);

private:
    void createTaskRow(KIconLoader *kil);
    WhatsNextTextBrowser *mView = nullptr;
    QString mText;
    QDate mStartDate;
    QDate mEndDate;

    Akonadi::Item::List mTodos;
};
}


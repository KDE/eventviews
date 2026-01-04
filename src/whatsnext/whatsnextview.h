/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"

#include <Akonadi/IncidenceChanger>

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

    /*! Reimplemented from QTextBrowser to handle links. */
    void doSetSource(const QUrl &name, QTextDocument::ResourceType type = QTextDocument::UnknownResource) override;

Q_SIGNALS:
    void showIncidence(const QUrl &url);
};

/*!
  \class EventViews::WhatsNextView
  \inmodule EventViews
  \inheaderfile EventViews/WhatsNextView

  This class provides a view of the next events and todos
*/
class EVENTVIEWS_EXPORT WhatsNextView : public EventViews::EventView
{
    Q_OBJECT
public:
    /*!
     */
    explicit WhatsNextView(QWidget *parent = nullptr);
    /*!
     */
    ~WhatsNextView() override;

    /*!
     */
    [[nodiscard]] int currentDateCount() const override;
    /*!
     */
    [[nodiscard]] Akonadi::Item::List selectedIncidences() const override
    {
        return {};
    }

    /*!
     */
    [[nodiscard]] KCalendarCore::DateList selectedIncidenceDates() const override
    {
        return {};
    }

    /*!
     */
    [[nodiscard]] bool supportsDateNavigation() const
    {
        return true;
    }

public Q_SLOTS:
    /*!
     */
    void updateView() override;
    /*!
     */
    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth) override;
    /*!
     */
    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;

    /*!
     */
    void changeIncidenceDisplay(const Akonadi::Item &, Akonadi::IncidenceChanger::ChangeType);

protected:
    /*!
     */
    void appendEvent(const Akonadi::CollectionCalendar::Ptr &,
                     const KCalendarCore::Incidence::Ptr &,
                     const QDateTime &start = QDateTime(),
                     const QDateTime &end = QDateTime());
    /*!
     */
    void appendTodo(const Akonadi::CollectionCalendar::Ptr &, const KCalendarCore::Incidence::Ptr &);

private Q_SLOTS:
    EVENTVIEWS_NO_EXPORT void showIncidence(const QUrl &);

private:
    EVENTVIEWS_NO_EXPORT void createTaskRow(KIconLoader *kil);
    WhatsNextTextBrowser *const mView;
    QString mText;
    QDate mStartDate;
    QDate mEndDate;

    Akonadi::Item::List mTodos;
};
}

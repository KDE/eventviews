/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"
#include <Akonadi/IncidenceChanger>
#include <KCalendarCore/Journal>

class QScrollArea;

namespace EventViews
{
class JournalDateView;

/**
 * This class provides a journal view.

 * @short View for Journal components.
 * @author Cornelius Schumacher <schumacher@kde.org>, Reinhold Kainhofer <reinhold@kainhofer.com>
 * @see EventView
 */
class EVENTVIEWS_EXPORT JournalView : public EventView
{
    Q_OBJECT
public:
    explicit JournalView(QWidget *parent = nullptr);
    ~JournalView() override;

    Q_REQUIRED_RESULT int currentDateCount() const override;
    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override
    {
        return {};
    }

    void appendJournal(const Akonadi::Item &journal, QDate dt);

    /** documentation in baseview.h */
    void getHighlightMode(bool &highlightEvents, bool &highlightTodos, bool &highlightJournals);

    bool eventFilter(QObject *, QEvent *) override;

public Q_SLOTS:
    // Don't update the view when midnight passed, otherwise we'll have data loss (bug 79145)
    void dayPassed(const QDate &) override
    {
    }

    void updateView() override;
    void flushView() override;

    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth = QDate()) override;
    void showIncidences(const Akonadi::Item::List &incidences, const QDate &date) override;

    void changeIncidenceDisplay(const Akonadi::Item &incidence, Akonadi::IncidenceChanger::ChangeType);
    void setIncidenceChanger(Akonadi::IncidenceChanger *changer) override;
    void newJournal();
Q_SIGNALS:
    void flushEntries();
    void setIncidenceChangerSignal(Akonadi::IncidenceChanger *);
    void journalEdited(const Akonadi::Item &journal);
    void journalDeleted(const Akonadi::Item &journal);
    void printJournal(const KCalendarCore::Journal::Ptr &, bool preview);

protected:
    void clearEntries();

private:
    QScrollArea *mSA = nullptr;
    QWidget *mCurrentWidget = nullptr;
    QMap<QDate, EventViews::JournalDateView *> mEntries;
    Akonadi::IncidenceChanger *mChanger = nullptr;
    //    DateList mSelectedDates;  // List of dates to be displayed
};
}

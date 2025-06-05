/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

// View of Journal entries

#include "journalview.h"
#include "journalframe.h"

#include <Akonadi/CalendarUtils>
#include <CalendarSupport/Utils>

#include "calendarview_debug.h"
#include <QEvent>
#include <QScrollArea>
#include <QVBoxLayout>

using namespace EventViews;

JournalView::JournalView(QWidget *parent)
    : EventView(parent)
{
    auto topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins({});
    mSA = new QScrollArea(this);
    mCurrentWidget = new QWidget(mSA->viewport());
    auto mVBoxVBoxLayout = new QVBoxLayout(mCurrentWidget);
    mVBoxVBoxLayout->setContentsMargins({});
    mSA->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mSA->setWidgetResizable(true);
    mSA->setWidget(mCurrentWidget);
    topLayout->addWidget(mSA);

    installEventFilter(this);
}

JournalView::~JournalView() = default;

void JournalView::appendJournal(const Akonadi::Item &journal, const Akonadi::CollectionCalendar::Ptr &calendar, QDate dt)
{
    JournalDateView *entry = nullptr;
    if (mEntries.contains(dt)) {
        entry = mEntries[dt];
    } else {
        entry = new JournalDateView(calendar, mCurrentWidget);
        mCurrentWidget->layout()->addWidget(entry);
        entry->setDate(dt);
        entry->setIncidenceChanger(mChanger);
        entry->show();
        connect(this, &JournalView::flushEntries, entry, &JournalDateView::flushEntries);
        connect(this, &JournalView::setIncidenceChangerSignal, entry, &JournalDateView::setIncidenceChanger);
        connect(this, &JournalView::journalEdited, entry, &JournalDateView::journalEdited);
        connect(this, &JournalView::journalDeleted, entry, &JournalDateView::journalDeleted);

        connect(entry, &JournalDateView::editIncidence, this, &EventView::editIncidenceSignal);
        connect(entry, &JournalDateView::deleteIncidence, this, &EventView::deleteIncidenceSignal);
        connect(entry, &JournalDateView::newJournal, this, &EventView::newJournalSignal);
        connect(entry, &JournalDateView::incidenceSelected, this, &EventView::incidenceSelected);
        connect(entry, &JournalDateView::printJournal, this, &JournalView::printJournal);
        mEntries.insert(dt, entry);
    }

    if (entry && CalendarSupport::hasJournal(journal)) {
        entry->addJournal(journal);
    }
}

int JournalView::currentDateCount() const
{
    return mEntries.size();
}

Akonadi::Item::List JournalView::selectedIncidences() const
{
    // We don't have a selection in the journal view.
    // FIXME: The currently edited journal is the selected incidence...
    Akonadi::Item::List eventList;
    return eventList;
}

void JournalView::clearEntries()
{
    // qDebug() << "JournalView::clearEntries()";
    QMap<QDate, JournalDateView *>::Iterator it;
    for (it = mEntries.begin(); it != mEntries.end(); ++it) {
        delete it.value();
    }
    mEntries.clear();
}

void JournalView::updateView()
{
    QMap<QDate, JournalDateView *>::Iterator it = mEntries.end();
    const auto cals = calendars();
    while (it != mEntries.begin()) {
        --it;
        it.value()->clear();
        for (const auto &calendar : cals) {
            const auto journals = calendar->journals(it.key());
            for (const auto &journal : journals) {
                it.value()->addJournal(calendar->item(journal));
            }
        }
    }
}

void JournalView::flushView()
{
    Q_EMIT flushEntries();
}

void JournalView::showDates(const QDate &start, const QDate &end, const QDate &)
{
    clearEntries();
    if (end < start) {
        qCWarning(CALENDARVIEW_LOG) << "End is smaller than start. end=" << end << "; start=" << start;
        return;
    }

    const auto cals = calendars();
    for (QDate d = end; d >= start; d = d.addDays(-1)) {
        for (const auto &calendar : cals) {
            const KCalendarCore::Journal::List jnls = calendar->journals(d);
            // qCDebug(CALENDARVIEW_LOG) << "Found" << jnls.count() << "journals on date" << d;
            for (const KCalendarCore::Journal::Ptr &journal : jnls) {
                Akonadi::Item const item = calendar->item(journal);
                appendJournal(item, calendar, d);
            }
            if (jnls.isEmpty()) {
                // create an empty dateentry widget
                // updateView();
                // qCDebug(CALENDARVIEW_LOG) << "Appended null journal";
                appendJournal(Akonadi::Item(), calendar, d);
            }
        }
    }
}

void JournalView::showIncidences(const Akonadi::Item::List &incidences, const QDate &date)
{
    Q_UNUSED(date)
    clearEntries();
    for (const Akonadi::Item &i : incidences) {
        if (const KCalendarCore::Journal::Ptr j = Akonadi::CalendarUtils::journal(i)) {
            appendJournal(i, calendar3(j), j->dtStart().date());
        }
    }
}

void JournalView::changeIncidenceDisplay(const Akonadi::Item &incidence, Akonadi::IncidenceChanger::ChangeType changeType)
{
    if (KCalendarCore::Journal::Ptr const journal = Akonadi::CalendarUtils::journal(incidence)) {
        switch (changeType) {
        case Akonadi::IncidenceChanger::ChangeTypeCreate:
            appendJournal(incidence, calendar3(incidence), journal->dtStart().date());
            break;
        case Akonadi::IncidenceChanger::ChangeTypeModify:
            Q_EMIT journalEdited(incidence);
            break;
        case Akonadi::IncidenceChanger::ChangeTypeDelete:
            Q_EMIT journalDeleted(incidence);
            break;
        default:
            qCWarning(CALENDARVIEW_LOG) << "Illegal change type" << changeType;
        }
    }
}

void JournalView::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    mChanger = changer;
    Q_EMIT setIncidenceChangerSignal(changer);
}

void JournalView::newJournal()
{
    Q_EMIT newJournalSignal(QDate::currentDate());
}

bool JournalView::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object)
    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
        Q_EMIT newJournalSignal(QDate());
        return true;
    default:
        return false;
    }
}

#include "moc_journalview.cpp"

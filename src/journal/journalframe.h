/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2007 Mike McQuaid <mike@mikemcquaid.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#pragma once

#include <Akonadi/CollectionCalendar>
#include <Akonadi/IncidenceChanger>
#include <Akonadi/Item>

#include <QDate>
#include <QFrame>

class QTextBrowser;
class QPushButton;

namespace EventViews
{
class JournalFrame : public QFrame
{
    Q_OBJECT
public:
    using List = QList<JournalFrame *>;

    JournalFrame(const Akonadi::Item &journal, const Akonadi::CollectionCalendar::Ptr &calendar, QWidget *parent);

    ~JournalFrame() override;
    bool eventFilter(QObject *, QEvent *) override;

    void setJournal(const Akonadi::Item &journal);
    [[nodiscard]] Akonadi::Item journal() const
    {
        return mJournal;
    }

    void setCalendar(const Akonadi::CollectionCalendar::Ptr &);
    [[nodiscard]] QDate date() const
    {
        return mDate;
    }

    void clear();
    void readJournal(const Akonadi::Item &item);

protected Q_SLOTS:
    void setDirty();
    void deleteItem();
    void editItem();
    void printJournal();
    void printPreviewJournal();

public Q_SLOTS:
    void setIncidenceChanger(Akonadi::IncidenceChanger *changer)
    {
        mChanger = changer;
    }

    void setDate(QDate date);

Q_SIGNALS:
    void printJournalSignal(const KCalendarCore::Journal::Ptr &, bool preview);
    void deleteIncidence(const Akonadi::Item &);
    void editIncidence(const Akonadi::Item &);
    void incidenceSelected(const Akonadi::Item &, const QDate &);

private:
    Akonadi::Item mJournal;
    Akonadi::CollectionCalendar::Ptr mCalendar;
    QDate mDate;

    QTextBrowser *mBrowser = nullptr;
    QPushButton *mEditButton = nullptr;
    QPushButton *mDeleteButton = nullptr;
    QPushButton *mPrintButton = nullptr;
    QPushButton *mPrintPreviewButton = nullptr;

    bool mDirty;
    bool mWriteInProgress;
    Akonadi::IncidenceChanger *mChanger = nullptr;
};

class JournalDateView : public QWidget
{
    Q_OBJECT
public:
    using List = QList<JournalDateView *>;

    JournalDateView(const Akonadi::CollectionCalendar::Ptr &, QWidget *parent);
    ~JournalDateView() override;

    void addJournal(const Akonadi::Item &journal);
    [[nodiscard]] Akonadi::Item::List journals() const;

    void setDate(QDate date);
    [[nodiscard]] QDate date() const
    {
        return mDate;
    }

    void clear();

Q_SIGNALS:
    void setIncidenceChangerSignal(Akonadi::IncidenceChanger *changer);
    void setDateSignal(const QDate &);
    void flushEntries();
    void editIncidence(const Akonadi::Item &journal);
    void deleteIncidence(const Akonadi::Item &journal);
    void newJournal(const QDate &);
    void incidenceSelected(const Akonadi::Item &, const QDate &);
    void printJournal(const KCalendarCore::Journal::Ptr &, bool preview);

public Q_SLOTS:
    void emitNewJournal();
    void setIncidenceChanger(Akonadi::IncidenceChanger *changer);
    void journalEdited(const Akonadi::Item &);
    void journalDeleted(const Akonadi::Item &);

private:
    Akonadi::CollectionCalendar::Ptr mCalendar;
    QDate mDate;
    QMap<Akonadi::Item::Id, JournalFrame *> mEntries;

    Akonadi::IncidenceChanger *mChanger = nullptr;
};
}

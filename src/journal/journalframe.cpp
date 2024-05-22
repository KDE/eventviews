/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2007 Mike McQuaid <mike@mikemcquaid.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

// Journal Entry

#include "journalframe.h"
using namespace Qt::Literals::StringLiterals;

#include <Akonadi/CalendarUtils>
#include <CalendarSupport/Utils>

#include <KCalendarCore/Journal>

#include <KCalUtils/IncidenceFormatter>

#include "calendarview_debug.h"
#include <KLocalizedString>
#include <QTextBrowser>

#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QPushButton>

using namespace EventViews;

JournalDateView::JournalDateView(const Akonadi::CollectionCalendar::Ptr &calendar, QWidget *parent)
    : QWidget(parent)
    , mCalendar(calendar)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
}

JournalDateView::~JournalDateView() = default;

void JournalDateView::setDate(QDate date)
{
    mDate = date;
    Q_EMIT setDateSignal(date);
}

void JournalDateView::clear()
{
    qDeleteAll(mEntries);
    mEntries.clear();
}

// should only be called by the JournalView now.
void JournalDateView::addJournal(const Akonadi::Item &j)
{
    QMap<Akonadi::Item::Id, JournalFrame *>::Iterator pos = mEntries.find(j.id());
    if (pos != mEntries.end()) {
        return;
    }

    auto container = new QWidget(this);
    layout()->addWidget(container);
    auto layout = new QHBoxLayout(container);
    layout->addStretch(1);
    auto entry = new JournalFrame(j, mCalendar, this);
    layout->addWidget(entry, 3 /*stretch*/);
    layout->addStretch(1);

    entry->show();
    entry->setDate(mDate);
    entry->setIncidenceChanger(mChanger);

    mEntries.insert(j.id(), entry);
    connect(this, &JournalDateView::setIncidenceChangerSignal, entry, &JournalFrame::setIncidenceChanger);
    connect(this, &JournalDateView::setDateSignal, entry, &JournalFrame::setDate);
    connect(entry, &JournalFrame::deleteIncidence, this, &JournalDateView::deleteIncidence);
    connect(entry, &JournalFrame::editIncidence, this, &JournalDateView::editIncidence);
    connect(entry, &JournalFrame::incidenceSelected, this, &JournalDateView::incidenceSelected);
    connect(entry,
            qOverload<const KCalendarCore::Journal::Ptr &, bool>(&JournalFrame::printJournal),
            this,
            qOverload<const KCalendarCore::Journal::Ptr &, bool>(&JournalDateView::printJournal));
}

Akonadi::Item::List JournalDateView::journals() const
{
    Akonadi::Item::List l;
    l.reserve(mEntries.count());
    for (const JournalFrame *const i : std::as_const(mEntries)) {
        l.push_back(i->journal());
    }
    return l;
}

void JournalDateView::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    mChanger = changer;
    Q_EMIT setIncidenceChangerSignal(changer);
}

void JournalDateView::emitNewJournal()
{
    Q_EMIT newJournal(mDate);
}

void JournalDateView::journalEdited(const Akonadi::Item &journal)
{
    QMap<Akonadi::Item::Id, JournalFrame *>::Iterator pos = mEntries.find(journal.id());
    if (pos == mEntries.end()) {
        return;
    }

    pos.value()->setJournal(journal);
}

void JournalDateView::journalDeleted(const Akonadi::Item &journal)
{
    QMap<Akonadi::Item::Id, JournalFrame *>::Iterator pos = mEntries.find(journal.id());
    if (pos == mEntries.end()) {
        return;
    }

    delete pos.value();
    mEntries.remove(journal.id());
}

JournalFrame::JournalFrame(const Akonadi::Item &j, const Akonadi::CollectionCalendar::Ptr &calendar, QWidget *parent)
    : QFrame(parent)
    , mJournal(j)
    , mCalendar(calendar)
{
    mDirty = false;
    mWriteInProgress = false;
    mChanger = nullptr;

    auto verticalLayout = new QVBoxLayout(this);

    mBrowser = new QTextBrowser(this);
    mBrowser->viewport()->installEventFilter(this);
    mBrowser->setFrameStyle(QFrame::NoFrame);
    verticalLayout->addWidget(mBrowser);

    auto buttonsLayout = new QHBoxLayout();
    verticalLayout->addLayout(buttonsLayout);
    buttonsLayout->addStretch();

    mEditButton = new QPushButton(this);
    mEditButton->setObjectName("editButton"_L1);
    mEditButton->setText(i18n("&Edit"));
    mEditButton->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    mEditButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mEditButton->setToolTip(i18nc("@info:tooltip", "Edit this journal entry"));
    mEditButton->setWhatsThis(i18n("Opens an editor dialog for this journal entry"));
    buttonsLayout->addWidget(mEditButton);
    connect(mEditButton, &QPushButton::clicked, this, &JournalFrame::editItem);

    mDeleteButton = new QPushButton(this);
    mDeleteButton->setObjectName("deleteButton"_L1);
    mDeleteButton->setText(i18n("&Delete"));
    mDeleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    mDeleteButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mDeleteButton->setToolTip(i18nc("@info:tooltip", "Delete this journal entry"));
    mDeleteButton->setWhatsThis(i18n("Delete this journal entry"));
    buttonsLayout->addWidget(mDeleteButton);
    connect(mDeleteButton, &QPushButton::pressed, this, &JournalFrame::deleteItem);

    mPrintButton = new QPushButton(this);
    mPrintButton->setText(i18n("&Print"));
    mPrintButton->setObjectName("printButton"_L1);
    mPrintButton->setIcon(QIcon::fromTheme(QStringLiteral("document-print")));
    mPrintButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mPrintButton->setToolTip(i18nc("@info:tooltip", "Print this journal entry"));
    mPrintButton->setWhatsThis(i18n("Opens a print dialog for this journal entry"));
    buttonsLayout->addWidget(mPrintButton);
    connect(mPrintButton, &QPushButton::clicked, this, qOverload<>(&JournalFrame::printJournal));

    mPrintPreviewButton = new QPushButton(this);
    mPrintPreviewButton->setText(i18n("Print preview"));
    mPrintPreviewButton->setObjectName("printButton"_L1);
    mPrintPreviewButton->setIcon(QIcon::fromTheme(QStringLiteral("document-print-preview")));
    mPrintPreviewButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mPrintPreviewButton->setToolTip(i18nc("@info:tooltip", "Print preview this journal entry"));
    buttonsLayout->addWidget(mPrintPreviewButton);
    connect(mPrintPreviewButton, &QAbstractButton::clicked, this, &JournalFrame::printPreviewJournal);

    readJournal(mJournal);
    mDirty = false;
    setFrameStyle(QFrame::Box);
    // These probably shouldn't be hardcoded
    setStyleSheet(QStringLiteral("QFrame { border: 1px solid; border-radius: 7px; } "));
    mBrowser->setStyleSheet(QStringLiteral("QFrame { border: 0px solid white } "));
}

JournalFrame::~JournalFrame() = default;

bool JournalFrame::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object)

    // object is our QTextBrowser
    if (!mJournal.isValid()) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        Q_EMIT incidenceSelected(mJournal, mDate);
        break;
    case QEvent::MouseButtonDblClick:
        Q_EMIT editIncidence(mJournal);
        break;
    default:
        break;
    }

    return false;
}

void JournalFrame::deleteItem()
{
    if (CalendarSupport::hasJournal(mJournal)) {
        Q_EMIT deleteIncidence(mJournal);
    }
}

void JournalFrame::editItem()
{
    if (CalendarSupport::hasJournal(mJournal)) {
        Q_EMIT editIncidence(mJournal);
    }
}

void JournalFrame::setCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    mCalendar = calendar;
}

void JournalFrame::setDate(QDate date)
{
    mDate = date;
}

void JournalFrame::setJournal(const Akonadi::Item &journal)
{
    if (!CalendarSupport::hasJournal(journal)) {
        return;
    }

    mJournal = journal;
    readJournal(journal);

    mDirty = false;
}

void JournalFrame::setDirty()
{
    mDirty = true;
    qCDebug(CALENDARVIEW_LOG);
}

void JournalFrame::printJournal()
{
    Q_EMIT printJournal(Akonadi::CalendarUtils::journal(mJournal), false);
}

void JournalFrame::printPreviewJournal()
{
    Q_EMIT printJournal(Akonadi::CalendarUtils::journal(mJournal), true);
}

void JournalFrame::readJournal(const Akonadi::Item &j)
{
    int baseFontSize = QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize();
    mJournal = j;
    const KCalendarCore::Journal::Ptr journal = Akonadi::CalendarUtils::journal(j);
    mBrowser->clear();
    QTextCursor cursor = QTextCursor(mBrowser->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextBlockFormat bodyBlock = QTextBlockFormat(cursor.blockFormat());
    // FIXME: Do padding
    bodyBlock.setTextIndent(2);
    QTextCharFormat bodyFormat = QTextCharFormat(cursor.charFormat());
    if (!journal->summary().isEmpty()) {
        QTextCharFormat titleFormat = bodyFormat;
        titleFormat.setFontWeight(QFont::Bold);
        titleFormat.setFontPointSize(baseFontSize + 4);
        cursor.insertText(journal->summary(), titleFormat);
        cursor.insertBlock();
    }
    QTextCharFormat dateFormat = bodyFormat;
    dateFormat.setFontWeight(QFont::Bold);
    dateFormat.setFontPointSize(baseFontSize + 1);
    cursor.insertText(KCalUtils::IncidenceFormatter::dateTimeToString(journal->dtStart(), journal->allDay()), dateFormat);
    cursor.insertBlock();
    cursor.insertBlock();
    cursor.setBlockCharFormat(bodyFormat);
    const QString description = journal->description();
    if (journal->descriptionIsRich()) {
        mBrowser->insertHtml(description);
    } else {
        mBrowser->insertPlainText(description);
    }
    cursor.movePosition(QTextCursor::Start);
    mBrowser->setTextCursor(cursor);
    mBrowser->ensureCursorVisible();

    if (mCalendar) {
        mEditButton->setEnabled(mCalendar->hasRight(Akonadi::Collection::CanChangeItem));
        mDeleteButton->setEnabled(mCalendar->hasRight(Akonadi::Collection::CanDeleteItem));
    }
}

#include "moc_journalframe.cpp"

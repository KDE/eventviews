/*
  This file is part of the kcal library.

  SPDX-FileCopyrightText: Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

  SPDX-FileCopyrightText: 2015 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "recurrenceactions.h"

#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>

#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QPushButton>

using namespace EventViews;
using namespace KCalendarCore;

int RecurrenceActions::availableOccurrences(const Incidence::Ptr &incidence, const QDateTime &selectedOccurrence)
{
    int result = NoOccurrence;

    if (incidence->recurrence()->recursOn(selectedOccurrence.date(), selectedOccurrence.timeZone())) {
        result |= SelectedOccurrence;
    }

    if (incidence->recurrence()->getPreviousDateTime(selectedOccurrence).isValid()) {
        result |= PastOccurrences;
    }

    if (incidence->recurrence()->getNextDateTime(selectedOccurrence).isValid()) {
        result |= FutureOccurrences;
    }

    return result;
}

int RecurrenceActions::questionSelectedAllCancel(const QString &message,
                                                 const QString &caption,
                                                 const KGuiItem &actionSelected,
                                                 const KGuiItem &actionAll,
                                                 QWidget *parent)
{
    QPointer<QDialog> const dialog = new QDialog(parent);
    dialog->setWindowTitle(caption);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::No | QDialogButtonBox::Yes, parent);
    dialog->setObjectName(QLatin1StringView("RecurrenceActions::questionSelectedAllCancel"));

    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), actionSelected);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Ok), actionAll);

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    bool checkboxResult = false;
    int const result =
        KMessageBox::createKMessageBox(dialog, buttonBox, QMessageBox::Question, message, QStringList(), QString(), &checkboxResult, KMessageBox::Notify);

    switch (result) {
    case QDialogButtonBox::Yes:
        return SelectedOccurrence;
    case QDialogButtonBox::Ok:
        return AllOccurrences;
    default:
        return NoOccurrence;
    }
}

int RecurrenceActions::questionSelectedFutureAllCancel(const QString &message,
                                                       const QString &caption,
                                                       const KGuiItem &actionSelected,
                                                       const KGuiItem &actionFuture,
                                                       const KGuiItem &actionAll,
                                                       QWidget *parent)
{
    QPointer<QDialog> const dialog = new QDialog(parent);
    dialog->setWindowTitle(caption);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::No | QDialogButtonBox::Yes, parent);
    dialog->setObjectName(QLatin1StringView("RecurrenceActions::questionSelectedFutureAllCancel"));
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), actionSelected);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), actionFuture);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Ok), actionAll);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    bool checkboxResult = false;
    QDialogButtonBox::StandardButton const result =
        KMessageBox::createKMessageBox(dialog, buttonBox, QMessageBox::Question, message, QStringList(), QString(), &checkboxResult, KMessageBox::Notify);

    switch (result) {
    case QDialogButtonBox::Yes:
        return SelectedOccurrence;
    case QDialogButtonBox::No:
        return FutureOccurrences;
    case QDialogButtonBox::Ok:
        return AllOccurrences;
    default:
        return NoOccurrence;
    }

    return NoOccurrence;
}

#include "moc_recurrenceactions.cpp"

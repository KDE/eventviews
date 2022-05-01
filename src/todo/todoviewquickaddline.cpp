/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "todoviewquickaddline.h"

#include <KLocalizedString>

#include <QKeyEvent>

TodoViewQuickAddLine::TodoViewQuickAddLine(QWidget *parent)
    : KLineEdit(parent)
{
    connect(this, &TodoViewQuickAddLine::returnPressed, this, &TodoViewQuickAddLine::returnPressedSlot);

    mClickMessage = i18n("Enter a summary to create a new to-do");
    setToolTip(mClickMessage);
}

void TodoViewQuickAddLine::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return) {
        mModifiers = event->modifiers();
    }

    KLineEdit::keyPressEvent(event);
}

void TodoViewQuickAddLine::returnPressedSlot()
{
    // Workaround bug #217592 (disappearing cursor)
    unsetCursor();

    Q_EMIT returnPressed(mModifiers);
}

void TodoViewQuickAddLine::resizeEvent(QResizeEvent *event)
{
    KLineEdit::resizeEvent(event);

    setPlaceholderText(fontMetrics().elidedText(mClickMessage, Qt::ElideRight, width() - clearButtonUsedSize().width()));
}

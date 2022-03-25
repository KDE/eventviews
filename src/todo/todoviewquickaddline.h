/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include <KLineEdit>

class TodoViewQuickAddLine : public KLineEdit
{
    Q_OBJECT

public:
    explicit TodoViewQuickAddLine(QWidget *parent);
    ~TodoViewQuickAddLine() override = default;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

Q_SIGNALS:
    void returnPressed(Qt::KeyboardModifiers modifiers);

private Q_SLOTS:
    void returnPressedSlot();

private:
    Qt::KeyboardModifiers mModifiers;
    QString mClickMessage;
};

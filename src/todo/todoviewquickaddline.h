/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
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
    void returnPressedSignal(Qt::KeyboardModifiers modifiers);

private:
    void returnPressedSlot();
    Qt::KeyboardModifiers mModifiers;
    const QString mClickMessage;
};

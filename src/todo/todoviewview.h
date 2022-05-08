/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include <QTimer>
#include <QTreeView>

class QMenu;

namespace CalendarSupport
{
    class KDatePickerPopup;
}

class TodoViewView : public QTreeView
{
    Q_OBJECT

public:
    explicit TodoViewView(QWidget *parent = nullptr);

    Q_REQUIRED_RESULT bool isEditing(const QModelIndex &index) const;

    Q_REQUIRED_RESULT bool eventFilter(QObject *watched, QEvent *event) override;
    Q_REQUIRED_RESULT CalendarSupport::KDatePickerPopup *startPopupMenu();

protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    QModelIndex getNextEditableIndex(const QModelIndex &cur, int inc);

    QMenu *mHeaderPopup = nullptr;
    QList<QAction *> mColumnActions;
    QTimer mExpandTimer;
    bool mIgnoreNextMouseRelease = false;

    // TODO KF6: move this next to TodoView::mMovePopupMenu.
    CalendarSupport::KDatePickerPopup *mStartPopupMenu = nullptr;

Q_SIGNALS:
    void visibleColumnCountChanged();

private Q_SLOTS:
    void toggleColumnHidden(QAction *action);
    void expandParent();
};


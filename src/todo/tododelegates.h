/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#pragma once

#include <QStyledItemDelegate>

class QPainter;
class QSize;
class QStyleOptionViewItem;
class QTextDocument;

/**
  This delegate is responsible for displaying progress bars for the completion
  status of indivitual todos. It also provides a slider to change the completion
  status of the todo when in editing mode.

  @author Thomas Thrainer
*/
class TodoCompleteDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TodoCompleteDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void initStyleOptionProgressBar(QStyleOptionProgressBar *option, const QModelIndex &index) const;
};

class TodoCompleteSlider : public QSlider
{
    Q_OBJECT

public:
    explicit TodoCompleteSlider(QWidget *parent);

private Q_SLOTS:
    void updateTip(int value);
};

/**
  This delegate is responsible for displaying the priority of todos.
  It also provides a combo box to change the priority of the todo
  when in editing mode.

  @author Thomas Thrainer
 */
class TodoPriorityDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TodoPriorityDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

/**
  This delegate is responsible for displaying the due date of todos.
  It also provides a combo box to change the due date of the todo
  when in editing mode.

  @author Thomas Thrainer
 */
class TodoDueDateDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TodoDueDateDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

/**
  This delegate is responsible for displaying the categories of todos.
  It also provides a combo box to change the categories of the todo
  when in editing mode.

  @author Thomas Thrainer
 */
class TodoCategoriesDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TodoCategoriesDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

/**
  This delegate is responsible for displaying possible rich text elements
  of a todo. That's the summary and the description.

  @author Thomas Thrainer
 */
class TodoRichTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TodoRichTextDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QTextDocument *m_textDoc = nullptr;
};

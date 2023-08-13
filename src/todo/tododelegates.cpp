/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "tododelegates.h"
#include "todoviewview.h"

#include <Akonadi/TagSelectionComboBox>
#include <Akonadi/TodoModel>
#include <CalendarSupport/CategoryHierarchyReader>

#include <KCalendarCore/CalFilter>

#include <KDateComboBox>

#include <QApplication>
#include <QPainter>
#include <QTextDocument>
#include <QToolTip>

// ---------------- COMPLETION DELEGATE --------------------------
// ---------------------------------------------------------------

TodoCompleteDelegate::TodoCompleteDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TodoCompleteDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);

    if (index.data(Qt::EditRole).toInt() > 0) {
        bool isEditing = false;
        auto view = qobject_cast<TodoViewView *>(parent());
        if (view) {
            isEditing = view->isEditing(index);
        }

        // TODO QTreeView does not set State_Editing. Qt task id 205051
        // should be fixed with Qt 4.5, but wasn't. According to the
        // task tracker the fix arrives in "Some future release".
        if (!(opt.state & QStyle::State_Editing) && !isEditing) {
            QStyleOptionProgressBar pbOption;
            pbOption.QStyleOption::operator=(option);
            initStyleOptionProgressBar(&pbOption, index);

            style->drawControl(QStyle::CE_ProgressBar, &pbOption, painter);
        }
    }
}

QSize TodoCompleteDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    QStyleOptionProgressBar pbOption;
    pbOption.QStyleOption::operator=(option);
    initStyleOptionProgressBar(&pbOption, index);

    return style->sizeFromContents(QStyle::CT_ProgressBar, &pbOption, QSize(), opt.widget);
}

void TodoCompleteDelegate::initStyleOptionProgressBar(QStyleOptionProgressBar *option, const QModelIndex &index) const
{
    option->rect.adjust(0, 1, 0, -1);
    option->maximum = 100;
    option->minimum = 0;
    option->progress = index.data().toInt();
    option->text = index.data().toString() + QChar::fromLatin1('%');
    option->textAlignment = Qt::AlignCenter;
    option->textVisible = true;
    option->state |= QStyle::State_Horizontal;
}

QWidget *TodoCompleteDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto slider = new TodoCompleteSlider(parent);

    slider->setRange(0, 100);
    slider->setOrientation(Qt::Horizontal);

    return slider;
}

void TodoCompleteDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto slider = static_cast<QSlider *>(editor);

    slider->setValue(index.data(Qt::EditRole).toInt());
}

void TodoCompleteDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto slider = static_cast<QSlider *>(editor);

    model->setData(index, slider->value());
}

void TodoCompleteDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

TodoCompleteSlider::TodoCompleteSlider(QWidget *parent)
    : QSlider(parent)
{
    connect(this, &TodoCompleteSlider::valueChanged, this, &TodoCompleteSlider::updateTip);
}

void TodoCompleteSlider::updateTip(int value)
{
    QPoint p;
    p.setY(height() / 2);
    p.setX(style()->sliderPositionFromValue(minimum(), maximum(), value, width()));

    const QString text = QStringLiteral("%1%").arg(value);
    QToolTip::showText(mapToGlobal(p), text, this);
}

// ---------------- PRIORITY DELEGATE ----------------------------
// ---------------------------------------------------------------

TodoPriorityDelegate::TodoPriorityDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *TodoPriorityDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto combo = new QComboBox(parent);

    combo->addItem(i18nc("@action:inmenu Unspecified priority", "unspecified"));
    combo->addItem(i18nc("@action:inmenu highest priority", "1 (highest)"));
    combo->addItem(i18nc("@action:inmenu", "2"));
    combo->addItem(i18nc("@action:inmenu", "3"));
    combo->addItem(i18nc("@action:inmenu", "4"));
    combo->addItem(i18nc("@action:inmenu medium priority", "5 (medium)"));
    combo->addItem(i18nc("@action:inmenu", "6"));
    combo->addItem(i18nc("@action:inmenu", "7"));
    combo->addItem(i18nc("@action:inmenu", "8"));
    combo->addItem(i18nc("@action:inmenu lowest priority", "9 (lowest)"));

    return combo;
}

void TodoPriorityDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto combo = static_cast<QComboBox *>(editor);

    combo->setCurrentIndex(index.data(Qt::EditRole).toInt());
}

void TodoPriorityDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto combo = static_cast<QComboBox *>(editor);

    model->setData(index, combo->currentIndex());
}

void TodoPriorityDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

// ---------------- DUE DATE DELEGATE ----------------------------
// ---------------------------------------------------------------

TodoDueDateDelegate::TodoDueDateDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *TodoDueDateDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    auto dateEdit = new KDateComboBox(parent);

    return dateEdit;
}

void TodoDueDateDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto dateEdit = static_cast<KDateComboBox *>(editor);

    dateEdit->setDate(index.data(Qt::EditRole).toDate());
}

void TodoDueDateDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto dateEdit = static_cast<KDateComboBox *>(editor);

    model->setData(index, dateEdit->date());
}

void TodoDueDateDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(QStyle::alignedRect(QApplication::layoutDirection(), Qt::AlignCenter, editor->size(), option.rect));
}

// ---------------- CATEGORIES DELEGATE --------------------------
// ---------------------------------------------------------------

TodoCategoriesDelegate::TodoCategoriesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *TodoCategoriesDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return new Akonadi::TagSelectionComboBox(parent);
}

void TodoCategoriesDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto combo = static_cast<Akonadi::TagSelectionComboBox *>(editor);
    combo->setSelection(index.data(Qt::EditRole).toStringList());
}

void TodoCategoriesDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto combo = static_cast<Akonadi::TagSelectionComboBox *>(editor);
    model->setData(index, combo->selectionNames());
}

void TodoCategoriesDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    editor->setGeometry(option.rect);
}

// ---------------- RICH TEXT DELEGATE ---------------------------
// ---------------------------------------------------------------

TodoRichTextDelegate::TodoRichTextDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    m_textDoc = new QTextDocument(this);
}

void TodoRichTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data(Akonadi::TodoModel::IsRichTextRole).toBool()) {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();

        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);

        // draw the item without text
        opt.text.clear();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

        // draw the text (rich text)
        QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active)) {
            cg = QPalette::Inactive;
        }

        if (opt.state & QStyle::State_Selected) {
            painter->setPen(QPen(opt.palette.brush(cg, QPalette::HighlightedText), 0));
        } else {
            painter->setPen(QPen(opt.palette.brush(cg, QPalette::Text), 0));
        }
        if (opt.state & QStyle::State_Editing) {
            painter->setPen(QPen(opt.palette.brush(cg, QPalette::Text), 0));
            painter->drawRect(textRect.adjusted(0, 0, -1, -1));
        }

        m_textDoc->setHtml(index.data().toString());

        painter->save();
        painter->translate(textRect.topLeft());

        QRect tmpRect = textRect;
        tmpRect.moveTo(0, 0);
        m_textDoc->setTextWidth(tmpRect.width());
        m_textDoc->drawContents(painter, tmpRect);

        painter->restore();
    } else {
        // align the text at top so that when it has more than two lines
        // it will just cut the extra lines out instead of aligning centered vertically
        QStyleOptionViewItem copy = option;
        copy.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        QStyledItemDelegate::paint(painter, copy, index);
    }
}

QSize TodoRichTextDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize ret = QStyledItemDelegate::sizeHint(option, index);
    if (index.data(Akonadi::TodoModel::IsRichTextRole).toBool()) {
        m_textDoc->setHtml(index.data().toString());
        ret = ret.expandedTo(m_textDoc->size().toSize());
    }
    // limit height to max. 2 lines
    // TODO add graphical hint when truncating! make configurable height?
    if (ret.height() > option.fontMetrics.height() * 2) {
        ret.setHeight(option.fontMetrics.height() * 2);
    }

    // This row might not have a checkbox, so give it more height so it appears the same size as other rows.
    const int checkboxHeight = QApplication::style()->sizeFromContents(QStyle::CT_CheckBox, &option, QSize()).height();
    return {ret.width(), qMax(ret.height(), checkboxHeight)};
}

#include "moc_tododelegates.cpp"

/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2004 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#pragma once

#include <QWidget>

namespace KPIM
{
class KCheckComboBox;
}

namespace Akonadi
{
class TagSelectionComboBox;
}

class QLineEdit;

class TodoViewQuickSearch : public QWidget
{
    Q_OBJECT
public:
    explicit TodoViewQuickSearch(QWidget *parent);

Q_SIGNALS:
    void searchTextChanged(const QString &);

    /**
     * The string list contains the new categories which are set on the filter.
     * All values belong to the Qt::UserRole of the combo box, not the Qt::DisplayRole,
     * so, if someone checks a subcategory, the value will be "ParentCategory:subCategory"
     * and not " subcategory".
     * */
    void filterCategoryChanged(const QStringList &);
    void filterPriorityChanged(const QStringList &);

public Q_SLOTS:
    void reset();

private:
    /** Helper method for the filling of the priority combo. */
    void fillPriorities();

    QLineEdit *const mSearchLine;
    Akonadi::TagSelectionComboBox *const mCategoryCombo;
    KPIM::KCheckComboBox *const mPriorityCombo;
};

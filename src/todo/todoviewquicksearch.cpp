/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2004 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "todoviewquicksearch.h"
#include <CalendarSupport/KCalPrefs>

#include <Akonadi/TagSelectionComboBox>

#include <Libkdepim/KCheckComboBox>

#include <CalendarSupport/CategoryHierarchyReader>

#include <KCalendarCore/CalFilter>

#include <KLocalizedString>
#include <QLineEdit>

#include <QHBoxLayout>

TodoViewQuickSearch::TodoViewQuickSearch(QWidget *parent)
    : QWidget(parent)
    , mSearchLine(new QLineEdit(this))
    , mCategoryCombo(new Akonadi::TagSelectionComboBox(this))
    , mPriorityCombo(new KPIM::KCheckComboBox(this))
{
    auto layout = new QHBoxLayout(this);
    // no special margin because it is added by the view
    layout->setContentsMargins({});

    mSearchLine->setToolTip(i18nc("@info:tooltip", "Filter on matching summaries"));
    mSearchLine->setWhatsThis(i18nc("@info:whatsthis", "Enter text here to filter the to-dos that are shown by matching summaries."));
    mSearchLine->setPlaceholderText(i18nc("@label in QuickSearchLine", "Search Summaries…"));
    mSearchLine->setClearButtonEnabled(true);
    connect(mSearchLine, &QLineEdit::textChanged, this, &TodoViewQuickSearch::searchTextChanged);

    layout->addWidget(mSearchLine, 3);

    mCategoryCombo->setCheckable(true);
    mCategoryCombo->setToolTip(i18nc("@info:tooltip", "Filter on these tags"));
    mCategoryCombo->setWhatsThis(i18nc("@info:whatsthis",
                                       "Use this combobox to filter the to-dos that are shown by "
                                       "a list of selected tags."));
    const QString defaultText = i18nc("@item:inlistbox", "Select Tags");
    mCategoryCombo->lineEdit()->setPlaceholderText(defaultText);

    connect(mCategoryCombo, &Akonadi::TagSelectionComboBox::selectionChanged, this, [this]() {
        Q_EMIT filterCategoryChanged(mCategoryCombo->selectionNames());
    });

    layout->addWidget(mCategoryCombo, 1);

    {
        // Make the combo big enough so that "Select Categories" fits.
        const QFontMetrics fm = mCategoryCombo->lineEdit()->fontMetrics();

        // QLineEdit::sizeHint() returns a nice size to fit 17 'x' chars.
        const int currentPreferedWidth = mCategoryCombo->lineEdit()->sizeHint().width();

        // Calculate a nice size for "Select Categories"
        const int newPreferedWidth = currentPreferedWidth - fm.boundingRect(u'x').width() * 17 + fm.boundingRect(defaultText).width();

        const int pixelsToAdd = newPreferedWidth - mCategoryCombo->lineEdit()->width();
        mCategoryCombo->setMinimumWidth(mCategoryCombo->width() + pixelsToAdd);
    }

    mPriorityCombo->setToolTip(i18nc("@info:tooltip", "Filter on these priorities"));
    mPriorityCombo->setWhatsThis(i18nc("@info:whatsthis",
                                       "Use this combobox to filter the to-dos that are shown by "
                                       "a list of selected priorities."));
    mPriorityCombo->lineEdit()->setPlaceholderText(i18nc("@item:inlistbox", "Select Priority"));
    connect(mPriorityCombo, &KPIM::KCheckComboBox::checkedItemsChanged, this, [this]() {
        Q_EMIT filterPriorityChanged(mPriorityCombo->checkedItems(Qt::UserRole));
    });

    layout->addWidget(mPriorityCombo, 1);
    fillPriorities();
}

void TodoViewQuickSearch::reset()
{
    mSearchLine->clear();
    mCategoryCombo->setCurrentIndex(0);
    mPriorityCombo->setCurrentIndex(0);
}

void TodoViewQuickSearch::fillPriorities()
{
    QStringList priorityValues;
    priorityValues.append(i18nc("@action:inmenu priority is unspecified", "unspecified"));
    priorityValues.append(i18nc("@action:inmenu highest priority", "%1 (highest)", 1));
    for (int p = 2; p < 10; ++p) {
        if (p == 5) {
            priorityValues.append(i18nc("@action:inmenu medium priority", "%1 (medium)", p));
        } else if (p == 9) {
            priorityValues.append(i18nc("@action:inmenu lowest priority", "%1 (lowest)", p));
        } else {
            priorityValues.append(i18nc("@action:inmenu", "%1", p));
        }
    }
    // TODO: Using the same method as for categories to fill mPriorityCombo
    CalendarSupport::CategoryHierarchyReaderQComboBox(mPriorityCombo).read(priorityValues);
}

#include "moc_todoviewquicksearch.cpp"

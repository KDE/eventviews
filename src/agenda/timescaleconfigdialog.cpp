/*
  SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno@virlet.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "timescaleconfigdialog.h"
#include "prefs.h"

#include <KCalUtils/Stringify>

#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QIcon>
#include <QPushButton>
#include <QTimeZone>
#include <QVBoxLayout>

using namespace EventViews;

class Q_DECL_HIDDEN TimeScaleConfigDialog::Private
{
public:
    Private(TimeScaleConfigDialog *parent, const PrefsPtr &preferences)
        : q(parent)
        , mPreferences(preferences)
    {
    }

public:
    TimeScaleConfigDialog *const q;
    PrefsPtr mPreferences;
};

enum { TimeZoneNameRole = Qt::UserRole };

using TimeZoneNamePair = QPair<QString, QByteArray>;

static QString tzWithUTC(const QByteArray &zoneId)
{
    auto tz = QTimeZone(zoneId);
    return QStringLiteral("%1 (%2)").arg(i18n(zoneId.constData()), KCalUtils::Stringify::tzUTCOffsetStr(tz));
}

TimeScaleConfigDialog::TimeScaleConfigDialog(const PrefsPtr &preferences, QWidget *parent)
    : QDialog(parent)
    , d(new Private(this, preferences))
{
    setWindowTitle(i18nc("@title:window", "Timezone"));
    auto mainLayout = new QVBoxLayout(this);
    setModal(true);

    auto mainwidget = new QWidget(this);
    setupUi(mainwidget);

    mainLayout->addWidget(mainwidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TimeScaleConfigDialog::reject);

    mainLayout->addWidget(buttonBox);

    QStringList shownTimeZones(QString::fromUtf8(d->mPreferences->timeZone().id()));
    shownTimeZones += d->mPreferences->timeScaleTimezones();
    shownTimeZones.removeDuplicates();

    QVector<TimeZoneNamePair> availList;
    QVector<TimeZoneNamePair> selList;
    const auto zoneIds = QTimeZone::availableTimeZoneIds();
    for (const auto &zoneId : std::as_const(zoneIds)) {
        // do not list timezones already shown
        if (!shownTimeZones.contains(QString::fromUtf8(zoneId))) {
            availList.append(TimeZoneNamePair(tzWithUTC(zoneId), zoneId));
        } else {
            selList.append(TimeZoneNamePair(tzWithUTC(zoneId), zoneId));
        }
    }
    std::sort(availList.begin(), availList.end());

    for (const TimeZoneNamePair &item : std::as_const(availList)) {
        zoneCombo->addItem(item.first, item.second);
    }
    zoneCombo->setCurrentIndex(0);

    addButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    removeButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    upButton->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    downButton->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));

    connect(addButton, &QPushButton::clicked, this, &TimeScaleConfigDialog::add);
    connect(removeButton, &QPushButton::clicked, this, &TimeScaleConfigDialog::remove);
    connect(upButton, &QPushButton::clicked, this, &TimeScaleConfigDialog::up);
    connect(downButton, &QPushButton::clicked, this, &TimeScaleConfigDialog::down);

    connect(okButton, &QPushButton::clicked, this, &TimeScaleConfigDialog::okClicked);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &TimeScaleConfigDialog::reject);
    connect(listWidget, &QListWidget::currentItemChanged, this, &TimeScaleConfigDialog::slotUpdateButton);

    for (const TimeZoneNamePair &item : std::as_const(selList)) {
        auto widgetItem = new QListWidgetItem(item.first);
        widgetItem->setData(TimeZoneNameRole, item.second);
        listWidget->addItem(widgetItem);
    }
    slotUpdateButton();
}

TimeScaleConfigDialog::~TimeScaleConfigDialog()
{
    delete d;
}

void TimeScaleConfigDialog::slotUpdateButton()
{
    removeButton->setEnabled(listWidget->currentItem());
    const bool numberElementMoreThanOneElement = (listWidget->count() > 1);
    upButton->setEnabled(numberElementMoreThanOneElement && (listWidget->currentRow() >= 1));
    downButton->setEnabled(numberElementMoreThanOneElement && (listWidget->currentRow() < listWidget->count() - 1));
}

void TimeScaleConfigDialog::okClicked()
{
    d->mPreferences->setTimeScaleTimezones(zones());
    d->mPreferences->writeConfig();
    accept();
}

void TimeScaleConfigDialog::add()
{
    // Do not add duplicates
    if (zoneCombo->currentIndex() >= 0) {
        const int numberItem(listWidget->count());
        for (int i = 0; i < numberItem; ++i) {
            if (listWidget->item(i)->data(TimeZoneNameRole).toString() == zoneCombo->itemData(zoneCombo->currentIndex(), TimeZoneNameRole).toString()) {
                return;
            }
        }

        auto item = new QListWidgetItem(zoneCombo->currentText());
        item->setData(TimeZoneNameRole, zoneCombo->itemData(zoneCombo->currentIndex(), TimeZoneNameRole).toString());
        listWidget->addItem(item);
        zoneCombo->removeItem(zoneCombo->currentIndex());
    }
    slotUpdateButton();
}

void TimeScaleConfigDialog::remove()
{
    zoneCombo->insertItem(0, listWidget->currentItem()->text(), zoneCombo->itemData(zoneCombo->currentIndex(), TimeZoneNameRole).toString());
    delete listWidget->takeItem(listWidget->currentRow());
    slotUpdateButton();
}

void TimeScaleConfigDialog::up()
{
    int row = listWidget->currentRow();
    QListWidgetItem *item = listWidget->takeItem(row);
    listWidget->insertItem(qMax(row - 1, 0), item);
    listWidget->setCurrentRow(qMax(row - 1, 0));
}

void TimeScaleConfigDialog::down()
{
    int row = listWidget->currentRow();
    QListWidgetItem *item = listWidget->takeItem(row);
    listWidget->insertItem(qMin(row + 1, listWidget->count()), item);
    listWidget->setCurrentRow(qMin(row + 1, listWidget->count() - 1));
}

QStringList TimeScaleConfigDialog::zones() const
{
    QStringList list;
    const int count = listWidget->count();
    list.reserve(count);
    for (int i = 0; i < count; ++i) {
        list << listWidget->item(i)->data(TimeZoneNameRole).toString();
    }
    return list;
}

/*
  Copyright (c) 2007 Bruno Virlet <bruno@virlet.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#include "timescaleconfigdialog.h"
#include "prefs.h"


#include <KLocalizedString>
#include <QIcon>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimeZone>
#include <QVBoxLayout>

using namespace EventViews;

class Q_DECL_HIDDEN TimeScaleConfigDialog::Private
{
public:
    Private(TimeScaleConfigDialog *parent, const PrefsPtr &preferences)
        : q(parent), mPreferences(preferences)
    {
    }

public:
    TimeScaleConfigDialog *const q;
    PrefsPtr mPreferences;
};

enum {
    TimeZoneNameRole = Qt::UserRole
};

typedef QPair<QString, QByteArray> TimeZoneNamePair;

//TODO: move to KCalCore::Stringify
static QString tzUTCOffsetStr(const QTimeZone &tz)
{
    auto currentOffset = tz.offsetFromUtc(QDateTime::currentDateTimeUtc());
    int utcOffsetHrs = currentOffset / 3600;  // in hours
    int utcOffsetMins = (currentOffset % 3600) / 60;    // in minutes
    QString utcStr;
    if (utcOffsetMins > 0) {
        utcStr = utcOffsetHrs >= 0 ?
                 QStringLiteral("+%1:%2").arg(utcOffsetHrs).arg(utcOffsetMins) :
                 QStringLiteral("%1:%2").arg(utcOffsetHrs).arg(utcOffsetMins);

    } else {
        utcStr = utcOffsetHrs >= 0 ?
                 QStringLiteral("+%1").arg(utcOffsetHrs) :
                 QStringLiteral("%1").arg(utcOffsetHrs);
    }
    return utcStr;
}

//TODO: move to KCalCore::Stringify
static QString tzWithUTC(const QByteArray &zoneId)
{
    auto tz = QTimeZone(zoneId);
    return
        QStringLiteral("%1 (UTC%2)").
        arg(i18n(zoneId.constData()), tzUTCOffsetStr(tz));
}

TimeScaleConfigDialog::TimeScaleConfigDialog(const PrefsPtr &preferences, QWidget *parent)
    : QDialog(parent), d(new Private(this, preferences))
{
    setWindowTitle(i18n("Timezone"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setModal(true);

    QWidget *mainwidget = new QWidget(this);
    setupUi(mainwidget);

    mainLayout->addWidget(mainwidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TimeScaleConfigDialog::reject);

    mainLayout->addWidget(buttonBox);

    QStringList shownTimeZones(QString::fromUtf8(d->mPreferences->timeZone().id()));
    shownTimeZones += d->mPreferences->timeScaleTimezones();
    shownTimeZones.removeDuplicates();

    QList<TimeZoneNamePair> availList, selList;
    const auto zoneIds = QTimeZone::availableTimeZoneIds();
    for (const auto &zoneId : qAsConst(zoneIds)) {
        // do not list timezones already shown
        if (!shownTimeZones.contains(QString::fromUtf8(zoneId))) {
            availList.append(TimeZoneNamePair(tzWithUTC(zoneId), zoneId));
        } else {
            selList.append(TimeZoneNamePair(tzWithUTC(zoneId), zoneId));
        }
    }
    qSort(availList.begin(), availList.end());

    for (const TimeZoneNamePair &item : qAsConst(availList)) {
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

    for (const TimeZoneNamePair &item : qAsConst(selList)) {
        QListWidgetItem *widgetItem = new QListWidgetItem(item.first);
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
    removeButton->setEnabled(listWidget->count() > 0);
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

        QListWidgetItem *item = new QListWidgetItem(zoneCombo->currentText());
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


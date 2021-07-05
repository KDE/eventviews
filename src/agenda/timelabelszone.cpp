/*
  SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno@virlet.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "timelabelszone.h"
#include "agenda.h"
#include "agendaview.h"
#include "prefs.h"
#include "timelabels.h"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>

using namespace EventViews;

TimeLabelsZone::TimeLabelsZone(QWidget *parent, const PrefsPtr &preferences, Agenda *agenda)
    : QWidget(parent)
    , mAgenda(agenda)
    , mPrefs(preferences)
    , mParent(qobject_cast<AgendaView *>(parent))
{
    mTimeLabelsLayout = new QHBoxLayout(this);
    mTimeLabelsLayout->setContentsMargins(0, 0, 0, 0);
    mTimeLabelsLayout->setSpacing(0);

    init();
}

void TimeLabelsZone::reset()
{
    for (QScrollArea *label : std::as_const(mTimeLabelsList)) {
        label->hide();
        label->deleteLater();
    }
    mTimeLabelsList.clear();

    init();

    // Update some related geometry from the agenda view
    updateAll();
    if (mParent) {
        mParent->updateTimeBarWidth();
        mParent->createDayLabels(true);
    }
}

void TimeLabelsZone::init()
{
    QStringList seenTimeZones(QString::fromUtf8(mPrefs->timeZone().id()));

    addTimeLabels(mPrefs->timeZone());

    const auto lst = mPrefs->timeScaleTimezones();
    for (const QString &zoneStr : lst) {
        if (!seenTimeZones.contains(zoneStr)) {
            auto zone = QTimeZone(zoneStr.toUtf8());
            if (zone.isValid()) {
                addTimeLabels(zone);
                seenTimeZones += zoneStr;
            }
        }
    }
}

void TimeLabelsZone::addTimeLabels(const QTimeZone &zone)
{
    auto area = new QScrollArea(this);
    auto labels = new TimeLabels(zone, 24, this);
    mTimeLabelsList.prepend(area);
    area->setWidgetResizable(true);
    area->setWidget(labels);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setBackgroundRole(QPalette::Window);
    area->setFrameStyle(QFrame::NoFrame);
    area->show();
    mTimeLabelsLayout->insertWidget(0, area);

    setupTimeLabel(area);
}

void TimeLabelsZone::setupTimeLabel(QScrollArea *area)
{
    if (mAgenda && mAgenda->verticalScrollBar()) {
        // Scrolling the agenda will scroll the timelabel
        connect(mAgenda->verticalScrollBar(), &QAbstractSlider::valueChanged, area->verticalScrollBar(), &QAbstractSlider::setValue);
        // and vice-versa. ( this won't loop )
        connect(area->verticalScrollBar(), &QAbstractSlider::valueChanged, mAgenda->verticalScrollBar(), &QAbstractSlider::setValue);

        area->verticalScrollBar()->setValue(mAgenda->verticalScrollBar()->value());
    }

    auto timeLabels = static_cast<TimeLabels *>(area->widget());
    timeLabels->setAgenda(mAgenda);

    // timelabel's scroll is just a slave, this shouldn't be here
    // if ( mParent ) {
    //  connect( area->verticalScrollBar(), SIGNAL(valueChanged(int)),
    //           mParent, SLOT(setContentsPos(int)) );
    // }
}

int TimeLabelsZone::preferedTimeLabelsWidth() const
{
    if (mTimeLabelsList.isEmpty()) {
        return 0;
    } else {
        return mTimeLabelsList.first()->widget()->sizeHint().width();
    }
}

void TimeLabelsZone::updateAll()
{
    for (QScrollArea *area : std::as_const(mTimeLabelsList)) {
        auto timeLabel = static_cast<TimeLabels *>(area->widget());
        timeLabel->updateConfig();
    }
}

QList<QScrollArea *> TimeLabelsZone::timeLabels() const
{
    return mTimeLabelsList;
}

void TimeLabelsZone::setAgendaView(AgendaView *agendaView)
{
    mParent = agendaView;
    mAgenda = agendaView ? agendaView->agenda() : nullptr;

    for (QScrollArea *timeLabel : std::as_const(mTimeLabelsList)) {
        setupTimeLabel(timeLabel);
    }
}

void TimeLabelsZone::updateTimeLabelsPosition()
{
    if (mAgenda) {
        const auto lst = timeLabels();
        for (QScrollArea *area : lst) {
            auto label = static_cast<TimeLabels *>(area->widget());
            const int adjustment = mAgenda->contentsY();
            // y() is the offset to our parent (QScrollArea)
            // and gets negative as we scroll
            if (adjustment != -label->y()) {
                area->verticalScrollBar()->setValue(adjustment);
            }
        }
    }
}

PrefsPtr TimeLabelsZone::preferences() const
{
    return mPrefs;
}

void TimeLabelsZone::setPreferences(const PrefsPtr &prefs)
{
    if (prefs != mPrefs) {
        mPrefs = prefs;
    }
}

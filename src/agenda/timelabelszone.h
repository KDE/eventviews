/*
  SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno@virlet.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include <QWidget>

class QHBoxLayout;
class QScrollArea;
class QTimeZone;

namespace EventViews
{
class Agenda;
class AgendaView;

class Prefs;
using PrefsPtr = QSharedPointer<Prefs>;

class TimeLabelsZone : public QWidget
{
    Q_OBJECT
public:
    explicit TimeLabelsZone(QWidget *parent, const PrefsPtr &preferences, Agenda *agenda = nullptr, bool readOnly = false);

    /** Add a new time label with the given time zone.
        If @p zone is not valid, use the display time zone.
    */
    void addTimeLabels(const QTimeZone &zone);

    /**
       Returns the best width for each TimeLabels widget
    */
    [[nodiscard]] int preferedTimeLabelsWidth() const;

    void updateAll();
    void reset();
    void init();
    void setAgendaView(AgendaView *agenda);
    void setReadOnly(bool enable);

    [[nodiscard]] QList<QScrollArea *> timeLabels() const;

    void setPreferences(const PrefsPtr &prefs);
    [[nodiscard]] PrefsPtr preferences() const;

    /** Checks how much agenda is scrolled relative to it's QScrollArea
        and makes each TimeLabels scroll that amount
    */
    void updateTimeLabelsPosition();

    AgendaView *agendaView() const;

private:
    void setupTimeLabel(QScrollArea *area);
    bool mReadOnly = false;
    Agenda *mAgenda = nullptr;
    PrefsPtr mPrefs;
    AgendaView *mParent = nullptr;

    QHBoxLayout *mTimeLabelsLayout = nullptr;
    QList<QScrollArea *> mTimeLabelsList;
};
}

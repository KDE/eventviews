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
#ifndef EVENTVIEWS_TIMELABELSZONE_H
#define EVENTVIEWS_TIMELABELSZONE_H


#include <QWidget>

class QHBoxLayout;
class QScrollArea;
class QTimeZone;

namespace EventViews
{

class Agenda;
class AgendaView;

class Prefs;
typedef QSharedPointer<Prefs> PrefsPtr;

class TimeLabelsZone : public QWidget
{
    Q_OBJECT
public:
    explicit TimeLabelsZone(QWidget *parent, const PrefsPtr &preferences, Agenda *agenda = nullptr);

    /** Add a new time label with the given time zone.
        If @p zone is not valid, use the display time zone.
    */
    void addTimeLabels(const QTimeZone &zone);

    /**
       Returns the best width for each TimeLabels widget
    */
    int preferedTimeLabelsWidth() const;

    void updateAll();
    void reset();
    void init();
    void setAgendaView(AgendaView *agenda);

    QList<QScrollArea *> timeLabels() const;

    void setPreferences(const PrefsPtr &prefs);
    PrefsPtr preferences() const;

    /** Checks how much agenda is scrolled relative to it's QScrollArea
        and makes each TimeLabels scroll that amount
    */
    void updateTimeLabelsPosition();

private:
    void setupTimeLabel(QScrollArea *area);
    Agenda *mAgenda;
    PrefsPtr mPrefs;
    AgendaView *mParent;

    QHBoxLayout *mTimeLabelsLayout;
    QList<QScrollArea *> mTimeLabelsList;
};

}

#endif

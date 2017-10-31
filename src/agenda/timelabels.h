/*
  Copyright (c) 2000,2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
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
#ifndef EVENTVIEWS_TIMELABELS_H
#define EVENTVIEWS_TIMELABELS_H

#include <QTimeZone>

#include <QFrame>

namespace EventViews {
class Agenda;
class TimeLabelsZone;

class Prefs;
typedef QSharedPointer<Prefs> PrefsPtr;

class TimeLabels : public QFrame
{
    Q_OBJECT
public:
    typedef QList<TimeLabels *> List;

    TimeLabels(const QTimeZone &zone, int rows, TimeLabelsZone *parent = nullptr,
               Qt::WindowFlags f = nullptr);

    /** updates widget's internal state */
    void updateConfig();

    /**  */
    void setAgenda(Agenda *agenda);

    /**  */
    void paintEvent(QPaintEvent *e) override;

    /** */
    void contextMenuEvent(QContextMenuEvent *event) override;

    /** Returns the time zone of this label */
    QTimeZone timeZone() const;

    /**
      Return string which can be used as a header for the time label.
    */
    QString header() const;

    /**
      Return string which can be used as a tool tip for the header.
    */
    QString headerToolTip() const;

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

private:
    /** update the position of the marker showing the mouse position */
    void mousePosChanged(const QPoint &pos);

    void showMousePos();
    void hideMousePos();

    void setCellHeight(double height);
    void colorMousePos();
    QTimeZone mTimezone;
    int mRows;
    double mCellHeight;
    int mMiniWidth;
    Agenda *mAgenda = nullptr;
    TimeLabelsZone *mTimeLabelsZone = nullptr;

    QFrame *mMousePos = nullptr;  // shows a marker for the current mouse position in y direction
};
}

#endif

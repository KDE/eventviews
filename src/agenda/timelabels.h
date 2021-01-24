/*
  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno@virlet.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#ifndef EVENTVIEWS_TIMELABELS_H
#define EVENTVIEWS_TIMELABELS_H

#include <QTimeZone>

#include <QFrame>

namespace EventViews {
class Agenda;
class TimeLabelsZone;

class Prefs;
using PrefsPtr = QSharedPointer<Prefs>;

class TimeLabels : public QFrame
{
    Q_OBJECT
public:
    using List = QList<TimeLabels *>;

    TimeLabels(const QTimeZone &zone, int rows, TimeLabelsZone *parent = nullptr, Qt::WindowFlags f = {});

    /** updates widget's internal state */
    void updateConfig();

    /**  */
    void setAgenda(Agenda *agenda);

    /**  */
    void paintEvent(QPaintEvent *e) override;

    /** */
    void contextMenuEvent(QContextMenuEvent *event) override;

    /** Returns the time zone of this label */
    Q_REQUIRED_RESULT QTimeZone timeZone() const;

    /**
      Return string which can be used as a header for the time label.
    */
    Q_REQUIRED_RESULT QString header() const;

    /**
      Return string which can be used as a tool tip for the header.
    */
    Q_REQUIRED_RESULT QString headerToolTip() const;

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

    /** */
    bool event(QEvent *event) override;

private:
    Q_REQUIRED_RESULT int yposToCell(const int ypos) const;
    Q_REQUIRED_RESULT int cellToHour(const int cell) const;
    Q_REQUIRED_RESULT QString cellToSuffix(const int cell) const;

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

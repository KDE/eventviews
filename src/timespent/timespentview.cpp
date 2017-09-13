/*
  This file is part of KOrganizer.

  Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "timespentview.h"

#include <KCalCore/Event>
#include <Akonadi/Calendar/ETMCalendar>

#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include <QBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QLocale>

using namespace EventViews;

namespace EventViews
{

class TimeSpentWidget : public QWidget
{
public:
    TimeSpentWidget(TimeSpentView *parent)
        : QWidget(parent), mTimeSpentView(parent) {}
    ~TimeSpentWidget() {}

    void paintEvent(QPaintEvent *e) override {
        QPainter p(this);
        p.fillRect(e->rect(), Qt::white);
        int margin = 10;
        int y = 90;

        p.fillRect(QRect(5, 5, width(), 35), QColor(54, 121, 173));       // sync with kowhatsnextview
        QPen oldPen = p.pen();
        QFont oldFont = p.font();
        QFont font = p.font();
        font.setPointSize(25);
        font.setBold(true);
        p.setFont(font);
        p.setPen(QColor(Qt::white));
        p.drawText(QPoint(25, 35), i18n("Time Tracker"));
        p.setPen(oldPen);
        p.setFont(oldFont);

        QString dateText;
        if (mTimeSpentView->mStartDate.daysTo(mTimeSpentView->mEndDate) < 1)
        {
            dateText = QLocale::system().toString(mTimeSpentView->mStartDate);
        } else {
            dateText = i18nc("Date from - to", "%1 - %2",
            QLocale::system().toString(mTimeSpentView->mStartDate),
            QLocale::system().toString(mTimeSpentView->mEndDate));
        }
        font.setPointSize(20);
        font.setBold(true);
        p.setFont(font);
        p.drawText(QPoint(margin, 60), dateText);
        p.setPen(oldPen);
        p.setFont(oldFont);

        QMap<QString, int> secondsSpent;

        int total = 0;

        foreach (const KCalCore::Event::Ptr &e, mEventList)
        {
            Q_ASSERT(e);
            QDateTime selectedStart(mTimeSpentView->mStartDate, QTime(0, 0), e->dtStart().timeZone());

            QDateTime selectedEnd(mTimeSpentView->mEndDate.addDays(1), QTime(0, 0), e->dtEnd().timeZone());

            QDateTime start;
            QDateTime end;

            // duration of all occurrences added
            int totalDuration = 0;

            if (e->recurs()) {
                int eventDuration = e->dtStart().secsTo(e->dtEnd());

                // timesInInterval only return events that have their start inside the interval
                // so we resize the interval by -eventDuration
                const auto times = e->recurrence()->timesInInterval(selectedStart.addSecs(-eventDuration), selectedEnd);

                foreach (const QDateTime &dt, times) {
                    // either the event's start or the event's end must be in the view's interval
                    if (dt >= selectedStart || dt.addSecs(eventDuration) >= selectedStart) {

                        start = dt > selectedStart ? dt : selectedStart;
                        end   = dt.addSecs(eventDuration) < selectedEnd ? dt.addSecs(eventDuration) : selectedEnd;
                        totalDuration += start.secsTo(end);
                    }
                }

            } else {
                // The event's start can be before the view's start date or end after the view's end
                start  = e->dtStart() > selectedStart ? e->dtStart() : selectedStart;
                end    = e->dtEnd()   < selectedEnd   ? e->dtEnd()   : selectedEnd;

                totalDuration += start.secsTo(end);
            }

            if (totalDuration == 0) {
                continue;
            }

            if (!e->categories().isEmpty()) {
                foreach (const QString &s, e->categories()) {
                    secondsSpent[ s ] += totalDuration;
                }
            } else {
                secondsSpent[ i18n("No category") ] += totalDuration;
            }

            total += totalDuration;
        }

        QMapIterator<QString, int> i(secondsSpent);
        QFontMetrics fm = p.fontMetrics();
        int lineHeight = fm.boundingRect(QStringLiteral("No category")).height();
        int totalLineHeight = lineHeight + 2; // vertical margin included

        while (i.hasNext())
        {
            i.next();

            // bar
            const QColor color = CalendarSupport::KCalPrefs::instance()->categoryColor(i.key());
            const int length =
                static_cast<int>(((double) i.value()) / total * (width() - 3 * margin));

            QPainterPath path(QPoint(margin, y));
            path.lineTo(margin + length, y);
            if (length < margin) {
                path.lineTo(margin + length, y + lineHeight);
            } else {
                path.arcTo(QRect(margin + length, y, 2 * margin, lineHeight), +90, -180);
            }
            path.lineTo(margin, y + lineHeight);
            path.closeSubpath();
            p.setBrush(color);
            p.drawPath(path);

            // text
            int totHr, perHr;
            if (total > 0) {
                totHr = i.value() / (60 * 60);
                perHr = static_cast<int>(((double) i.value() * 100 / total));
            } else {
                totHr = 0;
                perHr = 0;
            }
            p.drawText(QRect(margin + 2, y + 2, width() - 2 * margin, lineHeight),
                       i.key() + QLatin1String(": ") +
                       i18ncp("number of hours spent", "%1 hour", "%1 hours", totHr) +
                       i18nc("percent of hours spent", " (%1%)", perHr));
            y += totalLineHeight;
        }
    }

    KCalCore::Event::List mEventList;
    TimeSpentView *mTimeSpentView = nullptr;
};

}

TimeSpentView::TimeSpentView(QWidget *parent)
    : EventView(parent)
{
    mView = new TimeSpentWidget(this);

    QBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->addWidget(mView);
}

TimeSpentView::~TimeSpentView()
{
}

int TimeSpentView::currentDateCount() const
{
    return mStartDate.daysTo(mEndDate);
}

void TimeSpentView::showDates(const QDate &start, const QDate &end, const QDate &)
{
    mStartDate = start;
    mEndDate = end;
    updateView();
}

void TimeSpentView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    Q_UNUSED(incidenceList);
    Q_UNUSED(date);
}

void TimeSpentView::changeIncidenceDisplay(const Akonadi::Item &,
        Akonadi::IncidenceChanger::ChangeType)
{
    updateView();
}

void TimeSpentView::updateView()
{
    mView->mEventList = calendar()->events(mStartDate, mEndDate, QTimeZone::systemTimeZone());
    mView->repaint();
}


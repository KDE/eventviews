/*
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno@virlet.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "timelabels.h"
#include "agenda.h"
#include "prefs.h"
#include "timelabelszone.h"
#include "timescaleconfigdialog.h"

#include <KCalUtils/Stringify>

#include <KLocalizedString>

#include <QHelpEvent>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QToolTip>

using namespace EventViews;

TimeLabels::TimeLabels(const QTimeZone &zone, int rows, TimeLabelsZone *parent, Qt::WindowFlags f)
    : QFrame(parent, f)
    , mTimezone(zone)
{
    mTimeLabelsZone = parent;
    mRows = rows;
    mMiniWidth = 0;

    mCellHeight = mTimeLabelsZone->preferences()->hourSize() * 4;

    setBackgroundRole(QPalette::Window);

    mMousePos = new QFrame(this);
    mMousePos->setLineWidth(1);
    mMousePos->setFrameStyle(QFrame::HLine | QFrame::Plain);
    mMousePos->setFixedSize(width(), 1);
    colorMousePos();
    mAgenda = nullptr;

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    updateConfig();
}

void TimeLabels::mousePosChanged(const QPoint &pos)
{
    colorMousePos();
    mMousePos->move(0, pos.y());

    // The repaint somehow prevents that the red line leaves a black artifact when
    // moved down. It's not a full solution, though.
    repaint();
}

void TimeLabels::showMousePos()
{
    // touch screen have no mouse position
    mMousePos->show();
}

void TimeLabels::hideMousePos()
{
    mMousePos->hide();
}

void TimeLabels::colorMousePos()
{
    QPalette pal;
    pal.setColor(QPalette::Window,  // for Oxygen
                 mTimeLabelsZone->preferences()->agendaMarcusBainsLineLineColor());
    pal.setColor(QPalette::WindowText,  // for Plastique
                 mTimeLabelsZone->preferences()->agendaMarcusBainsLineLineColor());
    mMousePos->setPalette(pal);
}

void TimeLabels::setCellHeight(double height)
{
    if (mCellHeight != height) {
        mCellHeight = height;
        updateGeometry();
    }
}

QSize TimeLabels::minimumSizeHint() const
{
    QSize sh = QFrame::sizeHint();
    sh.setWidth(mMiniWidth);
    return sh;
}

static bool use12Clock()
{
    const QString str = QLocale().timeFormat();
    // 'A' or 'a' means am/pm is shown (and then 'h' uses 12-hour format)
    // but 'H' forces a 24-hour format anyway, even with am/pm shown.
    return str.contains(QLatin1Char('a'), Qt::CaseInsensitive) && !str.contains(QLatin1Char('H'));
}

/** updates widget's internal state */
void TimeLabels::updateConfig()
{
    setFont(mTimeLabelsZone->preferences()->agendaTimeLabelsFont());

    QString test = QStringLiteral("20");
    if (use12Clock()) {
        test = QStringLiteral("12");
    }
    mMiniWidth = fontMetrics().boundingRect(test).width();
    if (use12Clock()) {
        test = QStringLiteral("pm");
    } else {
        test = QStringLiteral("00");
    }
    QFont sFont = font();
    sFont.setPointSize(sFont.pointSize() / 2);
    QFontMetrics fmS(sFont);
    mMiniWidth += fmS.boundingRect(test).width() + frameWidth() * 2 + 4;

    /** Can happen if all resources are disabled */
    if (!mAgenda) {
        return;
    }

    // update HourSize
    mCellHeight = mTimeLabelsZone->preferences()->hourSize() * 4;
    // If the agenda is zoomed out so that more than 24 would be shown,
    // the agenda only shows 24 hours, so we need to take the cell height
    // from the agenda, which is larger than the configured one!
    if (mCellHeight < 4 * mAgenda->gridSpacingY()) {
        mCellHeight = 4 * mAgenda->gridSpacingY();
    }

    updateGeometry();

    repaint();
}

/**  */
void TimeLabels::setAgenda(Agenda *agenda)
{
    mAgenda = agenda;

    if (mAgenda) {
        connect(mAgenda, &Agenda::mousePosSignal, this, &TimeLabels::mousePosChanged);
        connect(mAgenda, &Agenda::enterAgenda, this, &TimeLabels::showMousePos);
        connect(mAgenda, &Agenda::leaveAgenda, this, &TimeLabels::hideMousePos);
        connect(mAgenda, &Agenda::gridSpacingYChanged, this, &TimeLabels::setCellHeight);
    }
}

int TimeLabels::yposToCell(const int ypos) const
{
    const KCalendarCore::DateList datelist = mAgenda->dateList();
    if (datelist.isEmpty()) {
        return 0;
    }

    const auto firstDay = QDateTime(datelist.first(), QTime(0, 0, 0), Qt::LocalTime).toUTC();
    const int beginning   // the hour we start drawing with
        = !mTimezone.isValid()
          ? 0
          : (mTimezone.offsetFromUtc(firstDay)
             -mTimeLabelsZone->preferences()->timeZone().offsetFromUtc(firstDay)) / 3600;

    return static_cast<int>(ypos / mCellHeight) + beginning;
}

int TimeLabels::cellToHour(const int cell) const
{
    int tCell = cell % 24;
    // handle different timezones
    if (tCell < 0) {
        tCell += 24;
    }
    // handle 24h and am/pm time formats
    if (use12Clock()) {
        if (tCell == 0) {
            tCell = 12;
        }
        if (tCell < 0) {
            tCell += 24;
        }
        if (tCell > 12) {
            tCell %= 12;
            if (tCell == 0) {
                tCell = 12;
            }
        }
    }
    return tCell;
}

QString TimeLabels::cellToSuffix(const int cell) const
{
    //TODO: rewrite this using QTime's time formats. "am/pm" doesn't make sense
    // in some locale's
    QString suffix;
    if (use12Clock()) {
        if ((cell / 12) % 2 != 0) {
            suffix = QStringLiteral("pm");
        } else {
            suffix = QStringLiteral("am");
        }
    } else {
        suffix = QStringLiteral("00");
    }
    return suffix;
}

/** This is called in response to repaint() */
void TimeLabels::paintEvent(QPaintEvent *)
{
    if (!mAgenda) {
        return;
    }
    const KCalendarCore::DateList datelist = mAgenda->dateList();
    if (datelist.isEmpty()) {
        return;
    }

    QPainter p(this);

    const int ch = height();

    // We won't paint parts that aren't visible
    const int cy = -y(); // y() returns a negative value.

    const auto firstDay = QDateTime(datelist.first(), QTime(0, 0, 0), Qt::LocalTime).toUTC();
    const int beginning
        = !mTimezone.isValid()
          ? 0
          : (mTimezone.offsetFromUtc(firstDay)
             -mTimeLabelsZone->preferences()->timeZone().offsetFromUtc(firstDay)) / 3600;

    // bug:  the parameters cx and cw are the areas that need to be
    //       redrawn, not the area of the widget.  unfortunately, this
    //       code assumes the latter...

    // now, for a workaround...
    const int cx = frameWidth() * 2;
    const int cw = width();
    // end of workaround

    int cell = yposToCell(cy);
    double y = (cell - beginning) * mCellHeight;
    QFontMetrics fm = fontMetrics();
    QString hour;
    int timeHeight = fm.ascent();
    QFont hourFont = mTimeLabelsZone->preferences()->agendaTimeLabelsFont();
    p.setFont(font());

    //TODO: rewrite this using QTime's time formats. "am/pm" doesn't make sense
    // in some locale's
    QString suffix;
    if (!use12Clock()) {
        suffix = QStringLiteral("00");
    } else {
        suffix = QStringLiteral("am");
    }

    // We adjust the size of the hour font to keep it reasonable
    if (timeHeight > mCellHeight) {
        timeHeight = static_cast<int>(mCellHeight - 1);
        int pointS = hourFont.pointSize();
        while (pointS > 4) {   // TODO: use smallestReadableFont() when added to kdelibs
            hourFont.setPointSize(pointS);
            fm = QFontMetrics(hourFont);
            if (fm.ascent() < mCellHeight) {
                break;
            }
            --pointS;
        }
        fm = QFontMetrics(hourFont);
        timeHeight = fm.ascent();
    }
    //timeHeight -= (timeHeight/4-2);
    QFont suffixFont = hourFont;
    suffixFont.setPointSize(suffixFont.pointSize() / 2);
    QFontMetrics fmS(suffixFont);
    const int startW = cw - frameWidth() - 2;
    const int tw2 = fmS.boundingRect(suffix).width();
    const int divTimeHeight = (timeHeight - 1) / 2 - 1;
    //testline
    //p->drawLine(0,0,0,contentsHeight());
    while (y < cy + ch + mCellHeight) {
        QColor lineColor, textColor;
        textColor = palette().color(QPalette::WindowText);
        if (cell < 0 || cell >= 24) {
            textColor.setAlphaF(0.5);
        }
        lineColor = textColor;
        lineColor.setAlphaF(lineColor.alphaF() / 5.);
        p.setPen(lineColor);

        // hour, full line
        p.drawLine(cx, int(y), cw + 2, int(y));

        // set the hour and suffix from the cell
        hour.setNum(cellToHour(cell));
        suffix = cellToSuffix(cell);

        // draw the time label
        p.setPen(textColor);
        const int timeWidth = fm.boundingRect(hour).width();
        int offset = startW - timeWidth - tw2 - 1;
        p.setFont(hourFont);
        p.drawText(offset, static_cast<int>(y + timeHeight), hour);
        p.setFont(suffixFont);
        offset = startW - tw2;
        p.drawText(offset, static_cast<int>(y + timeHeight - divTimeHeight), suffix);

        // increment indices
        y += mCellHeight;
        cell++;
    }
}

QSize TimeLabels::sizeHint() const
{
    return {mMiniWidth, static_cast<int>(mRows * mCellHeight)};
}

void TimeLabels::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)

    QMenu popup(this);
    QAction *editTimeZones = popup.addAction(QIcon::fromTheme(QStringLiteral("document-properties")),
                                             i18n("&Add Timezones..."));
    QAction *removeTimeZone = popup.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")),
                                              i18n("&Remove Timezone %1", i18n(mTimezone.id().constData())));
    if (!mTimezone.isValid()
        || !mTimeLabelsZone->preferences()->timeScaleTimezones().count()
        || mTimezone == mTimeLabelsZone->preferences()->timeZone()) {
        removeTimeZone->setEnabled(false);
    }

    QAction *activatedAction = popup.exec(QCursor::pos());
    if (activatedAction == editTimeZones) {
        QPointer<TimeScaleConfigDialog> dialog = new TimeScaleConfigDialog(mTimeLabelsZone->preferences(), this);
        if (dialog->exec() == QDialog::Accepted) {
            mTimeLabelsZone->reset();
        }
        delete dialog;
    } else if (activatedAction == removeTimeZone) {
        QStringList list = mTimeLabelsZone->preferences()->timeScaleTimezones();
        list.removeAll(QString::fromUtf8(mTimezone.id()));
        mTimeLabelsZone->preferences()->setTimeScaleTimezones(list);
        mTimeLabelsZone->preferences()->writeConfig();
        mTimeLabelsZone->reset();
        hide();
        deleteLater();
    }
}

QTimeZone TimeLabels::timeZone() const
{
    return mTimezone;
}

QString TimeLabels::header() const
{
    return i18n(mTimezone.id().constData());
}

QString TimeLabels::headerToolTip() const
{
    QDateTime now = QDateTime::currentDateTime();
    QString toolTip;

    toolTip += QLatin1String("<qt>");
    toolTip += i18nc("title for timezone info, the timezone id and utc offset",
                     "<b>%1 (%2)</b>", i18n(mTimezone.id().constData()),
                     KCalUtils::Stringify::tzUTCOffsetStr(mTimezone));
    toolTip += QLatin1String("<hr>");
    toolTip += i18nc("heading for timezone display name",
                     "<i>Name:</i> %1", mTimezone.displayName(now, QTimeZone::LongName));
    toolTip += QLatin1String("<br/>");

    if (mTimezone.country() != QLocale::AnyCountry) {
        toolTip += i18nc("heading for timezone country",
                         "<i>Country:</i> %1", QLocale::countryToString(mTimezone.country()));
        toolTip += QLatin1String("<br/>");
    }

    auto abbreviations = QStringLiteral("&nbsp;");
    const auto lst = mTimezone.transitions(now, now.addYears(1));
    for (const auto &transition : lst) {
        abbreviations += transition.abbreviation;
        abbreviations += QLatin1String(",&nbsp;");
    }
    abbreviations.chop(7);
    if (!abbreviations.isEmpty()) {
        toolTip += i18nc("heading for comma-separated list of timezone abbreviations",
                         "<i>Abbreviations:</i>");
        toolTip += abbreviations;
        toolTip += QLatin1String("<br/>");
    }
    const QString timeZoneComment(mTimezone.comment());
    if (!timeZoneComment.isEmpty()) {
        toolTip += i18nc("heading for the timezone comment",
                         "<i>Comment:</i> %1", timeZoneComment);
    }
    toolTip += QLatin1String("</qt>");

    return toolTip;
}

bool TimeLabels::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto helpEvent = static_cast<QHelpEvent *>(event);
        const int cell = yposToCell(helpEvent->pos().y());

        QString toolTip;
        toolTip += QLatin1String("<qt>");
        toolTip += i18nc("[hour of the day][am/pm/00] [timezone id (timezone-offset)]",
                         "%1%2<br/>%3 (%4)",
                         cellToHour(cell), cellToSuffix(cell),
                         i18n(mTimezone.id().constData()),
                         KCalUtils::Stringify::tzUTCOffsetStr(mTimezone));
        toolTip += QLatin1String("</qt>");

        QToolTip::showText(helpEvent->globalPos(), toolTip, this);

        return true;
    }
    return QWidget::event(event);
}

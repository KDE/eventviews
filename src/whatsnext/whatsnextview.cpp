/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "whatsnextview.h"
#include "calendarview_debug.h"

#include <Akonadi/CalendarUtils>
#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include <KCalUtils/IncidenceFormatter>

#include <QBoxLayout>
#include <QUrlQuery>

#include <optional>

using namespace EventViews;
void WhatsNextTextBrowser::doSetSource(const QUrl &name, QTextDocument::ResourceType type)
{
    Q_UNUSED(type);
    if (name.scheme() == QLatin1String("event")) {
        Q_EMIT showIncidence(name);
    } else if (name.scheme() == QLatin1String("todo")) {
        Q_EMIT showIncidence(name);
    } else {
        QTextBrowser::setSource(name);
    }
}

WhatsNextView::WhatsNextView(QWidget *parent)
    : EventView(parent)
    , mView(new WhatsNextTextBrowser(this))
{
    connect(mView, &WhatsNextTextBrowser::showIncidence, this, &WhatsNextView::showIncidence);

    QBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(mView);
}

WhatsNextView::~WhatsNextView() = default;

int WhatsNextView::currentDateCount() const
{
    return mStartDate.daysTo(mEndDate);
}

void WhatsNextView::createTaskRow(KIconLoader *kil)
{
    QString ipath;
    kil->loadIcon(QStringLiteral("view-calendar-tasks"), KIconLoader::NoGroup, 22, KIconLoader::DefaultState, QStringList(), &ipath);
    mText += QLatin1String("<h2><img src=\"");
    mText += ipath;
    mText += QLatin1String(R"(" width="22" height="22">)");
    mText += i18n("To-dos:") + QLatin1String("</h2>\n");
    mText += QLatin1String("<ul>\n");
}

void WhatsNextView::updateView()
{
    KIconLoader *kil = KIconLoader::global();
    QString ipath;
    kil->loadIcon(QStringLiteral("office-calendar"), KIconLoader::NoGroup, 32, KIconLoader::DefaultState, QStringList(), &ipath);

    mText = QStringLiteral("<table width=\"100%\">\n");
    mText += QLatin1String("<tr bgcolor=\"#3679AD\"><td><h1>");
    mText += QLatin1String("<img src=\"");
    mText += ipath;
    mText += QLatin1String(R"(" width="32" height="32">)");
    mText += QLatin1String("<font color=\"white\"> ");
    mText += i18n("What's Next?") + QLatin1String("</font></h1>");
    mText += QLatin1String("</td></tr>\n<tr><td>");

    mText += QLatin1String("<h2>");
    if (mStartDate.daysTo(mEndDate) < 1) {
        mText += QLocale::system().toString(mStartDate);
    } else {
        mText += i18nc("date from - to", "%1 - %2", QLocale::system().toString(mStartDate), QLocale::system().toString(mEndDate));
    }
    mText += QLatin1String("</h2>\n");

    KCalendarCore::Event::List events;
    for (const auto &calendar : calendars()) {
        events += calendar->events(mStartDate, mEndDate, QTimeZone::systemTimeZone(), false);
    }

    events = KCalendarCore::Calendar::sortEvents(std::move(events), KCalendarCore::EventSortStartDate, KCalendarCore::SortDirectionAscending);
    if (!events.isEmpty()) {
        mText += QLatin1String("<p></p>");
        kil->loadIcon(QStringLiteral("view-calendar-day"), KIconLoader::NoGroup, 22, KIconLoader::DefaultState, QStringList(), &ipath);
        mText += QLatin1String("<h2><img src=\"");
        mText += ipath;
        mText += QLatin1String(R"(" width="22" height="22">)");
        mText += i18n("Events:") + QLatin1String("</h2>\n");
        mText += QLatin1String("<table>\n");
        for (const KCalendarCore::Event::Ptr &ev : std::as_const(events)) {
            const auto calendar = calendar3(ev);
            if (!ev->recurs()) {
                appendEvent(calendar, ev);
            } else {
                KCalendarCore::Recurrence *recur = ev->recurrence();
                int duration = ev->dtStart().secsTo(ev->dtEnd());
                QDateTime start = recur->getPreviousDateTime(QDateTime(mStartDate, QTime(), Qt::LocalTime));
                QDateTime end = start.addSecs(duration);
                QDateTime endDate(mEndDate, QTime(23, 59, 59), Qt::LocalTime);
                if (end.date() >= mStartDate) {
                    appendEvent(calendar, ev, start.toLocalTime(), end.toLocalTime());
                }
                const auto times = recur->timesInInterval(start, endDate);
                int count = times.count();
                if (count > 0) {
                    int i = 0;
                    if (times[0] == start) {
                        ++i; // start has already been appended
                    }
                    if (!times[count - 1].isValid()) {
                        --count; // list overflow
                    }
                    for (; i < count && times[i].date() <= mEndDate; ++i) {
                        appendEvent(calendar, ev, times[i].toLocalTime());
                    }
                }
            }
        }
        mText += QLatin1String("</table>\n");
    }

    mTodos.clear();
    KCalendarCore::Todo::List todos;
    for (const auto &calendar : calendars()) {
        todos += calendar->todos(KCalendarCore::TodoSortDueDate, KCalendarCore::SortDirectionAscending);
    }
    if (!todos.isEmpty()) {
        bool taskHeaderWasCreated = false;
        for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todos)) {
            const auto calendar = calendar3(todo);
            if (!todo->isCompleted() && todo->hasDueDate() && todo->dtDue().date() <= mEndDate) {
                if (!taskHeaderWasCreated) {
                    createTaskRow(kil);
                    taskHeaderWasCreated = true;
                }
                appendTodo(calendar, todo);
            }
        }
        bool gotone = false;
        int priority = 1;
        while (!gotone && priority <= 9) {
            for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todos)) {
                const auto calendar = calendar3(todo);
                if (!todo->isCompleted() && (todo->priority() == priority)) {
                    if (!taskHeaderWasCreated) {
                        createTaskRow(kil);
                        taskHeaderWasCreated = true;
                    }
                    appendTodo(calendar, todo);
                    gotone = true;
                }
            }
            priority++;
        }
        if (taskHeaderWasCreated) {
            mText += QLatin1String("</ul>\n");
        }
    }

    QStringList myEmails(CalendarSupport::KCalPrefs::instance()->allEmails());
    int replies = 0;
    events.clear();
    for (const auto &calendar : calendars()) {
        events += calendar->events(QDate::currentDate(), QDate(2975, 12, 6), QTimeZone::systemTimeZone());
    }
    for (const KCalendarCore::Event::Ptr &ev : std::as_const(events)) {
        const auto calendar = calendar3(ev);
        KCalendarCore::Attendee me = ev->attendeeByMails(myEmails);
        if (!me.isNull()) {
            if (me.status() == KCalendarCore::Attendee::NeedsAction && me.RSVP()) {
                if (replies == 0) {
                    mText += QLatin1String("<p></p>");
                    kil->loadIcon(QStringLiteral("mail-reply-sender"), KIconLoader::NoGroup, 22, KIconLoader::DefaultState, QStringList(), &ipath);
                    mText += QLatin1String("<h2><img src=\"");
                    mText += ipath;
                    mText += QLatin1String(R"(" width="22" height="22">)");
                    mText += i18n("Events and to-dos that need a reply:") + QLatin1String("</h2>\n");
                    mText += QLatin1String("<table>\n");
                }
                replies++;
                appendEvent(calendar, ev);
            }
        }
    }

    todos.clear();
    for (const auto &calendar : calendars()) {
        todos += calendar->todos();
    }
    for (const KCalendarCore::Todo::Ptr &to : std::as_const(todos)) {
        const auto calendar = calendar3(to);
        KCalendarCore::Attendee me = to->attendeeByMails(myEmails);
        if (!me.isNull()) {
            if (me.status() == KCalendarCore::Attendee::NeedsAction && me.RSVP()) {
                if (replies == 0) {
                    mText += QLatin1String("<p></p>");
                    kil->loadIcon(QStringLiteral("mail-reply-sender"), KIconLoader::NoGroup, 22, KIconLoader::DefaultState, QStringList(), &ipath);
                    mText += QLatin1String("<h2><img src=\"");
                    mText += ipath;
                    mText += QLatin1String(R"(" width="22" height="22">)");
                    mText += i18n("Events and to-dos that need a reply:") + QLatin1String("</h2>\n");
                    mText += QLatin1String("<table>\n");
                }
                replies++;
                appendEvent(calendar, to);
            }
        }
    }
    if (replies > 0) {
        mText += QLatin1String("</table>\n");
    }

    mText += QLatin1String("</td></tr>\n</table>\n");

    mView->setText(mText);
}

void WhatsNextView::showDates(const QDate &start, const QDate &end, const QDate &)
{
    mStartDate = start;
    mEndDate = end;
    updateView();
}

void WhatsNextView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    Q_UNUSED(incidenceList)
    Q_UNUSED(date)
}

void WhatsNextView::changeIncidenceDisplay(const Akonadi::Item &, Akonadi::IncidenceChanger::ChangeType)
{
    updateView();
}

void WhatsNextView::appendEvent(const Akonadi::CollectionCalendar::Ptr &calendar,
                                const KCalendarCore::Incidence::Ptr &incidence,
                                const QDateTime &start,
                                const QDateTime &end)
{
    mText += QLatin1String("<tr><td><b>");
    if (const KCalendarCore::Event::Ptr event = incidence.dynamicCast<KCalendarCore::Event>()) {
        auto starttime = start.toLocalTime();
        if (!starttime.isValid()) {
            starttime = event->dtStart().toLocalTime();
        }
        auto endtime = end.toLocalTime();
        if (!endtime.isValid()) {
            endtime = starttime.addSecs(event->dtStart().secsTo(event->dtEnd()));
        }

        if (starttime.date().daysTo(endtime.date()) >= 1) {
            if (event->allDay()) {
                mText += i18nc("date from - to",
                               "%1 - %2",
                               QLocale().toString(starttime.date(), QLocale::ShortFormat),
                               QLocale().toString(endtime.date(), QLocale::ShortFormat));
            } else {
                mText +=
                    i18nc("date from - to", "%1 - %2", QLocale().toString(starttime, QLocale::ShortFormat), QLocale().toString(endtime, QLocale::ShortFormat));
            }
        } else {
            if (event->allDay()) {
                mText += QLocale().toString(starttime.date(), QLocale::ShortFormat);
            } else {
                mText += i18nc("date, from - to",
                               "%1, %2 - %3",
                               QLocale().toString(starttime.date(), QLocale::ShortFormat),
                               QLocale().toString(starttime.time(), QLocale::ShortFormat),
                               QLocale().toString(endtime.time(), QLocale::ShortFormat));
            }
        }
    }
    mText += QLatin1String("</b></td>");
    const QString proto = incidence->type() == KCalendarCore::Incidence::TypeTodo ? QStringLiteral("todo") : QStringLiteral("event");
    mText += QStringLiteral(R"(<td><a href="%1:%2?itemId=%3&calendarId=%5">%4</a></td>)")
                 .arg(proto, incidence->uid(), incidence->customProperty("VOLATILE", "AKONADI-ID"), incidence->summary())
                 .arg(calendar->collection().id());
    mText += QLatin1String("</tr>\n");
}

void WhatsNextView::appendTodo(const Akonadi::CollectionCalendar::Ptr &calendar, const KCalendarCore::Incidence::Ptr &incidence)
{
    Akonadi::Item aitem = calendar->item(incidence);
    if (mTodos.contains(aitem)) {
        return;
    }
    mTodos.append(aitem);
    mText += QLatin1String("<li>");
    mText += QStringLiteral(R"(<a href="todo:%1?itemId=%2&calendarId=%4">%3</a>)")
                 .arg(incidence->uid(), incidence->customProperty("VOLATILE", "AKONADI-ID"), incidence->summary())
                 .arg(calendar->collection().id());

    if (const KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(aitem)) {
        if (todo->hasDueDate()) {
            mText += i18nc("to-do due date", "  (Due: %1)", KCalUtils::IncidenceFormatter::dateTimeToString(todo->dtDue(), todo->allDay()));
        }
        mText += QLatin1String("</li>\n");
    }
}

static std::optional<Akonadi::Item::Id> idFromQuery(const QUrlQuery &query, const QString &queryValue)
{
    const auto strVal = query.queryItemValue(queryValue);
    if (strVal.isEmpty()) {
        return {};
    }

    bool ok = false;
    const auto intVal = strVal.toLongLong(&ok);
    if (!ok) {
        return {};
    }

    return intVal;
}

void WhatsNextView::showIncidence(const QUrl &uri)
{
    QUrlQuery query(uri);
    const auto itemId = idFromQuery(query, QStringLiteral("itemId"));
    const auto calendarId = idFromQuery(query, QStringLiteral("calendarId"));

    if (!itemId || !calendarId) {
        qCWarning(CALENDARVIEW_LOG) << "Invalid URI:" << uri;
        return;
    }

    auto calendar = calendarForCollection(*calendarId);
    if (!calendar) {
        qCWarning(CALENDARVIEW_LOG) << "Calendar for collection " << *calendarId << " not present in current view";
        return;
    }

    auto item = calendar->item(*itemId);
    if (!item.isValid()) {
        qCWarning(CALENDARVIEW_LOG) << "Item " << *itemId << " not found in collection " << *calendarId;
        return;
    }

    Q_EMIT showIncidenceSignal(item);
}

#include "moc_whatsnextview.cpp"

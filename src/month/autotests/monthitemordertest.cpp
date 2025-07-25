/*
 * SPDX-FileCopyrightText: 2021 Glen Ditchfield <GJDitchfield@acm.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include "../monthitem.h"

using namespace EventViews;

class MonthItemOrderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void longerInstancesFirst();
    void holidaysFirst();
    void stableOrder();

public:
    IncidenceMonthItem *eventItem(QDate start, QDate end);
};

/**
 * Longer instances are placed before shorter ones, regardless of their
 * relative dates.
 */
void MonthItemOrderTest::longerInstancesFirst()
{
    const QDate startDate(2000, 01, 01);
    const IncidenceMonthItem *longEvent = eventItem(startDate, startDate.addDays(1));
    auto longHoliday = new HolidayMonthItem(nullptr, startDate, startDate.addDays(1), QString());
    for (int offset = -1; offset < 3; offset++) {
        const QDate d = startDate.addDays(offset);

        const IncidenceMonthItem *shortEvent = eventItem(d, d);
        QVERIFY(MonthItem::greaterThan(longEvent, shortEvent));
        QVERIFY(!MonthItem::greaterThan(shortEvent, longEvent));
        QVERIFY(MonthItem::greaterThan(longHoliday, shortEvent));
        QVERIFY(!MonthItem::greaterThan(shortEvent, longHoliday));

        auto shortHoliday = new HolidayMonthItem(nullptr, d, QString());
        QVERIFY(MonthItem::greaterThan(longEvent, shortHoliday));
        QVERIFY(!MonthItem::greaterThan(shortHoliday, longEvent));
        QVERIFY(MonthItem::greaterThan(longHoliday, shortHoliday));
        QVERIFY(!MonthItem::greaterThan(shortHoliday, longHoliday));
    }
}

/**
 * Holidays are placed before events with the same length and day.
 */
void MonthItemOrderTest::holidaysFirst()
{
    const QDate startDate(2000, 01, 01);
    const IncidenceMonthItem *event = eventItem(startDate, startDate);
    auto holiday = new HolidayMonthItem(nullptr, startDate, QString());
    QVERIFY(!MonthItem::greaterThan(event, holiday));
    QVERIFY(MonthItem::greaterThan(holiday, event));
}

/**
 * If two holidays are on the same day, they do not both come before the other.
 * Similarly for two events with the same length and start day.
 */
void MonthItemOrderTest::stableOrder()
{
    const QDate startDate(2000, 01, 01);

    auto holiday = new HolidayMonthItem(nullptr, startDate, QString());
    auto otherHoliday = new HolidayMonthItem(nullptr, startDate, QString());
    QVERIFY(!(MonthItem::greaterThan(otherHoliday, holiday) && MonthItem::greaterThan(holiday, otherHoliday)));

    const IncidenceMonthItem *event = eventItem(startDate, startDate);
    const IncidenceMonthItem *otherEvent = eventItem(startDate, startDate);
    QVERIFY(!(MonthItem::greaterThan(otherEvent, event) && MonthItem::greaterThan(event, otherEvent)));
}

IncidenceMonthItem *MonthItemOrderTest::eventItem(QDate start, QDate end)
{
    auto e = new KCalendarCore::Event;
    e->setDtStart(QDateTime(start, QTime(00, 00, 00)));
    e->setDtEnd(QDateTime(end, QTime(00, 00, 00)));
    e->setAllDay(true);
    return new IncidenceMonthItem(nullptr, nullptr, Akonadi::Item(), KCalendarCore::Event::Ptr(e), start);
}

QTEST_MAIN(MonthItemOrderTest)

#include "monthitemordertest.moc"

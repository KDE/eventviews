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
    QDate startDate(2000, 01, 01);
    IncidenceMonthItem *longEvent = eventItem(startDate, startDate.addDays(1));
    HolidayMonthItem *longHoliday = new HolidayMonthItem(nullptr, startDate, startDate.addDays(1), QStringLiteral(""));
    for (int offset = -1; offset < 3; offset++) {
        QDate d = startDate.addDays(offset);

        IncidenceMonthItem *shortEvent = eventItem(d, d);
        QVERIFY(   MonthItem::greaterThan(longEvent, shortEvent));
        QVERIFY( ! MonthItem::greaterThan(shortEvent, longEvent));
        QVERIFY(   MonthItem::greaterThan(longHoliday, shortEvent));
        QVERIFY( ! MonthItem::greaterThan(shortEvent, longHoliday));

        HolidayMonthItem *shortHoliday = new HolidayMonthItem(nullptr, d, QStringLiteral(""));
        QVERIFY(   MonthItem::greaterThan(longEvent, shortHoliday));
        QVERIFY( ! MonthItem::greaterThan(shortHoliday, longEvent));
        QVERIFY(   MonthItem::greaterThan(longHoliday, shortHoliday));
        QVERIFY( ! MonthItem::greaterThan(shortHoliday, longHoliday));
    }
}

/**
 * Holidays are placed before events with the same length and day.
 */
void MonthItemOrderTest::holidaysFirst()
{
    QDate startDate(2000, 01, 01);
    IncidenceMonthItem *event = eventItem(startDate, startDate);
    HolidayMonthItem *holiday = new HolidayMonthItem(nullptr, startDate, QStringLiteral(""));
    QVERIFY( ! MonthItem::greaterThan(event, holiday));
    QVERIFY(   MonthItem::greaterThan(holiday, event));
}

/**
 * If two holidays are on the same day, they do not both come before the other.
 * Similarly for two events with the same length and start day.
 */
void MonthItemOrderTest::stableOrder()
{
    QDate startDate(2000, 01, 01);

    HolidayMonthItem *holiday = new HolidayMonthItem(nullptr, startDate, QStringLiteral(""));
    HolidayMonthItem *otherHoliday = new HolidayMonthItem(nullptr, startDate, QStringLiteral(""));
    QVERIFY( ! (MonthItem::greaterThan(otherHoliday, holiday) && MonthItem::greaterThan(holiday, otherHoliday)));

    IncidenceMonthItem *event = eventItem(startDate, startDate);
    IncidenceMonthItem *otherEvent = eventItem(startDate, startDate);
    QVERIFY( ! (MonthItem::greaterThan(otherEvent, event) && MonthItem::greaterThan(event, otherEvent)));
}

IncidenceMonthItem *MonthItemOrderTest::eventItem(QDate start, QDate end)
{
    KCalendarCore::Event *e = new KCalendarCore::Event;
    e->setDtStart(QDateTime(start, QTime(00, 00, 00)));
    e->setDtEnd(QDateTime(end, QTime(00, 00, 00)));
    e->setAllDay(true);
    return new IncidenceMonthItem(nullptr, nullptr, Akonadi::Item(), KCalendarCore::Event::Ptr(e), start);
}

QTEST_MAIN(MonthItemOrderTest)

#include "monthitemordertest.moc"

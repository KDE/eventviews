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
    std::unique_ptr<IncidenceMonthItem> eventItem(QDate start, QDate end);
};

/**
 * Longer instances are placed before shorter ones, regardless of their
 * relative dates.
 */
void MonthItemOrderTest::longerInstancesFirst()
{
    const QDate startDate(2000, 01, 01);
    const auto longEvent = eventItem(startDate, startDate.addDays(1));
    auto longHoliday = std::make_unique<HolidayMonthItem>(nullptr, startDate, startDate.addDays(1), QString());
    for (int offset = -1; offset < 3; offset++) {
        const QDate d = startDate.addDays(offset);

        const auto shortEvent = eventItem(d, d);
        QVERIFY(MonthItem::greaterThan(longEvent.get(), shortEvent.get()));
        QVERIFY(!MonthItem::greaterThan(shortEvent.get(), longEvent.get()));
        QVERIFY(MonthItem::greaterThan(longHoliday.get(), shortEvent.get()));
        QVERIFY(!MonthItem::greaterThan(shortEvent.get(), longHoliday.get()));

        auto shortHoliday = std::make_unique<HolidayMonthItem>(nullptr, d, QString());
        QVERIFY(MonthItem::greaterThan(longEvent.get(), shortHoliday.get()));
        QVERIFY(!MonthItem::greaterThan(shortHoliday.get(), longEvent.get()));
        QVERIFY(MonthItem::greaterThan(longHoliday.get(), shortHoliday.get()));
        QVERIFY(!MonthItem::greaterThan(shortHoliday.get(), longHoliday.get()));
    }
}

/**
 * Holidays are placed before events with the same length and day.
 */
void MonthItemOrderTest::holidaysFirst()
{
    const QDate startDate(2000, 01, 01);
    const auto event = eventItem(startDate, startDate);
    auto holiday = std::make_unique<HolidayMonthItem>(nullptr, startDate, QString());
    QVERIFY(!MonthItem::greaterThan(event.get(), holiday.get()));
    QVERIFY(MonthItem::greaterThan(holiday.get(), event.get()));
}

/**
 * If two holidays are on the same day, they do not both come before the other.
 * Similarly for two events with the same length and start day.
 */
void MonthItemOrderTest::stableOrder()
{
    const QDate startDate(2000, 01, 01);

    auto holiday = std::make_unique<HolidayMonthItem>(nullptr, startDate, QString());
    auto otherHoliday = std::make_unique<HolidayMonthItem>(nullptr, startDate, QString());
    QVERIFY(!(MonthItem::greaterThan(otherHoliday.get(), holiday.get()) && MonthItem::greaterThan(holiday.get(), otherHoliday.get())));

    const auto event = eventItem(startDate, startDate);
    const auto otherEvent = eventItem(startDate, startDate);
    QVERIFY(!(MonthItem::greaterThan(otherEvent.get(), event.get()) && MonthItem::greaterThan(event.get(), otherEvent.get())));
}

std::unique_ptr<IncidenceMonthItem> MonthItemOrderTest::eventItem(QDate start, QDate end)
{
    auto e = new KCalendarCore::Event;
    e->setDtStart(QDateTime(start, QTime(00, 00, 00)));
    e->setDtEnd(QDateTime(end, QTime(00, 00, 00)));
    e->setAllDay(true);
    return std::make_unique<IncidenceMonthItem>(nullptr, nullptr, Akonadi::Item(), KCalendarCore::Event::Ptr(e), start);
}

QTEST_MAIN(MonthItemOrderTest)

#include "monthitemordertest.moc"

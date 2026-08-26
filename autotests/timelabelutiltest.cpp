/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../src/agenda/timelabelutil.cpp"

#include <QTest>

using namespace EventViews;

namespace
{
class TimeLabelUtilTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    static void testUTCoffsetStrings()
    {
        const QTimeZone tz1(5 * 60 * 60); // 5 hrs
        QCOMPARE(tzUTCOffsetStr(tz1), QStringLiteral("+05:00"));

        const QTimeZone tz2(-5 * 60 * 60); //-5 hrs
        QCOMPARE(tzUTCOffsetStr(tz2), QStringLiteral("-05:00"));

        const QTimeZone tz3(0);
        QCOMPARE(tzUTCOffsetStr(tz3), QStringLiteral("+00:00"));

        const QTimeZone tz4(30 * 60 * 60); // 30 hrs -- out-of-range
        QCOMPARE(tzUTCOffsetStr(tz4), QStringLiteral("+00:00"));

        const QTimeZone tz5((5 * 60 * 60) + (30 * 60)); // 5:30
        QCOMPARE(tzUTCOffsetStr(tz5), QStringLiteral("+05:30"));

        const QTimeZone tz6(-((11 * 60 * 60) + (59 * 60))); //-11:59
        QCOMPARE(tzUTCOffsetStr(tz6), QStringLiteral("-11:59"));

        const QTimeZone tz7(12 * 60 * 60); // 12:00
        QCOMPARE(tzUTCOffsetStr(tz7), QStringLiteral("+12:00"));

        const QTimeZone tz8(-((12 * 60 * 60) + (59 * 60))); //-12:59
        QCOMPARE(tzUTCOffsetStr(tz8), QStringLiteral("-12:59"));
    }
};
}

QTEST_APPLESS_MAIN(TimeLabelUtilTest)

#include "timelabelutiltest.moc"

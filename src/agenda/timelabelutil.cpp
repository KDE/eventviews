/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileCopyrightText: 2017 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "timelabelutil_p.h"

#include <QTimeZone>

QString EventViews::tzUTCOffsetStr(const QTimeZone &tz)
{
    int const currentOffset = tz.offsetFromUtc(QDateTime::currentDateTimeUtc());
    int const absOffset = qAbs(currentOffset);
    int const utcOffsetHrs = absOffset / 3600; // in hours
    int const utcOffsetMins = (absOffset % 3600) / 60; // in minutes

    const QString hrStr = QStringLiteral("%1").arg(utcOffsetHrs, 2, 10, QLatin1Char('0'));
    const QString mnStr = QStringLiteral("%1").arg(utcOffsetMins, 2, 10, QLatin1Char('0'));

    if (currentOffset < 0) {
        return QStringLiteral("-%1:%2").arg(hrStr, mnStr);
    } else {
        return QStringLiteral("+%1:%2").arg(hrStr, mnStr);
    }
}

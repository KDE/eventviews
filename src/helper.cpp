/*
  SPDX-FileCopyrightText: 2005 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "helper.h"
#include "calendarview_debug.h"
#include "prefs.h"

#include <Collection>
#include <Item>

#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionModifyJob>

#include <QIcon>
#include <QPixmap>

bool EventViews::isColorDark(const QColor &c)
{
    double luminance = (c.red() * 0.299) + (c.green() * 0.587) + (c.blue() * 0.114);
    return (luminance < 128.0) ? true : false;
}

QColor EventViews::getTextColor(const QColor &c)
{
    return (!isColorDark(c)) ? QColor(0, 0, 0) : QColor(255, 255, 255);
}

void EventViews::setResourceColor(const Akonadi::Collection &coll, const QColor &color, const PrefsPtr &preferences)
{
    if (!coll.isValid() || !color.isValid()) {
        return;
    }

    const QString id = QString::number(coll.id());

    // Save the color in akonadi (so the resource can even save it server-side)
    Akonadi::Collection collection = coll;
    auto colorAttr = collection.attribute<Akonadi::CollectionColorAttribute>(Akonadi::Collection::AddIfMissing);
    if (colorAttr) {
        colorAttr->setColor(color);
        auto job = new Akonadi::CollectionModifyJob(collection, nullptr);
        QObject::connect(job, &Akonadi::CollectionModifyJob::result, [=]() {
            if (job->error()) {
                qCWarning(CALENDARVIEW_LOG) << "Failed to set CollectionColorAttribute:" << job->errorString();
            }
        });
    }

    // Also save the color in eventviewsrc (mostly historical)
    preferences->setResourceColor(id, color);
}

QColor EventViews::resourceColor(const Akonadi::Collection &coll, const PrefsPtr &preferences)
{
    if (!coll.isValid()) {
        return {};
    }
    const QString id = QString::number(coll.id());
    // Color stored in eventviewsrc (and in memory)
    QColor color = preferences->resourceColorKnown(id);
    if (color.isValid()) {
        return color;
    }
    // Color stored in akonadi
    if (coll.hasAttribute<Akonadi::CollectionColorAttribute>()) {
        const auto colorAttr = coll.attribute<Akonadi::CollectionColorAttribute>();
        if (colorAttr && colorAttr->color().isValid()) {
            return colorAttr->color();
        }
    }
    // Generate new color and store it in eventsviewsrc (and in memory)
    return preferences->resourceColor(id);
}

QColor EventViews::resourceColor(const Akonadi::Item &item, const PrefsPtr &preferences)
{
    if (!item.isValid()) {
        return {};
    }
    return resourceColor(item.parentCollection(), preferences);
}

int EventViews::yearDiff(QDate start, QDate end)
{
    return end.year() - start.year();
}

QPixmap EventViews::cachedSmallIcon(const QString &name)
{
    return QIcon::fromTheme(name).pixmap(16, 16);
}

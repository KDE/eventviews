/*
  Copyright (C) 2005 Reinhold Kainhofer <reinhold@kainhofer.com>

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

#include "helper.h"
#include "prefs.h"

#include <Collection>
#include <Item>

#include <AkonadiCore/CollectionColorAttribute>

#include <QIcon>
#include <QPixmap>

QColor EventViews::getTextColor(const QColor &c)
{
    double luminance = (c.red() * 0.299) + (c.green() * 0.587) + (c.blue() * 0.114);
    return (luminance > 128.0) ? QColor(0, 0, 0) : QColor(255, 255, 255);
}

void EventViews::setResourceColor(const Akonadi::Collection &coll, const QColor &color, const PrefsPtr &preferences)
{
    if (!coll.isValid()) {
        return;
    }

    const QString id = QString::number(coll.id());
    if (coll.hasAttribute<Akonadi::CollectionColorAttribute>()) {
        Akonadi::CollectionColorAttribute *colorAttr = coll.attribute<Akonadi::CollectionColorAttribute>();
        if (colorAttr && colorAttr->color().isValid() && (colorAttr->color() == color)) {
            // It's the same color: we save an invalid color
            preferences->setResourceColor(id, QColor());
        }
    }
    // in all other cases, we save the resourceColor
    preferences->setResourceColor(id, color);
}


QColor EventViews::resourceColor(const Akonadi::Collection &coll, const PrefsPtr &preferences)
{
    if (!coll.isValid()) {
        return QColor();
    }
    const QString id = QString::number(coll.id());
    QColor color = preferences->resourceColorKnown(id);
    if (!color.isValid() && coll.hasAttribute<Akonadi::CollectionColorAttribute>()) {
        Akonadi::CollectionColorAttribute *colorAttr = coll.attribute<Akonadi::CollectionColorAttribute>();
        if (colorAttr && colorAttr->color().isValid()) {
            color = colorAttr->color();
        } else {
            return preferences->resourceColor(id);
        }
    }
    return color;
}

QColor EventViews::resourceColor(const Akonadi::Item &item, const PrefsPtr &preferences)
{
    if (!item.isValid()) {
        return QColor();
    }
    const QString id = QString::number(item.parentCollection().id());

    QColor color = preferences->resourceColorKnown(id);
    if (!color.isValid() && item.parentCollection().hasAttribute<Akonadi::CollectionColorAttribute>()) {
        Akonadi::CollectionColorAttribute *colorAttr = item.parentCollection().attribute<Akonadi::CollectionColorAttribute>();
        if (colorAttr && colorAttr->color().isValid()) {
            color = colorAttr->color();
        } else {
            return preferences->resourceColor(id);
        }
    }
    return color;
}

int EventViews::yearDiff(const QDate &start, const QDate &end)
{
    return end.year() - start.year();
}

QPixmap EventViews::cachedSmallIcon(const QString &name)
{
    return QIcon::fromTheme(name).pixmap(16, 16);
}

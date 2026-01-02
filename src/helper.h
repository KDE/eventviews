/*
  SPDX-FileCopyrightText: 2005 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventviews_export.h"

#include <QColor>
#include <QSharedPointer>

namespace Akonadi
{
class Collection;
}

class QPixmap;
class QDate;

// This include file declares static methods that are useful to all views.
/*!
 * Namespace EventViews provides facilities for displaying incidences,
 * including events, to-dos, and journal entries.
 */
namespace EventViews
{
class Prefs;
using PrefsPtr = QSharedPointer<Prefs>;

/*!
 Returns a nice QColor for text, give the input color &c.
*/
[[nodiscard]] QColor getTextColor(const QColor &c);

/*!
 * Determines if the \a color is "dark" or "light" by looking at its luminance.
 * idea taken from:
 * https://stackoverflow.com/questions/9780632/how-do-i-determine-if-a-color-is-closer-to-white-or-black
 *
 * Returns true if the specified color is closer to black than white.
 */
[[nodiscard]] bool isColorDark(const QColor &color);

/*!
  This method returns the proper resource / subresource color for the view.
  If a value is stored in the preferences, we use it, else we try to find a
  CollectionColorAttribute in the collection. If everything else fails, a
  random color can be set.
  It is preferred to use this function instead of the
  EventViews::Prefs::resourceColor function.
  Returns The resource color for the incidence. If the incidence belongs
  to a subresource, the color for the subresource is returned (if set).
  \a calendar the calendar for which the resource color should be obtained
  \a incidence the incidence for which the color is needed (to
                   determine which  subresource needs to be used)
*/
[[nodiscard]] EVENTVIEWS_EXPORT QColor resourceColor(const Akonadi::Collection &collection, const PrefsPtr &preferences);

/*!
  This method sets the resource color as an Akonadi collection attribute and
  in the local preferences. It is preferred to use this
  instead of the EventViews::Prefs::setResourceColor function.
  \a collection the collection for which the resource color should be stored
  \a color the color to stored
  \a preferences a pointer to the EventViews::Prefs to use
*/
EVENTVIEWS_EXPORT void setResourceColor(const Akonadi::Collection &collection, const QColor &color, const PrefsPtr &preferences);

/*!
  Returns the number of years between the \a start QDate and the \a end QDate
  (i.e. the difference in the year number of both dates)
*/
[[nodiscard]] int yearDiff(QDate start, QDate end);

/*!
   Equivalent to SmallIcon( name ), but uses QPixmapCache.
   KIconLoader already uses a cache, but it's 20x slower on my tests.

   Returns A new pixmap if it isn't yet in cache, otherwise returns the
           cached one.
*/
[[nodiscard]] QPixmap cachedSmallIcon(const QString &name);
}

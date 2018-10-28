/*
  Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>

  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Author: Kevin Krammer, krake@kdab.com

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
#ifndef EVENTVIEWS_PREFS_H
#define EVENTVIEWS_PREFS_H

#include "eventviews_export.h"
#include "eventview.h"

#include <KConfigSkeleton>

#include <QTimeZone>

namespace EventViews {
class EVENTVIEWS_EXPORT Prefs
{
public:
    /**
      Creates an instance of Prefs with just base config
    */
    Prefs();

    /**
      Creates an instance of Prefs with base config and application override config

      The passed @p appConfig will be queried for matching items whenever one of the
      accessors is called. If one is found it is used for setting/getting the value
      otherwise the one from the eventviews base config is used.
    */
    explicit Prefs(KCoreConfigSkeleton *appConfig);

    ~Prefs();

    void readConfig();
    void writeConfig();

public:
    void setMarcusBainsShowSeconds(bool showSeconds);
    Q_REQUIRED_RESULT bool marcusBainsShowSeconds() const;

    void setAgendaMarcusBainsLineLineColor(const QColor &color);
    Q_REQUIRED_RESULT QColor agendaMarcusBainsLineLineColor() const;

    void setMarcusBainsEnabled(bool enabled);
    Q_REQUIRED_RESULT bool marcusBainsEnabled() const;

    void setAgendaMarcusBainsLineFont(const QFont &font);
    Q_REQUIRED_RESULT QFont agendaMarcusBainsLineFont() const;

    void setHourSize(int size);
    Q_REQUIRED_RESULT int hourSize() const;

    void setDayBegins(const QDateTime &dateTime);
    Q_REQUIRED_RESULT QDateTime dayBegins() const;

    void setWorkingHoursStart(const QDateTime &dateTime);
    Q_REQUIRED_RESULT QDateTime workingHoursStart() const;

    void setWorkingHoursEnd(const QDateTime &dateTime);
    Q_REQUIRED_RESULT QDateTime workingHoursEnd() const;

    void setSelectionStartsEditor(bool startEditor);
    Q_REQUIRED_RESULT bool selectionStartsEditor() const;

    void setAgendaGridWorkHoursBackgroundColor(const QColor &color);
    Q_REQUIRED_RESULT QColor agendaGridWorkHoursBackgroundColor() const;

    void setAgendaGridHighlightColor(const QColor &color);
    Q_REQUIRED_RESULT QColor agendaGridHighlightColor() const;

    void setAgendaGridBackgroundColor(const QColor &color);
    Q_REQUIRED_RESULT QColor agendaGridBackgroundColor() const;

    void setEnableAgendaItemIcons(bool enable);
    Q_REQUIRED_RESULT bool enableAgendaItemIcons() const;

    void setTodosUseCategoryColors(bool useColors);
    Q_REQUIRED_RESULT bool todosUseCategoryColors() const;

    void setAgendaHolidaysBackgroundColor(const QColor &color) const;
    Q_REQUIRED_RESULT QColor agendaHolidaysBackgroundColor() const;

    void setAgendaViewColors(int colors);
    Q_REQUIRED_RESULT int agendaViewColors() const;

    void setAgendaViewFont(const QFont &font);
    Q_REQUIRED_RESULT QFont agendaViewFont() const;

    void setMonthViewFont(const QFont &font);
    Q_REQUIRED_RESULT QFont monthViewFont() const;

    Q_REQUIRED_RESULT QColor monthGridBackgroundColor() const;
    void setMonthGridBackgroundColor(const QColor &color);

    Q_REQUIRED_RESULT QColor monthGridWorkHoursBackgroundColor() const;
    void monthGridWorkHoursBackgroundColor(const QColor &color);

    void setMonthViewColors(int colors) const;
    Q_REQUIRED_RESULT int monthViewColors() const;

    Q_REQUIRED_RESULT bool enableMonthItemIcons() const;
    void setEnableMonthItemIcons(bool enable);

    Q_REQUIRED_RESULT bool showTimeInMonthView() const;
    void setShowTimeInMonthView(bool show);

    Q_REQUIRED_RESULT bool showTodosMonthView() const;
    void setShowTodosMonthView(bool show);

    Q_REQUIRED_RESULT bool showJournalsMonthView() const;
    void setShowJournalsMonthView(bool show);

    Q_REQUIRED_RESULT bool fullViewMonth() const;
    void setFullViewMonth(bool fullView);

    Q_REQUIRED_RESULT bool sortCompletedTodosSeparately() const;
    void setSortCompletedTodosSeparately(bool sort);

    void setEnableToolTips(bool enable);
    Q_REQUIRED_RESULT bool enableToolTips() const;

    void setShowTodosAgendaView(bool show);
    Q_REQUIRED_RESULT bool showTodosAgendaView() const;

    void setAgendaTimeLabelsFont(const QFont &font);
    Q_REQUIRED_RESULT QFont agendaTimeLabelsFont() const;

    KConfigSkeleton::ItemFont *fontItem(const QString &name) const;

    void setResourceColor(const QString &, const QColor &);
    Q_REQUIRED_RESULT QColor resourceColor(const QString &);
    Q_REQUIRED_RESULT QColor resourceColorKnown(const QString &) const;

    Q_REQUIRED_RESULT QTimeZone timeZone() const;

    Q_REQUIRED_RESULT QStringList timeScaleTimezones() const;
    void setTimeScaleTimezones(const QStringList &list);

    Q_REQUIRED_RESULT QStringList selectedPlugins() const;
    void setSelectedPlugins(const QStringList &);

    Q_REQUIRED_RESULT QStringList decorationsAtAgendaViewTop() const;
    void setDecorationsAtAgendaViewTop(const QStringList &);

    Q_REQUIRED_RESULT QStringList decorationsAtAgendaViewBottom() const;
    void setDecorationsAtAgendaViewBottom(const QStringList &);

    Q_REQUIRED_RESULT bool colorAgendaBusyDays() const;
    void setColorAgendaBusyDays(bool enable);

    Q_REQUIRED_RESULT bool colorMonthBusyDays() const;
    void setColorMonthBusyDays(bool enable);

    Q_REQUIRED_RESULT QColor viewBgBusyColor() const;
    void setViewBgBusyColor(const QColor &);

    Q_REQUIRED_RESULT QColor holidayColor() const;
    void setHolidayColor(const QColor &color);

    Q_REQUIRED_RESULT QColor agendaViewBackgroundColor() const;
    void setAgendaViewBackgroundColor(const QColor &color);

    Q_REQUIRED_RESULT QColor workingHoursColor() const;
    void setWorkingHoursColor(const QColor &color);

    Q_REQUIRED_RESULT QColor todoDueTodayColor() const;
    void setTodoDueTodayColor(const QColor &color);

    Q_REQUIRED_RESULT QColor todoOverdueColor() const;
    void setTodoOverdueColor(const QColor &color);

    Q_REQUIRED_RESULT QSet<EventViews::EventView::ItemIcon> agendaViewIcons() const;
    void setAgendaViewIcons(const QSet<EventViews::EventView::ItemIcon> &icons);

    Q_REQUIRED_RESULT QSet<EventViews::EventView::ItemIcon> monthViewIcons() const;
    void setMonthViewIcons(const QSet<EventViews::EventView::ItemIcon> &icons);

    void setFlatListTodo(bool);
    Q_REQUIRED_RESULT bool flatListTodo() const;

    void setFullViewTodo(bool);
    Q_REQUIRED_RESULT bool fullViewTodo() const;

    Q_REQUIRED_RESULT bool enableTodoQuickSearch() const;
    void setEnableTodoQuickSearch(bool enable);

    Q_REQUIRED_RESULT bool enableQuickTodo() const;
    void setEnableQuickTodo(bool enable);

    Q_REQUIRED_RESULT bool highlightTodos() const;
    void setHighlightTodos(bool);

    void setFirstDayOfWeek(const int day);
    Q_REQUIRED_RESULT int firstDayOfWeek() const;

    KConfig *config() const;

private:
    class Private;
    Private *const d;
};

typedef QSharedPointer<Prefs> PrefsPtr;
}

#endif

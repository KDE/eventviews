/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"
#include "eventviews_export.h"

#include <KConfigSkeleton>

#include <QTimeZone>

#include <memory>

namespace EventViews
{
class PrefsPrivate;

class EVENTVIEWS_EXPORT Prefs
{
public:
    /*!
      Creates an instance of Prefs with just base config
    */
    Prefs();

    /*!
      Creates an instance of Prefs with base config and application override config

      The passed \a appConfig will be queried for matching items whenever one of the
      accessors is called. If one is found it is used for setting/getting the value
      otherwise the one from the eventviews base config is used.
    */
    explicit Prefs(KCoreConfigSkeleton *appConfig);

    ~Prefs();

    void readConfig();
    void writeConfig();

public:
    void setMarcusBainsShowSeconds(bool showSeconds);
    [[nodiscard]] bool marcusBainsShowSeconds() const;

    void setAgendaMarcusBainsLineLineColor(const QColor &color);
    [[nodiscard]] QColor agendaMarcusBainsLineLineColor() const;

    void setMarcusBainsEnabled(bool enabled);
    [[nodiscard]] bool marcusBainsEnabled() const;

    void setAgendaMarcusBainsLineFont(const QFont &font);
    [[nodiscard]] QFont agendaMarcusBainsLineFont() const;

    void setHourSize(int size);
    [[nodiscard]] int hourSize() const;

    void setDayBegins(const QDateTime &dateTime);
    [[nodiscard]] QDateTime dayBegins() const;

    void setWorkingHoursStart(const QDateTime &dateTime);
    [[nodiscard]] QDateTime workingHoursStart() const;

    void setWorkingHoursEnd(const QDateTime &dateTime);
    [[nodiscard]] QDateTime workingHoursEnd() const;

    void setSelectionStartsEditor(bool startEditor);
    [[nodiscard]] bool selectionStartsEditor() const;

    void setUseSystemColor(bool useSystemColor);
    [[nodiscard]] bool useSystemColor() const;

    void setAgendaGridWorkHoursBackgroundColor(const QColor &color);
    [[nodiscard]] QColor agendaGridWorkHoursBackgroundColor() const;

    void setAgendaGridHighlightColor(const QColor &color);
    [[nodiscard]] QColor agendaGridHighlightColor() const;

    void setAgendaGridBackgroundColor(const QColor &color);
    [[nodiscard]] QColor agendaGridBackgroundColor() const;

    void setEnableAgendaItemIcons(bool enable);
    [[nodiscard]] bool enableAgendaItemIcons() const;

    void setTodosUseCategoryColors(bool useColors);
    [[nodiscard]] bool todosUseCategoryColors() const;

    void setAgendaHolidaysBackgroundColor(const QColor &color) const;
    [[nodiscard]] QColor agendaHolidaysBackgroundColor() const;

    void setAgendaViewColors(int colors);
    [[nodiscard]] int agendaViewColors() const;

    void setAgendaViewFont(const QFont &font);
    [[nodiscard]] QFont agendaViewFont() const;

    void setMonthViewFont(const QFont &font);
    [[nodiscard]] QFont monthViewFont() const;

    [[nodiscard]] QColor monthGridBackgroundColor() const;
    void setMonthGridBackgroundColor(const QColor &color);

    [[nodiscard]] QColor monthGridWorkHoursBackgroundColor() const;
    void monthGridWorkHoursBackgroundColor(const QColor &color);

    void setMonthViewColors(int colors) const;
    [[nodiscard]] int monthViewColors() const;

    [[nodiscard]] bool enableMonthItemIcons() const;
    void setEnableMonthItemIcons(bool enable);

    [[nodiscard]] bool showTimeInMonthView() const;
    void setShowTimeInMonthView(bool show);

    [[nodiscard]] bool showTodosMonthView() const;
    void setShowTodosMonthView(bool show);

    [[nodiscard]] bool showJournalsMonthView() const;
    void setShowJournalsMonthView(bool show);

    [[nodiscard]] bool fullViewMonth() const;
    void setFullViewMonth(bool fullView);

    [[nodiscard]] bool sortCompletedTodosSeparately() const;
    void setSortCompletedTodosSeparately(bool enable);

    void setEnableToolTips(bool enable);
    [[nodiscard]] bool enableToolTips() const;

    void setShowTodosAgendaView(bool show);
    [[nodiscard]] bool showTodosAgendaView() const;

    void setAgendaTimeLabelsFont(const QFont &font);
    [[nodiscard]] QFont agendaTimeLabelsFont() const;

    KConfigSkeleton::ItemFont *fontItem(const QString &name) const;

    void setResourceColor(const QString &, const QColor &);
    [[nodiscard]] QColor resourceColor(const QString &);
    [[nodiscard]] QColor resourceColorKnown(const QString &) const;

    [[nodiscard]] QTimeZone timeZone() const;

    [[nodiscard]] QStringList timeScaleTimezones() const;
    void setTimeScaleTimezones(const QStringList &timeZones);

    [[nodiscard]] bool use24HourClock() const;
    void setUse24HourClock(bool);

    [[nodiscard]] bool useDualLabels() const;
    void setUseDualLabels(bool);

    [[nodiscard]] QStringList selectedPlugins() const;
    void setSelectedPlugins(const QStringList &);

    [[nodiscard]] QStringList decorationsAtAgendaViewTop() const;
    void setDecorationsAtAgendaViewTop(const QStringList &);

    [[nodiscard]] QStringList decorationsAtAgendaViewBottom() const;
    void setDecorationsAtAgendaViewBottom(const QStringList &);

    [[nodiscard]] bool colorAgendaBusyDays() const;
    void setColorAgendaBusyDays(bool enable);

    [[nodiscard]] bool colorMonthBusyDays() const;
    void setColorMonthBusyDays(bool enable);

    [[nodiscard]] QColor viewBgBusyColor() const;
    void setViewBgBusyColor(const QColor &);

    [[nodiscard]] QColor holidayColor() const;
    void setHolidayColor(const QColor &color);

    [[nodiscard]] QColor agendaViewBackgroundColor() const;
    void setAgendaViewBackgroundColor(const QColor &color);

    [[nodiscard]] QColor workingHoursColor() const;
    void setWorkingHoursColor(const QColor &color);

    [[nodiscard]] QColor todoDueTodayColor() const;
    void setTodoDueTodayColor(const QColor &color);

    [[nodiscard]] QColor todoOverdueColor() const;
    void setTodoOverdueColor(const QColor &color);

    [[nodiscard]] QSet<EventViews::EventView::ItemIcon> agendaViewIcons() const;
    void setAgendaViewIcons(const QSet<EventViews::EventView::ItemIcon> &icons);

    [[nodiscard]] QSet<EventViews::EventView::ItemIcon> monthViewIcons() const;
    void setMonthViewIcons(const QSet<EventViews::EventView::ItemIcon> &icons);

    void setFlatListTodo(bool);
    [[nodiscard]] bool flatListTodo() const;

    void setFullViewTodo(bool);
    [[nodiscard]] bool fullViewTodo() const;

    [[nodiscard]] bool enableTodoQuickSearch() const;
    void setEnableTodoQuickSearch(bool enable);

    [[nodiscard]] bool enableQuickTodo() const;
    void setEnableQuickTodo(bool enable);

    [[nodiscard]] bool highlightTodos() const;
    void setHighlightTodos(bool);

    void setFirstDayOfWeek(const int day);
    [[nodiscard]] int firstDayOfWeek() const;

    KConfig *config() const;

private:
    std::unique_ptr<PrefsPrivate> const d;
};

using PrefsPtr = QSharedPointer<Prefs>;
}

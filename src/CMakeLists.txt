# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
add_definitions(-DTRANSLATION_DOMAIN=\"libeventviews6\")
add_library(KPim6EventViews)
add_library(KPim6::EventViews ALIAS KPim6EventViews)

if(KDEPIM_BUILD_EXAMPLES)
    add_subdirectory(viewerapp)
endif()

########### next target ###############

target_sources(
    KPim6EventViews
    PRIVATE
        eventview_p.cpp
        eventview.cpp
        helper.cpp
        prefs.cpp
        # Agenda view specific code.
        agenda/agenda.cpp
        agenda/agendaitem.cpp
        agenda/agendaview.cpp
        agenda/alternatelabel.cpp
        agenda/calendardecoration.cpp
        agenda/decorationlabel.cpp
        agenda/timelabels.cpp
        agenda/timelabelszone.cpp
        agenda/timescaleconfigdialog.cpp
        agenda/viewcalendar.cpp
        journal/journalframe.cpp
        journal/journalview.cpp
        list/listview.cpp
        month/monthgraphicsitems.cpp
        month/monthitem.cpp
        month/monthscene.cpp
        month/monthview.cpp
        multiagenda/multiagendaview.cpp
        todo/tododelegates.cpp
        todo/coloredtodoproxymodel.cpp
        todo/todoviewquickaddline.cpp
        todo/todoviewquicksearch.cpp
        todo/todoviewsortfilterproxymodel.cpp
        todo/todoviewview.cpp
        todo/todoview.cpp
        timeline/timelineview.cpp
        timeline/timelineitem.cpp
        timeline/timelineview_p.cpp
        whatsnext/whatsnextview.cpp
        eventview_p.h
        journal/journalframe.h
        journal/journalview.h
        list/listview.h
        eventview.h
        whatsnext/whatsnextview.h
        timeline/timelineview_p.h
        timeline/timelineview.h
        timeline/timelineitem.h
        todo/todoviewsortfilterproxymodel.h
        todo/todoviewview.h
        todo/todoviewquicksearch.h
        todo/tododelegates.h
        todo/todoview.h
        todo/todoviewquickaddline.h
        multiagenda/multiagendaview.h
        multiagenda/configdialoginterface.h
        agenda/timelabelszone.h
        agenda/agendaview.h
        agenda/timescaleconfigdialog.h
        agenda/alternatelabel.h
        agenda/agendaitem.h
        agenda/timelabels.h
        agenda/calendardecoration.h
        agenda/decorationlabel.h
        agenda/viewcalendar.h
        agenda/agenda.h
        month/monthview.h
        month/monthscene.h
        month/monthgraphicsitems.h
        month/monthitem.h
        helper.h
        prefs.h
)

kconfig_add_kcfg_files(KPim6EventViews prefs_base.kcfgc)
set_source_files_properties(
    prefs_base.cpp
    PROPERTIES
        SKIP_LINTING
            ON
)

ecm_qt_declare_logging_category(KPim6EventViews HEADER calendarview_debug.h
        IDENTIFIER CALENDARVIEW_LOG
        CATEGORY_NAME org.kde.pim.calendarview
        OLD_CATEGORY_NAMES log_calendarview
        DESCRIPTION "kdepim (eventviews)" EXPORT EVENTVIEWS
)

ki18n_wrap_ui(KPim6EventViews
  agenda/timescaleedit_base.ui
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(
        KPim6EventViews
        PROPERTIES
            UNITY_BUILD
                ON
    )
endif()
ecm_generate_export_header(KPim6EventViews
    BASE_NAME eventviews
    VERSION ${EVENTVIEWS_VERSION}
    DEPRECATED_BASE_VERSION 0
    USE_VERSION_HEADER
)

target_link_libraries(
    KPim6EventViews
    PUBLIC
        KPim6::AkonadiCore
        KPim6::CalendarSupport
        KF6::CalendarCore
        KPim6::CalendarUtils
        KPim6::AkonadiCalendar
        KF6::ItemModels
    PRIVATE
        KF6::Holidays
        KF6::Completion
        KF6::Service
        KF6::IconThemes
        KF6::GuiAddons
        KF6::I18n
        KF6::ColorScheme
        KPim6::Libkdepim
        KF6::Codecs
        KGantt6
        KPim6::AkonadiWidgets
        KF6::Contacts
)

target_include_directories(KPim6EventViews INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/EventViews/>")
target_include_directories(
    KPim6EventViews
    PUBLIC
        "$<BUILD_INTERFACE:${eventviews_SOURCE_DIR}/src;${eventviews_BINARY_DIR}/src;>"
)

set_target_properties(
    KPim6EventViews
    PROPERTIES
        VERSION
            ${EVENTVIEWS_VERSION}
        SOVERSION
            ${EVENTVIEWS_SOVERSION}
        EXPORT_NAME
            EventViews
)

install(TARGETS KPim6EventViews EXPORT KPim6EventViewsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_generate_headers(eventviews_CamelCase_HEADERS
  HEADER_NAMES
  Helper
  Prefs
  EventView
  REQUIRED_HEADERS eventviews_CamelCase_HEADERS
  PREFIX EventViews
)

ecm_generate_headers(eventviews_CamelCaseagenda_HEADERS
  HEADER_NAMES
  CalendarDecoration
  AgendaView
  ViewCalendar
  REQUIRED_HEADERS eventviews_agenda_HEADERS
  PREFIX EventViews
  RELATIVE agenda
)

ecm_generate_headers(eventviews_CamelCasetodo_HEADERS
  HEADER_NAMES
  TodoView
  REQUIRED_HEADERS eventviews_todo_HEADERS
  PREFIX EventViews
  RELATIVE todo
)

ecm_generate_headers(eventviews_CamelCasemonth_HEADERS
  HEADER_NAMES
  MonthView
  REQUIRED_HEADERS eventviews_month_HEADERS
  PREFIX EventViews
  RELATIVE month
)

ecm_generate_headers(eventviews_CamelCaselist_HEADERS
  HEADER_NAMES
  ListView
  REQUIRED_HEADERS eventviews_list_HEADERS
  PREFIX EventViews
  RELATIVE list
)

ecm_generate_headers(eventviews_CamelCasejournal_HEADERS
  HEADER_NAMES
  JournalView
  REQUIRED_HEADERS eventviews_journal_HEADERS
  PREFIX EventViews
  RELATIVE journal
)

ecm_generate_headers(eventviews_CamelCasemultiagenda_HEADERS
  HEADER_NAMES
  MultiAgendaView
  ConfigDialogInterface
  REQUIRED_HEADERS eventviews_multiagenda_HEADERS
  PREFIX EventViews
  RELATIVE multiagenda
)

ecm_generate_headers(eventviews_CamelCasetimeline_HEADERS
  HEADER_NAMES
  TimeLineView
  REQUIRED_HEADERS eventviews_timeline_HEADERS
  PREFIX EventViews
  RELATIVE timeline
)

ecm_generate_headers(eventviews_CamelCasewhatsnext_HEADERS
  HEADER_NAMES
  WhatsNextView
  REQUIRED_HEADERS eventviews_whatsnext_HEADERS
  PREFIX EventViews
  RELATIVE whatsnext
)

install(
    FILES
        ${eventviews_CamelCase_HEADERS}
        ${eventviews_CamelCaseagenda_HEADERS}
        ${eventviews_CamelCasetodo_HEADERS}
        ${eventviews_CamelCasewhatsnext_HEADERS}
        ${eventviews_CamelCasetimeline_HEADERS}
        ${eventviews_CamelCasemultiagenda_HEADERS}
        ${eventviews_CamelCasejournal_HEADERS}
        ${eventviews_CamelCaselist_HEADERS}
        ${eventviews_CamelCasemonth_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/EventViews/EventViews
    COMPONENT Devel
)

install(
    FILES
        ${eventviews_HEADERS}
        ${CMAKE_CURRENT_BINARY_DIR}/eventviews_export.h
        ${eventviews_CamelCase_HEADERS}
        ${eventviews_agenda_HEADERS}
        ${eventviews_todo_HEADERS}
        ${eventviews_whatsnext_HEADERS}
        ${eventviews_timeline_HEADERS}
        ${eventviews_multiagenda_HEADERS}
        ${eventviews_journal_HEADERS}
        ${eventviews_list_HEADERS}
        ${eventviews_month_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/EventViews/eventviews
    COMPONENT Devel
)

ecm_qt_install_logging_categories(EXPORT EVENTVIEWS FILE eventviews.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

if(BUILD_TESTING)
    add_subdirectory(month/autotests)
endif()
ecm_generate_qdoc(KPim6EventViews eventviews.qdocconf)

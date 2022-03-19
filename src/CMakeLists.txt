# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
add_definitions(-DTRANSLATION_DOMAIN=\"libeventviews\")
add_library(KF5EventViews)
add_library(KF5::EventViews ALIAS KF5EventViews)
if(KDEPIM_BUILD_EXAMPLES)
  add_subdirectory(viewerapp)
endif()

########### next target ###############

target_sources(KF5EventViews PRIVATE
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
  todo/todomodel.cpp

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
  todo/todomodel.h
  todo/todoviewquicksearch.h
  todo/tododelegates.h
  todo/todomodel_p.h
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

kconfig_add_kcfg_files(KF5EventViews prefs_base.kcfgc)

ecm_qt_declare_logging_category(KF5EventViews HEADER calendarview_debug.h
        IDENTIFIER CALENDARVIEW_LOG
        CATEGORY_NAME org.kde.pim.calendarview
        OLD_CATEGORY_NAMES log_calendarview
        DESCRIPTION "kdepim (eventviews)" EXPORT EVENTVIEWS)

ki18n_wrap_ui(KF5EventViews
  agenda/timescaleedit_base.ui
)


if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KF5EventViews PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KF5EventViews BASE_NAME eventviews)



target_link_libraries(KF5EventViews
PUBLIC
  KF5::AkonadiCore
  KF5::CalendarSupport

  KF5::CalendarCore
  KF5::CalendarUtils
  KF5::AkonadiCalendar
  KF5::ItemModels
PRIVATE
  KF5::Holidays
  KF5::Completion
  KF5::Service
  KF5::IconThemes
  KF5::GuiAddons
  KF5::I18n
  KF5::Libkdepim
  KF5::Codecs
  KGantt
  KF5::AkonadiWidgets
  KF5::Contacts
)

target_include_directories(KF5EventViews INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/EventViews/>")
target_include_directories(KF5EventViews PUBLIC "$<BUILD_INTERFACE:${eventviews_SOURCE_DIR}/src;${eventviews_BINARY_DIR}/src;>")

set_target_properties(KF5EventViews PROPERTIES
  VERSION ${EVENTVIEWS_VERSION}
  SOVERSION ${EVENTVIEWS_SOVERSION}
  EXPORT_NAME EventViews
)

install(TARGETS
  KF5EventViews
  EXPORT KF5EventViewsTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

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
  TodoModel
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

ecm_generate_pri_file(BASE_NAME EventViews
  LIB_NAME KF5EventViews
  DEPS "KMime AkonadiCalendar" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR
  ${KDE_INSTALL_INCLUDEDIR_KF}/EventViews
)

install(FILES
  ${eventviews_CamelCase_HEADERS}
  ${eventviews_CamelCaseagenda_HEADERS}
  ${eventviews_CamelCasetodo_HEADERS}
  ${eventviews_CamelCasewhatsnext_HEADERS}
  ${eventviews_CamelCasetimeline_HEADERS}
  ${eventviews_CamelCasemultiagenda_HEADERS}
  ${eventviews_CamelCasejournal_HEADERS}
  ${eventviews_CamelCaselist_HEADERS}
  ${eventviews_CamelCasemonth_HEADERS}
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/EventViews/EventViews
  COMPONENT Devel
)

install(FILES
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
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/EventViews/eventviews
  COMPONENT Devel
)

install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
ecm_qt_install_logging_categories(EXPORT EVENTVIEWS FILE eventviews.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

if (BUILD_QCH)
    ecm_add_qch(
        KF5EventViews_QCH
        NAME KF5EventViews
        BASE_NAME KF5EventViews
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
        ${eventviews_CamelCase_HEADERS}
        ${eventviews_agenda_HEADERS}
        ${eventviews_todo_HEADERS}
        ${eventviews_whatsnext_HEADERS}
        ${eventviews_timeline_HEADERS}
        ${eventviews_multiagenda_HEADERS}
        ${eventviews_journal_HEADERS}
        ${eventviews_list_HEADERS}
        ${eventviews_month_HEADERS}
        #MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt5Core_QCH
            Qt5Gui_QCH
            Qt5Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            EVENTVIEWS_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()

install(FILES agenda/calendardecoration.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPESDIR})

if(BUILD_TESTING)
    add_subdirectory(month/autotests)
endif()

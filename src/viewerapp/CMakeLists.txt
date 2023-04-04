# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
set(viewerapp_SRCS
  main.cpp
  mainwindow.cpp
  ../calendarview_debug.cpp
)

kconfig_add_kcfg_files(viewerapp_SRCS settings.kcfgc)

ki18n_wrap_ui(viewerapp_SRCS
  mainwindow.ui
)

add_executable(viewerapp ${viewerapp_SRCS})

target_link_libraries(viewerapp
  KPim6::CalendarSupport
  KPim6::EventViews
  KPim6::AkonadiWidgets
  KF6::CalendarCore
  KPim6::AkonadiCalendar
  KF6::Holidays
)

########### install files ###############

#install(TARGETS viewerapp ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#install(PROGRAMS viewerapp.desktop  DESTINATION ${KDE_INSTALL_APPDIR})

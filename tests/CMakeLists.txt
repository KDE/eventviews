# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
add_executable(createcolorgui_test createcolorgui_test.cpp createcolorgui_test.h)
ecm_mark_as_test(createcolorgui_test)
target_link_libraries(createcolorgui_test
  Qt${QT_MAJOR_VERSION}::Core
  Qt${QT_MAJOR_VERSION}::Gui
  KF5::EventViews
  KF5::I18n
)


# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
add_executable(
    createcolorgui_test
    createcolorgui_test.cpp
    createcolorgui_test.h
)
ecm_mark_as_test(createcolorgui_test)
target_link_libraries(
    createcolorgui_test
    Qt::Core
    Qt::Gui
    KPim6::EventViews
    KF6::I18n
    KF6::ConfigGui
)

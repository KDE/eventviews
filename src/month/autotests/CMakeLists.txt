# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(monthitemordertest.cpp LINK_LIBRARIES Qt::Test KPim${KF_MAJOR_VERSION}::EventViews)

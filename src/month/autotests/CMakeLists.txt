set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(monthitemordertest.cpp LINK_LIBRARIES Qt::Test KF5::EventViews)

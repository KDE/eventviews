set(createcolorgui_test createcolorgui_test.cpp)
add_executable(createcolorgui_test ${createcolorgui_test})
ecm_mark_as_test(createcolorgui_test)
target_link_libraries(createcolorgui_test
  Qt::Core
  Qt::Gui
  KF5::EventViews
  KF5::I18n
)


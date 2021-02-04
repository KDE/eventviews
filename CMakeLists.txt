cmake_minimum_required(VERSION 3.5)
set(PIM_VERSION "5.16.3")

project(eventviews VERSION ${PIM_VERSION})

# ECM setup
set(KF5_MIN_VERSION "5.75.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW)
endif()

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)

include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)


set(EVENTVIEW_LIB_VERSION ${PIM_VERSION})
set(CALENDARUTILS_LIB_VERSION "5.16.2")
set(AKONADI_LIB_VERSION "5.16.2")
set(QT_REQUIRED_VERSION "5.13.0")
set(AKONADICALENDAR_LIB_VERSION "5.16.2")
set(KMIME_LIB_VERSION "5.16.2")
set(LIBKDEPIM_LIB_VERSION "5.16.2")
set(CALENDARSUPPORT_LIB_VERSION "5.16.2")

find_package(KF5Akonadi ${AKONADI_LIB_VERSION} CONFIG REQUIRED)
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets)
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5GuiAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Service ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Completion ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Holidays ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KF5_MIN_VERSION} CONFIG REQUIRED)

set(KDIAGRAM_LIB_VERSION "1.4.0")
find_package(KGantt ${KDIAGRAM_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarSupport ${CALENDARSUPPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

ecm_setup_version(PROJECT VARIABLE_PREFIX EVENTVIEWS
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/eventviews_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5EventViewsConfigVersion.cmake"
                        SOVERSION 5
)

########### Targets ###########


add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054C00)

add_definitions(-DQT_NO_FOREACH)
add_definitions(-DQT_NO_KEYWORDS)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5EventViews")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5EventViewsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5EventViewsConfig.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5EventViewsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5EventViewsConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5EventViewsTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5EventViewsTargets.cmake NAMESPACE KF5::)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/eventviews_version.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5} COMPONENT Devel
)

add_subdirectory(src)
if(BUILD_TESTING)
  find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Test)
  add_subdirectory(tests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

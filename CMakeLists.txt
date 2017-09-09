cmake_minimum_required(VERSION 3.0)
set(PIM_VERSION "5.6.42")

project(eventviews VERSION ${PIM_VERSION})

# ECM setup
set(KF5_VERSION "5.37.0")
option(RUN_UNINSTALL "test run uninstalled apps (experimental)" FALSE)

set(ECM_VERSION ${KF5_VERSION})
# ECM setup
if (RUN_UNINSTALL)
   set(ECM_VERSION "5.38.0")
endif()

find_package(ECM ${ECM_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(LIBRARY_NAMELINK)
if (POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(ECMCoverageOption)




set(EVENTVIEW_LIB_VERSION ${PIM_VERSION})
set(CALENDARUTILS_LIB_VERSION "5.6.42")
set(AKONADI_LIB_VERSION "5.6.40")
set(QT_REQUIRED_VERSION "5.8.0")
set(KCALENDARCORE_LIB_VERSION "5.6.40")
set(AKONADICALENDAR_LIB_VERSION "5.6.40")
set(KMIME_LIB_VERSION "5.6.40")
set(LIBKDEPIM_LIB_VERSION "5.6.40")
set(CALENDARSUPPORT_LIB_VERSION "5.6.42")

find_package(KF5Akonadi ${AKONADI_LIB_VERSION} CONFIG REQUIRED)
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets UiTools)
find_package(KF5I18n ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5LibkdepimAkonadi ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)

set(KDIAGRAM_LIB_VERSION "1.4.0")
find_package(KGantt ${KDIAGRAM_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarSupport ${CALENDARSUPPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

ecm_setup_version(PROJECT VARIABLE_PREFIX EVENTVIEWS
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/eventviews_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5EventViewsConfigVersion.cmake"
                        SOVERSION 5
)

########### Targets ###########

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
#add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5EventViews")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5EventViewsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5EventViewsConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
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
if (BUILD_TESTING)
    find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Test)
   add_subdirectory(tests)
endif()
install( FILES eventviews.renamecategories eventviews.categories DESTINATION ${KDE_INSTALL_CONFDIR} )
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

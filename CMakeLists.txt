# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "6.5.40")

project(eventviews VERSION ${PIM_VERSION})

# ECM setup
set(KF_MIN_VERSION "6.15.0")
set(QT_REQUIRED_VERSION "6.8.0")

find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)
file(
    GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES
    src/*.cpp
    src/*.h
    tests/*.cpp
    tests/*.h
)
set(PRE_COMMIT_INSTALLED FALSE)
if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    if(EXISTS "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit")
        file(READ "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit" FILE_CONTENTS)
        string(FIND "${FILE_CONTENTS}" "File generated by pre-commit" INDEX)
        if(${INDEX} GREATER_EQUAL 0)
            set(PRE_COMMIT_INSTALLED TRUE)
        endif()
    endif()
endif()
if(NOT ${PRE_COMMIT_INSTALLED})
    include(KDEGitCommitHooks)
    include(KDEClangFormat)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
endif()
include(ECMDeprecationSettings)
include(ECMFeatureSummary)
include(ECMAddTests)
include(ECMCheckOutboundLicense)
include(ECMAddQch)
option(
    BUILD_QCH
    "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)"
    OFF
)
add_feature_info(
    QCH
    ${BUILD_QCH}
    "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)"
)

set(EVENTVIEW_LIB_VERSION ${PIM_VERSION})
set(CALENDARUTILS_LIB_VERSION "6.5.40")
set(AKONADI_LIB_VERSION "6.5.40")
set(AKONADICALENDAR_LIB_VERSION "6.5.40")
set(KMIME_LIB_VERSION "6.5.40")
set(LIBKDEPIM_LIB_VERSION "6.5.40")
set(CALENDARSUPPORT_LIB_VERSION "6.5.40")

find_package(KPim6Akonadi ${AKONADI_LIB_VERSION} CONFIG REQUIRED)
find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets)
find_package(KF6I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Codecs ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6GuiAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6IconThemes ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ColorScheme ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ItemModels ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Service ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Completion ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Holidays ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Contacts ${KF_MIN_VERSION} CONFIG REQUIRED)

set(KDIAGRAM_LIB_VERSION "3.0.0")
find_package(KGantt6 ${KDIAGRAM_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim6Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF6CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(
    KPim6CalendarSupport
    ${CALENDARSUPPORT_LIB_VERSION}
    CONFIG
    REQUIRED
)
find_package(
    KPim6AkonadiCalendar
    ${AKONADICALENDAR_LIB_VERSION}
    CONFIG
    REQUIRED
)
find_package(KPim6Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

ecm_setup_version(PROJECT VARIABLE_PREFIX EVENTVIEWS
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/eventviews_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KPim6EventViewsConfigVersion.cmake"
                        SOVERSION 6
)
option(
    USE_UNITY_CMAKE_SUPPORT
    "Use UNITY cmake support (speedup compile time)"
    OFF
)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if(USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()

########### Targets ###########
add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
if(BUILD_TESTING)
    add_definitions(-DBUILD_TESTING)
endif()

ecm_set_disabled_deprecation_versions(QT 6.10.0
     KF 6.17.0
)
add_subdirectory(src)
if(BUILD_TESTING)
    find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Test)
    add_subdirectory(tests)
endif()

########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KPim6EventViews")
if(BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KPim6EventViews_QCH
        FILE KPim6EventViewsQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/KPim6EventViewsQchTargets.cmake\")"
    )
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KPimEventViewsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KPim6EventViewsConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/KPim6EventViewsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/KPim6EventViewsConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(
    EXPORT KPim6EventViewsTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KPim6EventViewsTargets.cmake
    NAMESPACE KPim6::
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/src/eventviews_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/EventViews
    COMPONENT Devel
)

if(DEFINED kde_configure_git_pre_commit_hook)
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()

ki18n_install(po)
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp src/*.h tests/*.cpp tests/*.h)
ecm_check_outbound_license(LICENSES GPL-2.0-only  FILES ${ALL_SOURCE_FILES})

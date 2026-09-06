cmake_minimum_required(VERSION 3.21)

project(qcma VERSION 0.5.3 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

option(QCMA_INCLUDE_GIT_REVISION "Include the current Git revision in build metadata" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Network Sql Widgets LinguistTools)
find_package(VitaMTP 2.5.12 CONFIG REQUIRED)
if(WIN32)
    find_package(FFMPEG REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
endif()

set(QCMA_VERSION "${PROJECT_VERSION}")
if(QCMA_INCLUDE_GIT_REVISION)
    execute_process(COMMAND git describe --tags --always --dirty
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE QCMA_GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(QCMA_GIT_VERSION)
        set(QCMA_VERSION "${QCMA_GIT_VERSION}")
    endif()
endif()
add_compile_definitions(QCMA_VER="${QCMA_VERSION}" __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS)
if(UNIX AND NOT APPLE)
    add_compile_definitions(_FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE)
endif()

set(QCMA_TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/common/resources/translations/qcma_es.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/resources/translations/qcma_fr.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/resources/translations/qcma_ja.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/resources/translations/qcma_cn.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/resources/translations/qcma_it.ts")

add_subdirectory(common)
add_subdirectory(gui)
if(UNIX AND NOT APPLE)
    add_subdirectory(cli)
endif()

if(BUILD_TESTING)
    add_test(NAME qcma_version COMMAND qcma --version)
    set_tests_properties(qcma_version PROPERTIES PASS_REGULAR_EXPRESSION "qcma 0.5.3")
    if(TARGET qcma_cli)
        add_test(NAME qcma_cli_version COMMAND qcma_cli --version)
        set_tests_properties(qcma_cli_version PROPERTIES PASS_REGULAR_EXPRESSION "qcma_cli 0.5.3")
    endif()
endif()

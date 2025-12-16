cmake_minimum_required(VERSION 3.14)
project(dbus-canopen-motordrive LANGUAGES C CXX)

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Building with GCC code coverage enabled")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(COVERAGE_FLAGS
            -O0
            --coverage
        )

        # Add coverage flags to all targets
        add_compile_options(${COVERAGE_FLAGS})
        add_link_options(${COVERAGE_FLAGS})
    else()
        message(FATAL_ERROR "Coverage is only supported with GCC in this configuration")
    endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
find_package(PkgConfig REQUIRED)
pkg_check_modules(EVENT REQUIRED libevent)

include_directories(${DBUS_INCLUDE_DIRS})
include_directories(${EVENT_INCLUDE_DIRS})

add_library(dbus-canopen-motordrive-lib
    ../src/list.c
    ../src/array.c
    ../src/device.c
    ../src/localsettings.c
    ../src/canopen.c
    ../src/discovery.c
    ../src/drivers/sevcon.c
    ../src/drivers/curtis_e.c
    ../src/drivers/curtis_f.c
    ../src/node.c
    ../src/servicemanager.c

    ../velib/task/task/platform_init.c
    ../velib/task/task/console_no_arguments.c
    ../velib/src/utils/ve_logger.c
    ../velib/src/types/ve_stamp.c
    ../velib/src/types/ve_str.c
    ../velib/src/types/ve_variant.c
    ../velib/src/types/ve_variant_print.c
    ../velib/src/types/ve_item.c
    ../velib/src/utils/ve_item_utils.c
    ../velib/src/utils/ve_timer.c
    ../velib/src/vecan/products.c
    ../velib/src/io/ve_iostream_items_le.c
    ../velib/src/io/ve_iostream_items.c
    ../velib/src/io/ve_istream.c
    ../velib/src/io/ve_ostream.c
    ../velib/src/io/ve_istream_le.c
    ../velib/src/io/ve_ostream_le.c
    ../velib/src/io/ve_istream_variant_le.c
    ../velib/src/base/ve_string.c
)

target_include_directories(dbus-canopen-motordrive-lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../inc
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../velib/inc
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../velib/src/types
)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

# Prevent GoogleTest from forcing its own compiler flags
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(dbus-canopen-motordrive-test
    environment.cpp
    test_list.cpp
    test_array.cpp
    test_device.cpp
    test_canopen.cpp
    test_localsettings.cpp
    test_discovery.cpp
    test_servicemanager.cpp
    test_node.cpp
    test_sevcon.cpp
    test_curtis_f.cpp
    test_curtis_e.cpp
)

target_link_directories(dbus-canopen-motordrive-test PRIVATE
    /opt/homebrew/lib
)

target_link_libraries(
    dbus-canopen-motordrive-test
    dbus-canopen-motordrive-lib
    gtest
    gtest_main
    ${DBUS_LIBRARIES}
    ${EVENT_LIBRARIES}
)

add_test(NAME Test COMMAND dbus-canopen-motordrive-test)
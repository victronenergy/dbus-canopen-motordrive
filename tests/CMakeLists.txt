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

# Build your C code into a library
add_library(dbus-canopen-motordrive-lib
    ../src/list.c
    ../src/array.c
    ../src/device.c
    ../src/localsettings.c

    ../velib/task/task/platform_init.c
    ../velib/task/task/console_no_arguments.c
    ../velib/src/utils/ve_logger.c
    ../velib/src/types/ve_stamp.c
    ../velib/src/types/ve_str.c
    ../velib/src/types/ve_variant.c
    ../velib/src/types/ve_variant_print.c
    ../velib/src/types/ve_item.c
    ../velib/src/utils/ve_item_utils.c
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

# add_library(velib
#     ../velib/task/task/platform_init.c
#     ../velib/task/task/console_no_arguments.c
#     ../velib/src/utils/ve_logger.c
#     ../velib/src/types/ve_stamp.c
#     ../velib/src/types/ve_str.c
#     ../velib/src/types/ve_variant.c
# )

target_include_directories(dbus-canopen-motordrive-lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../inc
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../velib/inc
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../velib/src/types
)

# target_include_directories(velib
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/../inc
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/../velib/inc
# )

# Enable fetching external projects like GoogleTest
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

# Add a custom target for generating coverage reports
# if ("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage") # Check if the build type is 'Coverage'
#   find_package(Python3 REQUIRED COMPONENTS Interpreter)  # Required for gcovr
#   add_custom_target(coverage
#     COMMAND ${CMAKE_COMMAND} -E make_directory coverage
#     COMMAND ${Python3_EXECUTABLE} -m gcovr -r ${CMAKE_SOURCE_DIR} --html --html-nested -o coverage/index.html
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#     COMMENT "Generating HTML coverage report..."
#   )

# set(CMAKE_CXX_FLAGS "--coverage -g -O0 ${CMAKE_CXX_FLAGS}")
# set(CMAKE_C_FLAGS "--coverage -g -O0 ${CMAKE_C_FLAGS}")
# set(CMAKE_EXE_LINKER_FLAGS "--coverage -g -O0 -lgcov ${CMAKE_EXE_LINKER_FLAGS}")
# endif()

# Create the test executable
add_executable(dbus-canopen-motordrive-test
    environment.cpp
    test_list.cpp
    test_array.cpp
    test_device.cpp
)

target_link_directories(dbus-canopen-motordrive-test PRIVATE
    /opt/homebrew/lib
)

# Link test executable with:
# - your C library
# - GoogleTest framework
target_link_libraries(
    dbus-canopen-motordrive-test
    dbus-canopen-motordrive-lib
    gtest
    gtest_main
    ${DBUS_LIBRARIES}
    ${EVENT_LIBRARIES}
)

# Register the test with CTest
add_test(NAME Test COMMAND dbus-canopen-motordrive-test)
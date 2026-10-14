cmake_minimum_required(VERSION 3.16)
project(price_alert LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
    )
endif()

# Core library sources
set(CORE_SOURCES
    src/date_utils.cpp
    src/csv_parser.cpp
    src/json_parser.cpp
    src/alert_engine.cpp
    src/output_formatter.cpp
)

# Main CLI Executable
add_executable(price_alert
    src/main.cpp
    ${CORE_SOURCES}
)

target_include_directories(price_alert PRIVATE
    include
    third_party
    .
)

# Test Executable
add_executable(run_tests
    tests/test_main.cpp
    tests/test_date_utils.cpp
    tests/test_csv_parser.cpp
    tests/test_json_parser.cpp
    tests/test_alert_engine.cpp
    tests/test_integration.cpp
    ${CORE_SOURCES}
)

target_include_directories(run_tests PRIVATE
    include
    tests
    third_party
    .
)

enable_testing()
add_test(NAME TestSuite COMMAND run_tests)
set_tests_properties(TestSuite PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_test(NAME CliContractTest COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.sh")
set_tests_properties(CliContractTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

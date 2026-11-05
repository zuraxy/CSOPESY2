cmake_minimum_required(VERSION 3.16)
project(CSOPESYEmulator LANGUAGES CXX)

find_package(Threads REQUIRED)

add_library(emulator src/scheduler.cpp)
target_compile_features(emulator PUBLIC cxx_std_17)
set_target_properties(emulator PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(emulator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(console_runtime src/console.cpp)
target_link_libraries(console_runtime PUBLIC emulator Threads::Threads)
set_target_properties(console_runtime PROPERTIES CXX_EXTENSIONS OFF)

add_executable(csopesy src/main.cpp)
target_link_libraries(csopesy PRIVATE console_runtime)
set_target_properties(csopesy PROPERTIES CXX_EXTENSIONS OFF)

add_executable(case6_runner runners/case6_runner.cpp)
target_link_libraries(case6_runner PRIVATE console_runtime)
set_target_properties(case6_runner PROPERTIES CXX_EXTENSIONS OFF)

# Runtime data follows each executable, including Debug/Release output folders.
foreach(target csopesy case6_runner)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/config.txt" "$<TARGET_FILE_DIR:${target}>/config.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/configs" "$<TARGET_FILE_DIR:${target}>/configs"
        VERBATIM)
endforeach()

include(CTest)
if(BUILD_TESTING)
    add_executable(scheduler_tests src/scheduler_tests.cpp)
    target_link_libraries(scheduler_tests PRIVATE emulator)
    add_test(NAME scheduler_tests COMMAND scheduler_tests)
    set_tests_properties(scheduler_tests PROPERTIES LABELS unit)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME cli_integration
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_tests.py"
                --app "$<TARGET_FILE:csopesy>" --runner "$<TARGET_FILE:case6_runner>")
        set_tests_properties(cli_integration PROPERTIES LABELS integration TIMEOUT 30
            ENVIRONMENT "CSOPESY_TEST_TMP=${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()

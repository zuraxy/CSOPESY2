cmake_minimum_required(VERSION 3.16)
project(CSOPESYEmulator LANGUAGES CXX)

find_package(Threads REQUIRED)

add_library(emulator scheduler.cpp)
target_compile_features(emulator PUBLIC cxx_std_17)
set_target_properties(emulator PROPERTIES CXX_EXTENSIONS OFF)

add_executable(csopesy main.cpp)
target_link_libraries(csopesy PRIVATE emulator Threads::Threads)
set_target_properties(csopesy PROPERTIES CXX_EXTENSIONS OFF)
configure_file(config.txt config.txt COPYONLY)

include(CTest)
if(BUILD_TESTING)
    add_executable(scheduler_tests scheduler_tests.cpp)
    target_link_libraries(scheduler_tests PRIVATE emulator)
    add_test(NAME scheduler_tests COMMAND scheduler_tests)
endif()

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(BreakableToy VERSION 0.1.0 LANGUAGES C CXX)

option(BT_BUILD_TESTS "Build tests" ON)

# C11 standard, no extensions
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# C++20 standard, no extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(BT_BUILD_TESTS)
    include(CTest)
    mark_as_advanced(BUILD_TESTING)
endif()

add_subdirectory(src)
add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party third_party)

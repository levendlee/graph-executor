cmake_minimum_required(VERSION 3.5)
project(graph_executor VERSION 0.0.1 LANGUAGES CXX)

include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
    doctest
    PREFIX ${CMAKE_BINARY_DIR}/doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

# Expose required variable (DOCTEST_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(doctest source_dir)
set(DOCTEST_INCLUDE_DIR ${source_dir}/doctest CACHE INTERNAL "Path to include folder for doctest")
include_directories(${DOCTEST_INCLUDE_DIR})

set (CMAKE_CXX_STANDARD 17)

add_library(context STATIC context.cpp)
add_library(node STATIC node.cpp)
add_library(graph STATIC graph.cpp)

# Make test executable
add_executable(graph_test graph_test.cpp doctest)
set_target_properties(graph_test PROPERTIES ENABLE_EXPORTS 1)
target_include_directories(graph_test PRIVATE ${DOCTEST_INCLUDE_DIR})
target_link_libraries(graph_test PRIVATE context node graph)

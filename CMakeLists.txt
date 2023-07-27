cmake_minimum_required(VERSION 3.2)

project(
    async_http
    VERSION "0.1"
    HOMEPAGE_URL "https://github.com/ollirelander/async_http"
    DESCRIPTION "A modern, simple, fast, and single-header C++17 library for performing HTTP GET/POST requests with customizable options."
    LANGUAGES CXX
)

option(ASYNC_HTTP_OPT_BUILD_EXAMPLES "Build async_http examples" ON)

if(ASYNC_HTTP_OPT_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

add_library(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/async_http.hpp")
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

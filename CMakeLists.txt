cmake_minimum_required(VERSION 3.8)
project(StructuredInterface)

if (NOT TARGET clean-core)
    message(FATAL_ERROR "[structured-interface] clean-core must be available")
endif()

# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")

arcana_add_library(SI structured-interface SOURCES HEADERS)

target_include_directories(structured-interface PUBLIC src/)

target_link_libraries(structured-interface PUBLIC
    clean-core
    typed-geometry
    rich-log
    reflector
)

find_package(Threads REQUIRED)
target_link_libraries(structured-interface PRIVATE Threads::Threads)

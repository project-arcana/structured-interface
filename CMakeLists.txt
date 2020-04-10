cmake_minimum_required(VERSION 3.8)
project(StructuredInterfcae)

include(cmake/UnityBuild.cmake)
include(cmake/SourceGroup.cmake)

# =========================================
# global options


# =========================================
# define library

file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.hh")

arcana_source_group(SOURCES)

add_library(structured-interface STATIC ${SOURCES})

target_include_directories(structured-interface PUBLIC src/)

target_link_libraries(structured-interface PUBLIC
    clean-core
    typed-geometry
    rich-log
    reflector
)

find_package(Threads REQUIRED)
target_link_libraries(structured-interface PRIVATE Threads::Threads)

# =========================================
# set up compile flags

if (MSVC)
    target_compile_options(structured-interface PUBLIC /MP)
else()
    target_compile_options(structured-interface PRIVATE -Wall -fPIC)
    target_link_libraries(structured-interface PUBLIC -fuse-ld=gold)
endif()

cmake_minimum_required(VERSION 3.13)
project(NodeMatcherPlugin)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(NodeMatcherPlugin MODULE
    NodeMatcherPlugin.cpp
    QueryParser.cpp
    Query.cpp
)

set_target_properties(NodeMatcherPlugin PROPERTIES
    OUTPUT_NAME NodeMatcherPlugin
    PREFIX ""
)

target_compile_options(NodeMatcherPlugin PRIVATE -Os)

target_link_libraries(NodeMatcherPlugin
    PRIVATE
    clang-cpp
    LLVM
)

set_target_properties(NodeMatcherPlugin PROPERTIES
    LINK_WHAT_YOU_USE TRUE
)

if(APPLE)
    set_target_properties(NodeMatcherPlugin PROPERTIES
        SUFFIX ".dylib"
        LINK_FLAGS "-undefined dynamic_lookup -dead_strip"
    )
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        set_target_properties(NodeMatcherPlugin PROPERTIES
            COMPILE_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
        )
    endif()
else()
    set_target_properties(NodeMatcherPlugin PROPERTIES
        SUFFIX ".so"
    )
endif()

# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.cpp")

idf_component_register(SRCS ${app_sources})

# list(APPEND kconfigs "${CMAKE_SOURCE_DIR}/src/Kconfig.projbuild")
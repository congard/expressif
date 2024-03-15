FILE(GLOB_RECURSE common_sources
        "src/*.h"
        "src/*.c"
        "src/*.cpp"
        "include/*.h")

idf_component_register(SRCS ${common_sources}
        INCLUDE_DIRS "include")
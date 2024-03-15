FILE(GLOB_RECURSE http_server_sources
        "src/*.h"
        "src/*.c"
        "src/*.cpp"
        "include/*.h")

idf_component_register(SRCS ${http_server_sources}
        INCLUDE_DIRS "include"
        REQUIRES esp_http_server exp_common)
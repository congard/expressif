cmake_minimum_required(VERSION 3.16.0)
set(CMAKE_CXX_STANDARD 20)

# workaround for https://github.com/espressif/esp-idf/issues/3920
set(PROJECT_VER 1)

if (DEFINED ENV{EXPRESSIF_PATH})
    set(EXPRESSIF_PATH $ENV{EXPRESSIF_PATH})
else()
    set(EXPRESSIF_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
endif()

list(APPEND EXTRA_COMPONENT_DIRS "${EXPRESSIF_PATH}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(ExpressifWiFiExample LANGUAGES CXX)

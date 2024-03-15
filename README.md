# Expressif

![stability-wip](https://img.shields.io/badge/stability-wip-lightgrey.svg)

Expressif is a library for ESP32 boards that provides C++ interface for esp-idf functions.
Nevertheless, Expressif is not just a wrapper for esp-idf: it offers additional functionality
that speeds up development for ESP32 boards.

Expressif is written in modern C++ (C++20) to utilize the whole power of this powerful language.

## Components

The library consists of components, which can be found in the corresponding directory (`components`)

The following components are available now:

| Component                                       | Description                                 |
|-------------------------------------------------|---------------------------------------------|
| [`exp_http_server`](components/exp_http_server) | HTTP Server with REST API support           |
| [`exp_wifi`](components/exp_wifi)               | Establishes Wi-Fi connection in an easy way |

## Usage

Append the `components` directory to the `EXTRA_COMPONENT_DIRS` list in your main `CMakeLists.txt`, e.g.:

```cmake
# /CMakeLists.txt

cmake_minimum_required(VERSION 3.16.0)
set(CMAKE_CXX_STANDARD 20)

# workaround for https://github.com/espressif/esp-idf/issues/3920
set(PROJECT_VER 1)

if (DEFINED ENV{EXPRESSIF_PATH})
    set(EXPRESSIF_PATH $ENV{EXPRESSIF_PATH})
else()
    message(FATAL_ERROR "Environment variable EXPRESSIF_PATH is not specified")
endif()

# add this line before `include`
list(APPEND EXTRA_COMPONENT_DIRS "${EXPRESSIF_PATH}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(ESPIDFSandbox LANGUAGES CXX)
```

More examples can be found in the [corresponding folder](examples).

## Tested boards

Expressif has been tested on the following boards:

|                   | ESP32-C3 SuperMini | ESP32-S3-DevKitC-1 N16R8 |
|-------------------|--------------------|--------------------------|
| `exp_http_server` | ✅                  | ✅                        |
| `exp_wifi`        | ✅                  | ✅                        |

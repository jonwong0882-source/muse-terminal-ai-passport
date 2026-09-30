cmake_minimum_required(VERSION 3.16)
if("$ENV{IDF_COMPONENT_MANAGER}" STREQUAL "0")
    set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/build/offline-components")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(FoloToy-AI-Passport)

#
# SPDX-License-Identifier: MIT
# Copyright (c) 2021-present Guillermo Trabes
# ARSLab - Carleton University
# Copyright (c) 2021-present jsoulier
# ARSLab - Carleton University
# Copyright (c) 2021-present Román Cárdenas Rodríguez
# ARSLab - Carleton University
#

cmake_minimum_required(VERSION 3.24)
project(cadmium)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_library(cadmium INTERFACE)
target_include_directories(cadmium INTERFACE include)
target_link_libraries(cadmium INTERFACE nlohmann_json unordered_dense)

if(USE_TRACY)
    target_link_libraries(cadmium INTERFACE Tracy::TracyClient)
    target_compile_definitions(cadmium INTERFACE USE_TRACY=1)
endif()

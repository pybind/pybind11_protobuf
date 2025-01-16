cmake_minimum_required(VERSION 3.18)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(pybind11_protobuf LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  if(MSVC)
    set(CMAKE_CXX_STANDARD 20)
  else()
    set(CMAKE_CXX_STANDARD 17)
  endif()
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================================
# Options

option(USE_SYSTEM_ABSEIL "Force usage of system provided abseil-cpp" OFF)
option(USE_SYSTEM_PROTOBUF "Force usage of system provided Protobuf" OFF)
option(USE_SYSTEM_PYBIND "Force usage of system provided pybind11" OFF)

# ============================================================================
# Testing
include(CTest)
option(PYBIND11_PROTOBUF_BUILD_TESTING
       "If ON, build all of pybind11_protobuf's own tests." ${BUILD_TESTING})

# ============================================================================
# Find Python
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

# ============================================================================
# Build dependencies
add_subdirectory(cmake/dependencies dependencies)

set(TOP_LEVEL_DIR ${CMAKE_CURRENT_LIST_DIR})
include_directories(${TOP_LEVEL_DIR} ${pybind11_INCLUDE_DIRS})
add_subdirectory(pybind11_protobuf)

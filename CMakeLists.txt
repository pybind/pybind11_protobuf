# The minimum CMake version and C++ standard follow Google's Foundational C++
# Support matrix; keep them in sync with it:
# https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md
cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(pybind11_protobuf LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  cmake_policy(SET CMP0135 NEW)
endif()

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

# Place runtime artifacts (such as protoc.exe and shared Abseil/Protobuf DLLs on
# Windows) in a single directory so build-time protoc invocations find their
# dependent DLLs.
if(WIN32 AND NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# ============================================================================
# Build dependencies
add_subdirectory(cmake/dependencies dependencies)

set(TOP_LEVEL_DIR ${CMAKE_CURRENT_LIST_DIR})
include_directories(${TOP_LEVEL_DIR} ${pybind11_INCLUDE_DIRS})

# Makes extension module `target` importable from the build tree:
#
# * Multi-config generators (e.g. Xcode, Visual Studio) put build outputs in a
#   per-configuration subdirectory, outside the Python package directory. A
#   generator expression in the output directory disables that subdirectory.
# * On Windows, Python loads the DLLs an extension module depends on from the
#   module's directory, not from PATH, so copy them there. There are none if
#   dependencies are linked statically, and `copy_if_different` fails without
#   files, so run `cmake -E true` instead then.
function(pybind11_protobuf_build_tree_module target)
  set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                             "$<1:${CMAKE_CURRENT_BINARY_DIR}>")
  if(WIN32)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E
        $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>,copy_if_different,true>
        $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
      COMMAND_EXPAND_LISTS)
  endif()
endfunction()

add_subdirectory(pybind11_protobuf)

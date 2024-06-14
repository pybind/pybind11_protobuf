cmake_minimum_required(VERSION 3.18)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(pybind11_protobuf)

if(MSVC)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================================
# Options

option(BUILD_TESTS "Build tests." OFF)
option(ENABLE_PYPROTO_API "Enable usage of proto_api." OFF)
option(USE_SYSTEM_ABSEIL "Force usage of system provided abseil-cpp" OFF)
option(USE_SYSTEM_PROTOBUF "Force usage of system provided Protobuf" OFF)
option(USE_SYSTEM_PYBIND "Force usage of system provided pybind11" OFF)

# ============================================================================
# Find Python

find_package(Python COMPONENTS Interpreter Development)

# ============================================================================
# Build dependencies

if(USE_SYSTEM_ABSEIL)
  # Version omitted, as absl only allows EXACT version matches
  set(_absl_package_args REQUIRED)
else()
  set(_absl_package_args 20230125)
endif()
if(USE_SYSTEM_PROTOBUF)
  set(_protobuf_package_args 4.23.3 REQUIRED)
else()
  set(_protobuf_package_args 4.23.3)
endif()
if(USE_SYSTEM_PYBIND)
  set(_pybind11_package_args 2.11.1 REQUIRED)
else()
  set(_pybind11_package_args 2.11.1)
endif()

set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_ENABLE_INSTALL ON)

include(FetchContent)
FetchContent_Declare(
  absl
  GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
  GIT_TAG 20230125.3
  FIND_PACKAGE_ARGS ${_absl_package_args} NAMES absl)

FetchContent_Declare(
  Protobuf
  GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
  GIT_TAG v23.3
  GIT_SUBMODULES "" #
  FIND_PACKAGE_ARGS ${_protobuf_package_args} NAMES protobuf)
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "")

FetchContent_Declare(
  pybind11
  GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
  GIT_TAG v2.11.1
  FIND_PACKAGE_ARGS ${_pybind11_package_args} NAMES pybind11)

message(CHECK_START "Checking for external dependencies")
list(APPEND CMAKE_MESSAGE_INDENT "  ")
FetchContent_MakeAvailable(absl Protobuf pybind11)
list(POP_BACK CMAKE_MESSAGE_INDENT)

# ============================================================================
# pybind11_proto_utils pybind11 extension module
pybind11_add_module(
  pybind11_proto_utils MODULE pybind11_protobuf/proto_utils.cc
  pybind11_protobuf/proto_utils.h)

target_link_libraries(
  pybind11_proto_utils PRIVATE absl::strings protobuf::libprotobuf
                               ${Python_LIBRARIES})

target_include_directories(
  pybind11_proto_utils PRIVATE ${PROJECT_SOURCE_DIR} ${protobuf_INCLUDE_DIRS}
                               ${protobuf_SOURCE_DIR} ${pybind11_INCLUDE_DIRS})

# ============================================================================
# pybind11_native_proto_caster shared library
add_library(
  pybind11_native_proto_caster STATIC
  # bazel: pybind_library: native_proto_caster
  pybind11_protobuf/native_proto_caster.h
  # bazel: pybind_library: enum_type_caster
  pybind11_protobuf/enum_type_caster.h
  # bazel: pybind_library: proto_cast_util
  pybind11_protobuf/proto_cast_util.cc
  pybind11_protobuf/proto_cast_util.h
  pybind11_protobuf/proto_caster_impl.h)

target_sources(
  # bazel: cc_library::check_unknown_fields
  pybind11_native_proto_caster #
  PRIVATE pybind11_protobuf/check_unknown_fields.cc
          pybind11_protobuf/check_unknown_fields.h)

if(ENABLE_PYPROTO_API)
  target_compile_definitions(pybind11_native_proto_caster
                             PRIVATE PYBIND11_PROTOBUF_ENABLE_PYPROTO_API)
endif()

target_link_libraries(
  pybind11_native_proto_caster
  absl::flat_hash_map
  absl::flat_hash_set
  absl::hash
  absl::strings
  absl::optional
  protobuf::libprotobuf
  pybind11::pybind11
  ${Python_LIBRARIES})

target_include_directories(
  pybind11_native_proto_caster
  PRIVATE ${PROJECT_SOURCE_DIR} ${protobuf_INCLUDE_DIRS} ${protobuf_SOURCE_DIR}
          ${pybind11_INCLUDE_DIRS})

# ============================================================================
# pybind11_wrapped_proto_caster shared library
add_library(
  pybind11_wrapped_proto_caster STATIC
  # bazel: pybind_library: wrapped_proto_caster
  pybind11_protobuf/wrapped_proto_caster.h
  # bazel: pybind_library: proto_cast_util
  pybind11_protobuf/proto_cast_util.cc
  pybind11_protobuf/proto_cast_util.h
  pybind11_protobuf/proto_caster_impl.h)

target_sources(
  # bazel: cc_library::check_unknown_fields
  pybind11_wrapped_proto_caster
  PRIVATE pybind11_protobuf/check_unknown_fields.cc
          pybind11_protobuf/check_unknown_fields.h)

if(ENABLE_PYPROTO_API)
  target_compile_definitions(pybind11_wrapped_proto_caster
                             PRIVATE PYBIND11_PROTOBUF_ENABLE_PYPROTO_API)
endif()

target_link_libraries(
  pybind11_wrapped_proto_caster
  absl::flat_hash_map
  absl::flat_hash_set
  absl::hash
  absl::strings
  absl::optional
  protobuf::libprotobuf
  pybind11::pybind11
  ${Python_LIBRARIES})

target_include_directories(
  pybind11_wrapped_proto_caster
  PRIVATE ${PROJECT_SOURCE_DIR} ${protobuf_INCLUDE_DIRS} ${protobuf_SOURCE_DIR}
          ${pybind11_INCLUDE_DIRS})

# bazel equivs. checklist
#
# bazel: pybind_library: enum_type_caster - enum_type_caster.h
#
# bazel: pybind_library: native_proto_caster - native_proto_caster.h
#
# check_unknown_fields enum_type_caster proto_cast_util
#
# bazel: pybind_library: proto_cast_util - proto_cast_util.cc -
# proto_cast_util.h - proto_caster_impl.h
#
# check_unknown_fields
#
# bazel: pybind_library: wrapped_proto_caster - wrapped_proto_caster.h
#
# proto_cast_util
#

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

# ============================================================================
# Find Python

find_package(Python COMPONENTS Interpreter Development)

# ============================================================================
# Build dependencies

set(_absl_repository "https://github.com/abseil/abseil-cpp.git")
set(_absl_version 20220623.1)
set(_absl_tag 20220623.1)

set(_protobuf_repository "https://github.com/protocolbuffers/protobuf.git")
set(_protobuf_version 21.5)
set(_protobuf_tag v21.5)

set(_pybind11_repository "https://github.com/pybind/pybind11.git")
set(_pybind11_version 2.10)
set(_pybind11_tag v2.10.1)

add_subdirectory(cmake/dependencies dependencies)

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
  pybind11_native_proto_caster SHARED
  # bazel: pybind_library: native_proto_caster
  pybind11_protobuf/native_proto_caster.h
  # bazel: pybind_library: enum_type_caster
  pybind11_protobuf/enum_type_caster.h
  # bazel: pybind_library: proto_cast_util
  pybind11_protobuf/proto_cast_util.cc
  pybind11_protobuf/proto_cast_util.h
  pybind11_protobuf/proto_caster_impl.h
  # bazel: cc_library::check_unknown_fields
  pybind11_protobuf/check_unknown_fields.cc
  pybind11_protobuf/check_unknown_fields.h)

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
  pybind11_wrapped_proto_caster SHARED
  # bazel: pybind_library: wrapped_proto_caster
  pybind11_protobuf/wrapped_proto_caster.h
  # bazel: pybind_library: proto_cast_util
  pybind11_protobuf/proto_cast_util.cc
  pybind11_protobuf/proto_cast_util.h
  pybind11_protobuf/proto_caster_impl.h
  # bazel: cc_library: check_unknown_fields
  pybind11_protobuf/check_unknown_fields.cc
  pybind11_protobuf/check_unknown_fields.h)

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

# TODO set defines PYBIND11_PROTOBUF_ENABLE_PYPROTO_API see: bazel:
# pybind_library: proto_cast_util

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

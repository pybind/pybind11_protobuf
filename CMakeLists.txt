# We can lower the minimum required version from 3.18 to 3.16.
cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(pybind11_protobuf)

if(MSVC)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This is the same as -fPIC, for static libraries it is OFF by default.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ============================================================================
# Options

option(BUILD_TESTS "Build tests." OFF)

# ============================================================================
# Find Python

find_package(Python COMPONENTS Interpreter Development)

# ============================================================================
# Build dependencies

# We use the latest version of abseil-cpp. It is not available as debian package.
set(_absl_repository "https://github.com/abseil/abseil-cpp.git")
set(_absl_version 20230802)
set(_absl_tag 20230802.1)

# We use the protobuf version that is installed via debian package available with the chosen ubuntu version.
find_package(Protobuf ${_protobuf_version} QUIET)

# We use the latest version of pybind11. It is not available as debian package.
set(_pybind11_repository "https://github.com/pybind/pybind11.git")
set(_pybind11_version 2.11.1)
set(_pybind11_tag v2.11.1)

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
# pybind11_native_proto_caster static library
# We need to make it static to avoid an "undefined symbol" error when running the python code.
# There should be a solution which allows us to use a shared library, but it is not clear what is missing.
add_library(
  pybind11_native_proto_caster
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
  # We need to include this API header because it is not included in the debian package.
  # https://github.com/protocolbuffers/protobuf/issues/9464
  PUBLIC extern/protobuf/include
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

# ============================================================================
# pybind11_proto_utils pybind11 extension module
pybind11_add_module(pybind11_proto_utils MODULE proto_utils.cc proto_utils.h)
pybind11_protobuf_build_tree_module(pybind11_proto_utils)
add_library(pybind11_protobuf::pybind11_proto_utils ALIAS pybind11_proto_utils)

# note: macOS is APPLE and also UNIX !
if(APPLE)
  set_target_properties(pybind11_proto_utils PROPERTIES SUFFIX ".so")
endif()

target_include_directories(pybind11_proto_utils
                           INTERFACE $<BUILD_INTERFACE:${TOP_LEVEL_DIR}>)

target_link_libraries(pybind11_proto_utils PUBLIC absl::strings
                                                  protobuf::libprotobuf)

# ============================================================================
# pybind11_native_proto_caster static library
add_library(
  pybind11_native_proto_caster STATIC
  # bazel: pybind_library: native_proto_caster
  native_proto_caster.h
  # bazel: pybind_library: enum_type_caster
  enum_type_caster.h
  # bazel: pybind_library: proto_cast_util
  proto_cast_util.cc
  proto_cast_util.h
  proto_caster_impl.h)
add_library(pybind11_protobuf::pybind11_native_proto_caster ALIAS
            pybind11_native_proto_caster)

target_link_libraries(
  pybind11_native_proto_caster
  PUBLIC absl::flat_hash_map
         absl::flat_hash_set
         absl::hash
         absl::strings
         absl::optional
         protobuf::libprotobuf
         pybind11::pybind11)

target_include_directories(pybind11_native_proto_caster
                           PUBLIC $<BUILD_INTERFACE:${TOP_LEVEL_DIR}>)

# ============================================================================
# pybind11_wrapped_proto_caster static library
add_library(
  pybind11_wrapped_proto_caster STATIC
  # bazel: pybind_library: wrapped_proto_caster
  wrapped_proto_caster.h
  # bazel: pybind_library: proto_cast_util
  proto_cast_util.cc proto_cast_util.h proto_caster_impl.h)
add_library(pybind11_protobuf::pybind11_wrapped_proto_caster ALIAS
            pybind11_wrapped_proto_caster)

target_link_libraries(
  pybind11_wrapped_proto_caster
  PUBLIC absl::flat_hash_map
         absl::flat_hash_set
         absl::hash
         absl::strings
         absl::optional
         protobuf::libprotobuf
         pybind11::pybind11)

target_include_directories(pybind11_wrapped_proto_caster
                           PUBLIC $<BUILD_INTERFACE:${TOP_LEVEL_DIR}>)

if(PYBIND11_PROTOBUF_BUILD_TESTING)
  add_subdirectory(tests)
endif()

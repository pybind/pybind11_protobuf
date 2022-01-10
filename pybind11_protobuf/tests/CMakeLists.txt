#============================================================================
# tests
#============================================================================

#============================================================================
# generate protobuf bindings

set(Protobuf_IMPORT_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../..;${Protobuf_IMPORT_DIRS}"
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
  # extension.proto
  test.proto
)

protobuf_generate_python(PROTO_PYS
  # extension.proto
  test.proto
)

#============================================================================
# adding a custom target forces CMake to generate the bindings

add_custom_target(pybind11_test_proto_cpp ALL DEPENDS ${PROTO_SRCS} ${PROTO_HDRS})
add_custom_target(pybind11_test_proto_python ALL DEPENDS ${PROTO_PYS})

#============================================================================
# generate protobuf library

add_library(pybind11_test_proto ${PROTO_SRCS} ${PROTO_HDR})

target_include_directories(pybind11_test_proto
  PUBLIC
    ${Protobuf_INCLUDE_DIR}
)

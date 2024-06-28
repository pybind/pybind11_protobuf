#
# Evaluate if Protobuf uses the system package, otherwise explicitly include the
# required macro
#
FetchContent_GetProperties(Protobuf SOURCE_DIR Protobuf_SOURCE_DIR)
if(Protobuf_SOURCE_DIR)
  # Use macros from content made available by FetchContent
  include(${Protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
endif()

# cmake-format: off
function(generate_cc_proto protoname)
  # Generate C++ files (.pb.h, .pb.cc)
  #
  add_library(${protoname}_cc_proto OBJECT)
  target_include_directories(${protoname}_cc_proto
    PRIVATE $<TARGET_PROPERTY:protobuf::libprotobuf,INCLUDE_DIRECTORIES>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)
  protobuf_generate(
    TARGET ${protoname}_cc_proto
    PROTOS ${CMAKE_SOURCE_DIR}/pybind11_protobuf/tests/${protoname}.proto
    IMPORT_DIRS ${CMAKE_SOURCE_DIR}
    PROTOC_OUT_DIR ${CMAKE_BINARY_DIR})
endfunction()

function(generate_py_proto protoname)
  # Generate Python files (_pb2.py)
  #
  add_custom_target(${protoname}_py_pb2 ALL)
  protobuf_generate(
    TARGET ${protoname}_py_pb2
    LANGUAGE PYTHON
    PROTOS ${CMAKE_SOURCE_DIR}/pybind11_protobuf/tests/${protoname}.proto
    IMPORT_DIRS ${CMAKE_SOURCE_DIR}
    PROTOC_OUT_DIR ${CMAKE_BINARY_DIR})
endfunction()
# cmake-format: on

generate_cc_proto("test")
generate_cc_proto("extension")
generate_cc_proto("extension_nest_repeated")
generate_cc_proto("extension_in_other_file_in_deps")
generate_cc_proto("extension_in_other_file")
generate_cc_proto("we-love-dashes")

generate_py_proto("test")
generate_py_proto("extension")
generate_py_proto("extension_nest_repeated")
generate_py_proto("extension_in_other_file_in_deps")
generate_py_proto("extension_in_other_file")

function(generate_extension modulename deps)
  pybind11_add_module(${modulename}_module ${modulename}_module.cc)
  add_dependencies(${modulename}_module ${deps})
  target_include_directories(${modulename}_module #
                             PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
  target_link_libraries(${modulename}_module #
                        PRIVATE protobuf::libprotobuf ${deps})
endfunction()

generate_extension(proto_enum "test_cc_proto")
generate_extension(dynamic_message "pybind11_native_proto_caster")
generate_extension(
  extension #
  "extension_in_other_file_in_deps_cc_proto;extension_nest_repeated_cc_proto;test_cc_proto;extension_cc_proto;pybind11_native_proto_caster"
)
generate_extension(message "test_cc_proto;pybind11_native_proto_caster")
generate_extension(pass_by "test_cc_proto;pybind11_native_proto_caster")
generate_extension(pass_proto2_message "pybind11_native_proto_caster")
generate_extension(wrapped_proto "test_cc_proto;pybind11_wrapped_proto_caster")
generate_extension(thread "test_cc_proto;pybind11_native_proto_caster")
generate_extension(regression_wrappers "pybind11_native_proto_caster")
generate_extension(we_love_dashes_cc_only #
                   "we-love-dashes_cc_proto;pybind11_native_proto_caster")

function(add_py_test testname)
  add_test(NAME ${testname}_test
           COMMAND ${Python_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/${testname}_test.py)
  set_property(TEST ${testname}_test #
               PROPERTY ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}")
endfunction()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/compare.py
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_py_test(proto_enum)
add_py_test(dynamic_message)
add_py_test(extension)
add_py_test(message)
add_py_test(pass_by)
add_py_test(wrapped_proto_module)
add_py_test(thread_module)
add_py_test(regression_wrappers)
add_py_test(we_love_dashes_cc_only)

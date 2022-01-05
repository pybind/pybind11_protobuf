#ifndef PYBIND11_PROTOBUF_PROTO_CAST_UTIL_H_
#define PYBIND11_PROTOBUF_PROTO_CAST_UTIL_H_

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "absl/types/optional.h"

namespace pybind11_protobuf {

// Initialize internal proto cast dependencies, which includes importing
// various protobuf-related modules.
void InitializePybindProtoCastUtil();

// Imports a module pertaining to a given ::google::protobuf::Descriptor, if possible.
void ImportProtoDescriptorModule(const ::google::protobuf::Descriptor *);

// Returns a ::google::protobuf::Message* from a cpp_fast_proto, if backed by C++.
const ::google::protobuf::Message *PyProtoGetCppMessagePointer(pybind11::handle src);

// Pins a proto.DESCRIPTOR.pool to ensure that it remains alive.
bool PyProtoPinDescriptorPool(pybind11::handle src);

// Returns the protocol buffer's py_proto.DESCRIPTOR.full_name attribute.
absl::optional<std::string> PyProtoDescriptorName(pybind11::handle py_proto);

// Allocates a C++ protocol buffer for a given name.
std::unique_ptr<::google::protobuf::Message> AllocateCProtoFromPythonSymbolDatabase(
    pybind11::handle src, const std::string &full_name);

// Serialize the py_proto and deserialize it into the provided message.
// Caller should enforce any type identity that is required.
bool PyProtoCopyToCProto(pybind11::handle py_proto, ::google::protobuf::Message *message);

// Returns whether two ::google::protobuf::Descriptor* are compatible.
bool PyCompatibleDescriptor(const ::google::protobuf::Descriptor *a,
                            const ::google::protobuf::Descriptor *b);

// Returns a handle to a python protobuf suitably
pybind11::handle GenericFastCppProtoCast(::google::protobuf::Message *src,
                                         pybind11::return_value_policy policy,
                                         pybind11::handle parent,
                                         bool is_const);

pybind11::handle GenericPyProtoCast(::google::protobuf::Message *src,
                                    pybind11::return_value_policy policy,
                                    pybind11::handle parent, bool is_const);

pybind11::handle GenericProtoCast(::google::protobuf::Message *src,
                                  pybind11::return_value_policy policy,
                                  pybind11::handle parent, bool is_const);

}  // namespace pybind11_protobuf

#endif  // PYBIND11_PROTOBUF_PROTO_CAST_UTIL_H_

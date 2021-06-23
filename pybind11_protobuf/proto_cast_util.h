#ifndef PYBIND11_PROTOBUF_PROTO_CAST_UTIL_H_
#define PYBIND11_PROTOBUF_PROTO_CAST_UTIL_H_

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace pybind11_protobuf {

// Returns a ::google::protobuf::Message* from a cpp_fast_proto.
const ::google::protobuf::Message *PyProtoGetCppMessagePointer(pybind11::handle src);

// Returns the protocol buffer's py_proto.DESCRIPTOR.full_name attribute.
std::optional<std::string> PyProtoDescriptorName(pybind11::handle py_proto);

// Allocates a C++ protocol buffer for a given name.
std::unique_ptr<::google::protobuf::Message> AllocateCProtoByName(
    const std::string &full_name);

// Serialize the py_proto and deserialize it into the provided message.
// Caller should enforce any type identity that is required.
bool PyProtoCopyToCProto(pybind11::handle py_proto, ::google::protobuf::Message *message);

// Allocates a fast cpp proto python object, also returning
// the embedded c++ proto2 message type. The returned message
// pointer cannot be null.
std::pair<pybind11::object, ::google::protobuf::Message *> AllocatePyFastCppProto(
    const ::google::protobuf::Descriptor *descriptor);

// Returns a python object that references an existing c++ proto
// message. This is potentially unsafe.
pybind11::object ReferencePyFastCppProto(::google::protobuf::Message *message);

}  // namespace pybind11_protobuf

#endif  // PYBIND11_PROTOBUF_PROTO_CAST_UTIL_H_

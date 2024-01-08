// IWYU pragma: private, include "pybind11_protobuf/native_proto_caster.h"

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
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

// PYBIND11_PROTOBUF_ASSUME_FULL_ABI_COMPATIBILITY can be defined by users
// certain about ABI compatibility between all Python extensions in their
// environment using protobufs. If defined, passing protos from Python to C++
// may skip serialization/deserialization.
// Assuming full ABI compatibility means (roughly) that the following are
// compatible between all involved Python extensions:
// * Protobuf library versions.
// * Compiler/linker & compiler/linker options.
// #define PYBIND11_PROTOBUF_ASSUME_FULL_ABI_COMPATIBILITY

namespace pybind11_protobuf {

// Simple helper. Caller has to ensure that the py_bytes argument outlives the
// returned string_view.
absl::string_view PyBytesAsStringView(pybind11::bytes py_bytes);

// Initialize internal proto cast dependencies, which includes importing
// various protobuf-related modules.
void InitializePybindProtoCastUtil();

// Imports a module pertaining to a given ::google::protobuf::Descriptor, if possible.
void ImportProtoDescriptorModule(const ::google::protobuf::Descriptor *);

// Returns a ::google::protobuf::Message* from a cpp_fast_proto, if backed by C++.
const ::google::protobuf::Message *PyProtoGetCppMessagePointer(pybind11::handle src);

// Returns the protocol buffer's py_proto.DESCRIPTOR.full_name attribute.
absl::optional<std::string> PyProtoDescriptorFullName(
    pybind11::handle py_proto);

// Returns true if py_proto full name matches descriptor full name.
bool PyProtoHasMatchingFullName(pybind11::handle py_proto,
                                const ::google::protobuf::Descriptor *descriptor);

// Caller should enforce any type identity that is required.
pybind11::bytes PyProtoSerializePartialToString(pybind11::handle py_proto,
                                                bool raise_if_error);

// Allocates a C++ protocol buffer for a given name.
std::unique_ptr<::google::protobuf::Message> AllocateCProtoFromPythonSymbolDatabase(
    pybind11::handle src, const std::string &full_name);

// Serialize the py_proto and deserialize it into the provided message.
// Caller should enforce any type identity that is required.
void CProtoCopyToPyProto(::google::protobuf::Message *message, pybind11::handle py_proto);

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

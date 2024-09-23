// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef PYBIND11_PROTOBUF_PROTO_UTILS_H_
#define PYBIND11_PROTOBUF_PROTO_UTILS_H_

#include <pybind11/pybind11.h>

#include <memory>

#include "net/proto2/public/message.h"

namespace pybind11 {
namespace google {

// Initializes the fields in the given message from the the keyword args.
// Unlike ProtoSetField, this allows setting message, map and repeated fields.
void ProtoInitFields(::google::protobuf::Message* message, kwargs kwargs_in);

// Wrapper around ::google::protobuf::Message::CopyFrom which can efficiently copy from
// either a wrapped C++ or native python proto. Throws an error if `other`
// is not a proto of the correct type.
void ProtoCopyFrom(::google::protobuf::Message* msg, handle other);

// Allocate and return the ProtoType given by the template argument.
// py_proto is not used in this version, but is used by a specialization below.
template <typename ProtoType>
std::unique_ptr<ProtoType> PyProtoAllocateMessage(handle py_proto = handle(),
                                                  kwargs kwargs_in = kwargs()) {
  auto message = std::make_unique<ProtoType>();
  ProtoInitFields(message.get(), kwargs_in);
  return message;
}

// Specialization for the case that the template argument is a generic message.
// The type is pulled from the py_proto, which can be a native python proto,
// a wrapped C proto, or a string with the full type name.
template <>
std::unique_ptr<::google::protobuf::Message> PyProtoAllocateMessage(handle py_proto,
                                                        kwargs kwargs_in);

// Allocate a C++ proto of the same type as py_proto and copy the contents
// from py_proto. This works whether py_proto is a native or wrapped proto.
// If expected_type is given and the type in py_proto does not match it, an
// invalid argument error will be thrown.
template <typename ProtoType>
std::unique_ptr<ProtoType> PyProtoAllocateAndCopyMessage(handle py_proto) {
  auto new_msg = PyProtoAllocateMessage<ProtoType>(py_proto);
  ProtoCopyFrom(new_msg.get(), py_proto);
  return new_msg;
}

}  // namespace google
}  // namespace pybind11

#endif  // PYBIND11_PROTOBUF_PROTO_UTILS_H_

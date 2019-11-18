// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"
#include "pybind11_protobuf/proto_utils.h"

namespace pybind11 {
namespace google {

PYBIND11_MODULE(proto, m) {
  // Return whether the given python object is a wrapped C proto.
  m.def("is_wrapped_c_proto", &IsWrappedCProto, arg("src"));

  // Add bindings for the descriptor class.
  class_<proto2::Descriptor>(m, "Descriptor")
      .def_property_readonly("full_name", &proto2::Descriptor::full_name)
      .def_property_readonly("name", &proto2::Descriptor::name);

  // Add bindings for the base message class.
  class_<proto2::Message>(m, "Message")
      .def_property_readonly("DESCRIPTOR", &proto2::Message::GetDescriptor,
                             return_value_policy::reference)
      .def("__repr__", &proto2::Message::DebugString)
      .def("SerializeToString", &proto2::Message::SerializeAsString)
      .def("ParseFromString", &proto2::Message::ParseFromString, arg("data"))
      .def("MergeFromString", &proto2::Message::MergeFromString, arg("data"))
      .def("ByteSize", &proto2::Message::ByteSizeLong)
      .def("Clear", &proto2::Message::Clear)
      .def("CopyFrom", &proto2::Message::CopyFrom)
      .def("MergeFrom", &proto2::Message::MergeFrom)
      .def_property_readonly(kIsWrappedCProtoAttr, [](void*) { return true; });

  // Add bindings for the repeated field containers.
  BindEachFieldType<RepeatedFieldBindings>(m, "Repeated");

  // Add bindings for the map field containers.
  BindEachFieldType<MapFieldBindings>(m, "Mapped");
}

}  // namespace google
}  // namespace pybind11

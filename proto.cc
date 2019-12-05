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
// The bindings defined bellow should match the native python proto API:
// https://developers.google.com/protocol-buffers/docs/reference/python-generated
// However, there are still some differences. If you discover a difference that
// impacts your use case, please file a bug:
// https://b.corp.google.com/issues/new?component=779382&template=1371463

// Class to add python bindings to a RepeatedFieldContainer.
// This corresponds to the repeated field API:
// https://developers.google.com/protocol-buffers/docs/reference/python-generated#repeated-fields
// https://developers.google.com/protocol-buffers/docs/reference/python-generated#repeated-message-fields
template <typename T>
class RepeatedFieldBindings : public class_<RepeatedFieldContainer<T>> {
 public:
  RepeatedFieldBindings(handle scope, const std::string& name)
      : class_<RepeatedFieldContainer<T>>(scope, name.c_str()) {
    // Repeated message fields don't support item assignment.
    if (!std::is_same_v<T, proto2::Message>)
      this->def("__setitem__", &RepeatedFieldContainer<T>::SetItem);
    this->def("__repr__", &RepeatedFieldContainer<T>::Repr);
    this->def("__len__", &RepeatedFieldContainer<T>::Size);
    this->def("__getitem__", &RepeatedFieldContainer<T>::GetItem);
    this->def("__delitem__", &RepeatedFieldContainer<T>::DelItem);
    this->def("MergeFrom", &RepeatedFieldContainer<T>::Extend);
    this->def("extend", &RepeatedFieldContainer<T>::Extend);
    this->def("append", &RepeatedFieldContainer<T>::Add);
    this->def("insert", &RepeatedFieldContainer<T>::Insert);
    // TODO(b/145687883): Remove the clear method, which doesn't exist in the
    // native API. __delitem__ should be used instead, once it supports slicing.
    this->def("clear", &RepeatedFieldContainer<T>::Clear);
  }
};

// Class to add python bindings to a MapFieldContainer.
// This corresponds to the map field API:
// https://developers.google.com/protocol-buffers/docs/reference/python-generated#map-fields
template <typename T>
class MapFieldBindings : public class_<MapFieldContainer<T>> {
 public:
  MapFieldBindings(handle scope, const std::string& name)
      : class_<MapFieldContainer<T>>(scope, name.c_str()) {
    // Mapped message fields don't support item assignment.
    if (!std::is_same_v<T, proto2::Message>)
      this->def("__setitem__", &MapFieldContainer<T>::Set);
    this->def("__repr__", &MapFieldContainer<T>::Repr);
    this->def("__len__", &MapFieldContainer<T>::Size);
    this->def("__contains__", &MapFieldContainer<T>::Contains);
    this->def("__getitem__", &MapFieldContainer<T>::GetPython);
    this->def("clear", &RepeatedFieldContainer<T>::Clear);
  }
};

// Function to instantiate the given Bindings class with each of the types
// supported by protobuffers.
template <template <typename> class Bindings>
void BindEachFieldType(module& module, const std::string& name) {
  Bindings<int32>(module, name + "Int32");
  Bindings<int64>(module, name + "Int64");
  Bindings<uint32>(module, name + "UInt32");
  Bindings<uint64>(module, name + "UInt64");
  Bindings<float>(module, name + "Float");
  Bindings<double>(module, name + "Double");
  Bindings<bool>(module, name + "Bool");
  Bindings<std::string>(module, name + "String");
  Bindings<proto2::Message>(module, name + "Message");
  Bindings<GenericEnum>(module, name + "Enum");
}

PYBIND11_MODULE(proto, m) {
  // Return whether the given python object is a wrapped C proto.
  m.def("is_wrapped_c_proto", &IsWrappedCProto, arg("src"));
  m.def("make_wrapped_c_proto", &PyProtoAllocateMessage<proto2::Message>,
        arg("type"),
        "Returns a wrapped C proto of the given type. The type may be passed "
        "as a string ('package_name.MessageName'), an instance of a native "
        "python proto, or an instance of a wrapped C proto. The C++ version "
        "of your proto library must be linked in for this to work.");

  // Add bindings for the descriptor class.
  class_<proto2::Descriptor>(m, "Descriptor")
      .def_property_readonly("full_name", &proto2::Descriptor::full_name)
      .def_property_readonly("name", &proto2::Descriptor::name);

  // Add bindings for the base message class.
  // These bindings use __getattr__, __setattr__ and the reflection interface
  // to access message-specific fields, so no additional bindings are needed
  // for derived message types.
  class_<proto2::Message>(m, "ProtoMessage")
      .def_property_readonly("DESCRIPTOR", &proto2::Message::GetDescriptor,
                             return_value_policy::reference)
      .def("__repr__", &proto2::Message::DebugString)
      .def("__getattr__", &ProtoGetAttr)
      .def("__setattr__", &ProtoSetAttr)
      .def("SerializeToString",
           [](proto2::Message* msg) { return bytes(msg->SerializeAsString()); })
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

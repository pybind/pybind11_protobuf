// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "net/proto2/proto/descriptor.proto.h"
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
//
// In general, any code that defines bindings (ie, uses pybind11::class_)
// should go in this file, while all other code should go in proto_utils.cc/h.

// Class to add python bindings to a RepeatedFieldContainer.
// This corresponds to the repeated field API:
// https://developers.google.com/protocol-buffers/docs/reference/python-generated#repeated-fields
// https://developers.google.com/protocol-buffers/docs/reference/python-generated#repeated-message-fields
template <typename T>
class RepeatedFieldBindings : public class_<RepeatedFieldContainer<T>> {
 public:
  RepeatedFieldBindings(handle scope, const std::string& name)
      : class_<RepeatedFieldContainer<T>>(scope, name.c_str()) {
    // Repeated message fields support `add` but not `__setitem__`.
    if (std::is_same_v<T, proto2::Message>) {
      this->def("add", &AddMessage<T>, return_value_policy::reference_internal);
    } else {
      this->def("__setitem__", &RepeatedFieldContainer<T>::SetItem);
    }
    this->def("__repr__", &RepeatedFieldContainer<T>::Repr);
    this->def("__len__", &RepeatedFieldContainer<T>::Size);
    this->def("__getitem__", &RepeatedFieldContainer<T>::GetItem);
    this->def("__delitem__", &RepeatedFieldContainer<T>::DelItem);
    this->def("MergeFrom", &RepeatedFieldContainer<T>::Extend);
    this->def("extend", &RepeatedFieldContainer<T>::Extend);
    this->def("append", &RepeatedFieldContainer<T>::Append);
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
    if (std::is_same_v<T, proto2::Message>) {
      this->def("__setitem__", [](void*, int, handle) {
        throw value_error("Cannot assign to message in a map field.");
      });
    } else {
      this->def("__setitem__", &MapFieldContainer<T>::SetPython);
    }
    this->def("__repr__", &MapFieldContainer<T>::Repr);
    this->def("__len__", &MapFieldContainer<T>::Size);
    this->def("__contains__", &MapFieldContainer<T>::Contains);
    this->def("__getitem__", &MapFieldContainer<T>::GetPython);
    this->def("update", &MapFieldContainer<T>::UpdateFromDict);
    this->def("update", &MapFieldContainer<T>::UpdateFromKWArgs);
    this->def("clear", &MapFieldContainer<T>::Clear);
    this->def("GetEntryClass", &MapFieldContainer<T>::GetEntryClassFactory,
              "Returns a factory function which can be called with keyword "
              "Arguments to create an instance of the map entry class (ie, "
              "a message with `key` and `value` fields). Used by text_format.");
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

// Define a property in the given class_ with the given name which is constant
// for the life of an object instance. The generator function will be invoked
// the first time the property is accessed. The result of this function will
// be cached and returned for all future accesses, without invoking the
// generator function again. dynamic_attr() must have been passed to the class_
// constructor or you will get "AttributeError: can't set attribute".
// The given policy is applied to the return value of the generator function.
template <typename Class, typename Return, typename... Args>
void DefConstantProperty(
    class_<detail::intrinsic_t<Class>>* pyclass, const std::string& name,
    std::function<Return(Class, Args...)> generator,
    return_value_policy policy = return_value_policy::automatic_reference) {
  auto wrapper = [generator, name, policy](handle pyinst, Args... args) {
    std::string cache_name = "_cache_" + name;
    if (!hasattr(pyinst, cache_name.c_str())) {
      // Invoke the generator and cache the result.
      Return result =
          generator(cast<Class>(pyinst), std::forward<Args>(args)...);
      setattr(
          pyinst, cache_name.c_str(),
          detail::make_caster<object>::cast(std::move(result), policy, pyinst));
    }
    return getattr(pyinst, cache_name.c_str());
  };
  pyclass->def_property_readonly(name.c_str(), wrapper);
}

// Same as above, but for a function pointer rather than a std::function.
template <typename Class, typename Return, typename... Args>
void DefConstantProperty(
    class_<detail::intrinsic_t<Class>>* pyclass, const std::string& name,
    Return (*generator)(Class, Args...),
    return_value_policy policy = return_value_policy::automatic_reference) {
  DefConstantProperty(
      pyclass, name, std::function<Return(Class, Args...)>(generator), policy);
}

PYBIND11_MODULE(proto, m) {
  // Return whether the given python object is a wrapped C proto.
  m.def("is_wrapped_c_proto", &IsWrappedCProto, arg("src"));

  // Construct and optionally initialize a wrapped C proto.
  m.def("make_wrapped_c_proto", &PyProtoAllocateMessage<proto2::Message>,
        arg("type"),
        "Returns a wrapped C proto of the given type. The type may be passed "
        "as a string ('package_name.MessageName'), an instance of a native "
        "python proto, or an instance of a wrapped C proto. Fields may be "
        "initialized with keyword arguments, as with the native constructors. "
        "The C++ version of the proto library for your message type must be "
        "linked in for this to work.");

  // Add bindings for the descriptor class.
  class_<proto2::Descriptor>(m, "Descriptor")
      .def_property_readonly("full_name", &proto2::Descriptor::full_name)
      .def_property_readonly("name", &proto2::Descriptor::name)
      .def_property_readonly("has_options",
                             [](proto2::Descriptor*) { return true; })
      .def(
          "GetOptions",
          [](proto2::Descriptor* descriptor) -> const proto2::Message* {
            return &descriptor->options();
          },
          return_value_policy::reference);

  // Add bindings for the enum descriptor class.
  class_<proto2::EnumDescriptor> enum_desc_t(m, "EnumDescriptor",
                                             dynamic_attr());
  enum_desc_t.def_property_readonly("name", &proto2::EnumDescriptor::name);
  DefConstantProperty(&enum_desc_t, "values_by_number", &EnumValuesByNumber);

  // Add bindings for the enum value descriptor class.
  class_<proto2::EnumValueDescriptor>(m, "EnumDescriptorValue")
      .def_property_readonly("name", &proto2::EnumValueDescriptor::name);

  // Add bindings for the field descriptor class.
  class_<proto2::FieldDescriptor> field_descriptor_class(m, "FieldDescriptor");
  field_descriptor_class
      .def_property_readonly("name", &proto2::FieldDescriptor::name)
      .def_property_readonly("type", &proto2::FieldDescriptor::type)
      .def_property_readonly("cpp_type", &proto2::FieldDescriptor::cpp_type)
      .def_property_readonly("containing_type",
                             &proto2::FieldDescriptor::containing_type,
                             return_value_policy::reference)
      .def_property_readonly("message_type",
                             &proto2::FieldDescriptor::message_type,
                             return_value_policy::reference)
      .def_property_readonly("enum_type", &proto2::FieldDescriptor::enum_type,
                             return_value_policy::reference)
      .def_property_readonly("is_extension",
                             &proto2::FieldDescriptor::is_extension)
      .def_property_readonly("label", &proto2::FieldDescriptor::label);

  // Add Type enum values.
  enum_<proto2::FieldDescriptor::Type>(field_descriptor_class, "Type")
      .value("TYPE_DOUBLE", proto2::FieldDescriptor::TYPE_DOUBLE)
      .value("TYPE_FLOAT", proto2::FieldDescriptor::TYPE_FLOAT)
      .value("TYPE_INT64", proto2::FieldDescriptor::TYPE_INT64)
      .value("TYPE_UINT64", proto2::FieldDescriptor::TYPE_UINT64)
      .value("TYPE_INT32", proto2::FieldDescriptor::TYPE_INT32)
      .value("TYPE_FIXED64", proto2::FieldDescriptor::TYPE_FIXED64)
      .value("TYPE_FIXED32", proto2::FieldDescriptor::TYPE_FIXED32)
      .value("TYPE_BOOL", proto2::FieldDescriptor::TYPE_BOOL)
      .value("TYPE_STRING", proto2::FieldDescriptor::TYPE_STRING)
      .value("TYPE_GROUP", proto2::FieldDescriptor::TYPE_GROUP)
      .value("TYPE_MESSAGE", proto2::FieldDescriptor::TYPE_MESSAGE)
      .value("TYPE_BYTES", proto2::FieldDescriptor::TYPE_BYTES)
      .value("TYPE_UINT32", proto2::FieldDescriptor::TYPE_UINT32)
      .value("TYPE_ENUM", proto2::FieldDescriptor::TYPE_ENUM)
      .value("TYPE_SFIXED32", proto2::FieldDescriptor::TYPE_SFIXED32)
      .value("TYPE_SFIXED64", proto2::FieldDescriptor::TYPE_SFIXED64)
      .value("TYPE_SINT32", proto2::FieldDescriptor::TYPE_SINT32)
      .value("TYPE_SINT64", proto2::FieldDescriptor::TYPE_SINT64)
      .export_values();

  // Add C++ Type enum values.
  enum_<proto2::FieldDescriptor::CppType>(field_descriptor_class, "CppType")
      .value("CPPTYPE_INT32", proto2::FieldDescriptor::CPPTYPE_INT32)
      .value("CPPTYPE_INT64", proto2::FieldDescriptor::CPPTYPE_INT64)
      .value("CPPTYPE_UINT32", proto2::FieldDescriptor::CPPTYPE_UINT32)
      .value("CPPTYPE_UINT64", proto2::FieldDescriptor::CPPTYPE_UINT64)
      .value("CPPTYPE_DOUBLE", proto2::FieldDescriptor::CPPTYPE_DOUBLE)
      .value("CPPTYPE_FLOAT", proto2::FieldDescriptor::CPPTYPE_FLOAT)
      .value("CPPTYPE_BOOL", proto2::FieldDescriptor::CPPTYPE_BOOL)
      .value("CPPTYPE_ENUM", proto2::FieldDescriptor::CPPTYPE_ENUM)
      .value("CPPTYPE_STRING", proto2::FieldDescriptor::CPPTYPE_STRING)
      .value("CPPTYPE_MESSAGE", proto2::FieldDescriptor::CPPTYPE_MESSAGE)
      .export_values();

  // Add Label enum values.
  enum_<proto2::FieldDescriptor::Label>(field_descriptor_class, "Label")
      .value("LABEL_OPTIONAL", proto2::FieldDescriptor::LABEL_OPTIONAL)
      .value("LABEL_REQUIRED", proto2::FieldDescriptor::LABEL_REQUIRED)
      .value("LABEL_REPEATED", proto2::FieldDescriptor::LABEL_REPEATED)
      .export_values();

  // Add bindings for the base message class.
  // These bindings use __getattr__, __setattr__ and the reflection interface
  // to access message-specific fields, so no additional bindings are needed
  // for derived message types.
  class_<proto2::Message>(m, "ProtoMessage")
      .def_property_readonly("DESCRIPTOR", &proto2::Message::GetDescriptor,
                             return_value_policy::reference)
      .def_property_readonly(kIsWrappedCProtoAttr, [](void*) { return true; })
      .def("__repr__", &proto2::Message::DebugString)
      .def("__getattr__", &ProtoGetField)
      .def("__setattr__", &ProtoSetField)
      .def("SerializeToString",
           [](proto2::Message* msg) { return bytes(msg->SerializeAsString()); })
      .def("ParseFromString", &proto2::Message::ParseFromString, arg("data"))
      .def("MergeFromString", &proto2::Message::MergeFromString, arg("data"))
      .def("ByteSize", &proto2::Message::ByteSizeLong)
      .def("Clear", &proto2::Message::Clear)
      .def("CopyFrom", &proto2::Message::CopyFrom)
      .def("MergeFrom", &proto2::Message::MergeFrom)
      .def("FindInitializationErrors", &FindInitializationErrors,
           "Slowly build a list of all required fields that are not set.")
      .def("ListFields", &ListFields)
      // Pickle support is provided only because copy.deepcopy uses it.
      // Do not use it directly; use serialize/parse instead (go/nopickle).
      .def(pickle(
          [](proto2::Message* message) {
            return dict("serialized"_a = bytes(message->SerializeAsString()),
                        "type_name"_a = message->GetTypeName());
          },
          [](dict d) {
            auto message =
                PyProtoAllocateMessage<proto2::Message>(d["type_name"]);
            // TODO(b/145925674): Use ParseFromStringPiece once str
            // supports string_view casting.
            message->ParseFromString(str(d["serialized"]));
            return message;
          }));

  // Add bindings for the repeated field containers.
  BindEachFieldType<RepeatedFieldBindings>(m, "Repeated");

  // Add bindings for the map field containers.
  BindEachFieldType<MapFieldBindings>(m, "Mapped");
}

}  // namespace google
}  // namespace pybind11

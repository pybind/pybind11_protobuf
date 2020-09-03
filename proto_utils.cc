// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "pybind11_protobuf/proto_utils.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>

#include "google/protobuf/any.proto.h"
#include "net/proto2/proto/descriptor.proto.h"
#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"

namespace pybind11 {
namespace google {

bool PyProtoFullName(handle py_proto, std::string* name) {
  if (hasattr(py_proto, "DESCRIPTOR")) {
    auto descriptor = py_proto.attr("DESCRIPTOR");
    if (hasattr(descriptor, "full_name")) {
      if (name) *name = cast<std::string>(descriptor.attr("full_name"));
      return true;
    }
  }
  return false;
}

bool PyProtoCheckType(handle py_proto, const std::string& expected_type) {
  if (std::string name; PyProtoFullName(py_proto, &name))
    return name == expected_type;
  return false;
}

void PyProtoCheckTypeOrThrow(handle py_proto,
                             const std::string& expected_type) {
  std::string name;
  if (!PyProtoFullName(py_proto, &name)) {
    auto builtins = module::import("builtins");
    std::string type_str =
        str(builtins.attr("repr")(builtins.attr("type")(py_proto)));
    throw type_error("Expected a proto, got a " + type_str + ".");
  } else if (name != expected_type) {
    throw type_error("Passed proto is the wrong type. Expected " +
                     expected_type + " but got " + name + ".");
  }
}

std::string PyProtoSerializeToString(handle py_proto) {
  if (hasattr(py_proto, "SerializeToString"))
    return cast<std::string>(py_proto.attr("SerializeToString")());
  throw std::invalid_argument("Passed python object is not a proto.");
}

template <>
std::unique_ptr<proto2::Message> PyProtoAllocateMessage(handle py_proto,
                                                        kwargs kwargs_in) {
  std::string full_type_name;
  if (isinstance<str>(py_proto)) {
    full_type_name = str(py_proto);
  } else if (!PyProtoFullName(py_proto, &full_type_name)) {
    throw std::invalid_argument("Could not get the name of the proto.");
  }
  const proto2::Descriptor* descriptor =
      proto2::DescriptorPool::generated_pool()->FindMessageTypeByName(
          full_type_name);
  if (!descriptor)
    throw std::runtime_error("Proto Descriptor not found: " + full_type_name);
  return PyProtoAllocateMessage(descriptor, kwargs_in);
}

std::unique_ptr<proto2::Message> PyProtoAllocateMessage(
    const proto2::Descriptor* descriptor, kwargs kwargs_in) {
  const proto2::Message* prototype =
      proto2::MessageFactory::generated_factory()->GetPrototype(descriptor);
  if (!prototype) {
    throw std::runtime_error(
        "Not able to generate prototype for descriptor of: " +
        descriptor->full_name());
  }
  auto message = std::unique_ptr<proto2::Message>(prototype->New());
  ProtoInitFields(message.get(), kwargs_in);
  return message;
}

bool AnyPackFromPyProto(handle py_proto, ::google::protobuf::Any* any_proto) {
  std::string name;
  if (!PyProtoFullName(py_proto, &name)) return false;
  any_proto->set_type_url("type.googleapis.com/" + name);
  any_proto->set_value(PyProtoSerializeToString(py_proto));
  return true;
}

bool AnyUnpackToPyProto(const ::google::protobuf::Any& any_proto,
                        handle py_proto) {
  // Check that py_proto is a proto message of the same type that is stored
  // in the any_proto.
  if (std::string any_type, proto_type;
      !(PyProtoFullName(py_proto, &proto_type) &&
        ::google::protobuf::Any::ParseAnyTypeUrl(
            std::string(any_proto.type_url()), &any_type) &&
        proto_type == any_type))
    return false;
  // Unpack. The serialized string is not copied if py_proto is a wrapped C
  // proto, and copied once if py_proto is a native python proto.
  if (detail::type_caster_base<proto2::Message> caster;
      caster.load(py_proto, false)) {
    return static_cast<proto2::Message&>(caster).ParseFromCord(
        any_proto.value());
  } else {
    bytes serialized(nullptr, any_proto.value().size());
    any_proto.value().CopyToArray(PYBIND11_BYTES_AS_STRING(serialized.ptr()));
    getattr(py_proto, "ParseFromString")(serialized);
    return true;
  }
}

// Throws an out_of_range exception if the index is bad.
void ProtoFieldContainerBase::CheckIndex(int idx, int allowed_size) const {
  if (allowed_size < 0) allowed_size = Size();
  if (idx < 0 || idx >= allowed_size)
    throw std::out_of_range(("Bad index: " + std::to_string(idx) +
                             " (max: " + std::to_string(allowed_size - 1) + ")")
                                .c_str());
}

const proto2::FieldDescriptor* GetFieldDescriptor(
    proto2::Message* message, const std::string& name,
    PyObject* error_type = PyExc_AttributeError) {
  auto* field_desc = message->GetDescriptor()->FindFieldByName(name);
  if (!field_desc) {
    std::string error_str = "'" + message->GetTypeName() +
                            "' object has no attribute '" + name + "'";
    PyErr_SetString(error_type, error_str.c_str());
    throw error_already_set();
  }
  return field_desc;
}

object ProtoGetField(proto2::Message* message, const std::string& name) {
  return ProtoGetField(message, GetFieldDescriptor(message, name));
}

object ProtoGetField(proto2::Message* message,
                     const proto2::FieldDescriptor* field_desc) {
  return DispatchFieldDescriptor<TemplatedProtoGetField>(field_desc, message);
}

void ProtoSetField(proto2::Message* message, const std::string& name,
                   handle value) {
  ProtoSetField(message, GetFieldDescriptor(message, name), value);
}

void ProtoSetField(proto2::Message* message,
                   const proto2::FieldDescriptor* field_desc, handle value) {
  if (field_desc->is_map() || field_desc->is_repeated() ||
      field_desc->type() == proto2::FieldDescriptor::TYPE_MESSAGE) {
    std::string error = "Assignment not allowed to field \"" +
                        field_desc->name() + "\" in protocol message object.";
    PyErr_SetString(PyExc_AttributeError, error.c_str());
    throw error_already_set();
  }
  DispatchFieldDescriptor<TemplatedProtoSetField>(field_desc, message, value);
}

void ProtoInitFields(proto2::Message* message, kwargs kwargs_in) {
  for (auto& item : kwargs_in) {
    DispatchFieldDescriptor<TemplatedProtoSetField>(
        GetFieldDescriptor(message, str(item.first)), message, item.second);
  }
}

std::vector<std::string> MessageFindInitializationErrors(
    proto2::Message* message) {
  std::vector<std::string> errors;
  message->FindInitializationErrors(&errors);
  return errors;
}

std::vector<tuple> MessageListFields(proto2::Message* message) {
  std::vector<const proto2::FieldDescriptor*> fields;
  message->GetReflection()->ListFields(*message, &fields);
  std::vector<tuple> result;
  result.reserve(fields.size());
  for (auto* field_desc : fields) {
    result.push_back(
        make_tuple(cast(field_desc, return_value_policy::reference),
                   ProtoGetField(message, field_desc)));
  }
  return result;
}

bool MessageHasField(proto2::Message* message, const std::string& field_name) {
  auto* field_desc = GetFieldDescriptor(message, field_name, PyExc_ValueError);
  return message->GetReflection()->HasField(*message, field_desc);
}

bytes MessageSerializeAsString(proto2::Message* msg, kwargs kwargs_in) {
  static constexpr char kwargs_key[] = "deterministic";
  std::string result;
  bool deterministic = false;
  if (!kwargs_in.empty()) {
    if (kwargs_in.size() == 1 && kwargs_in.contains(kwargs_key)) {
      deterministic = bool_(kwargs_in[kwargs_key]);
    } else {
      throw std::invalid_argument(
          "Invalid kwargs; the only valid key is 'deterministic'");
    }
  }
  if (deterministic) {
    proto2::io::StringOutputStream string_stream(&result);
    proto2::io::CodedOutputStream coded_stream(&string_stream);
    coded_stream.SetSerializationDeterministic(true);
    msg->SerializeToCodedStream(&coded_stream);
  } else {
    result = msg->SerializeAsString();
  }
  return bytes(result);
}

void MessageCopyFrom(proto2::Message* msg, handle other) {
  PyProtoCheckTypeOrThrow(other, msg->GetTypeName());
  detail::type_caster_base<proto2::Message> caster;
  if (caster.load(other, false)) {
    msg->CopyFrom(static_cast<proto2::Message&>(caster));
  } else {
    if (!msg->ParseFromString(PyProtoSerializeToString(other)))
      throw std::runtime_error("Error copying message.");
  }
}

void MessageMergeFrom(proto2::Message* msg, handle other) {
  PyProtoCheckTypeOrThrow(other, msg->GetTypeName());
  detail::type_caster_base<proto2::Message> caster;
  if (caster.load(other, false)) {
    msg->MergeFrom(static_cast<proto2::Message&>(caster));
  } else {
    if (!msg->MergeFromString(PyProtoSerializeToString(other)))
      throw std::runtime_error("Error merging message.");
  }
}

dict MessageFieldsByName(const proto2::Descriptor* descriptor) {
  dict result;
  for (int i = 0; i < descriptor->field_count(); ++i) {
    auto* field_desc = descriptor->field(i);
    result[cast(field_desc->name())] =
        cast(field_desc, return_value_policy::reference);
  }
  return result;
}

dict EnumValuesByNumber(const proto2::EnumDescriptor* enum_descriptor) {
  dict result;
  for (int i = 0; i < enum_descriptor->value_count(); ++i) {
    auto* value_desc = enum_descriptor->value(i);
    result[cast(value_desc->number())] =
        cast(value_desc, return_value_policy::reference);
  }
  return result;
}

dict EnumValuesByName(const proto2::EnumDescriptor* enum_descriptor) {
  dict result;
  for (int i = 0; i < enum_descriptor->value_count(); ++i) {
    auto* value_desc = enum_descriptor->value(i);
    result[cast(value_desc->name())] =
        cast(value_desc, return_value_policy::reference);
  }
  return result;
}

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
      this->def("add", &RepeatedFieldContainer<T>::Add,
                return_value_policy::reference_internal);
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
      this->def("__setitem__", &MapFieldContainer<T>::SetItem);
    }
    this->def("__repr__", &MapFieldContainer<T>::Repr);
    this->def("__len__", &MapFieldContainer<T>::Size);
    this->def("__contains__", &MapFieldContainer<T>::Contains);
    this->def("__getitem__", &MapFieldContainer<T>::GetItem);
    this->def("__iter__", &MapFieldContainer<T>::KeyIterator,
              keep_alive<0, 1>());
    this->def("keys", &MapFieldContainer<T>::KeyIterator, keep_alive<0, 1>());
    this->def("values", &MapFieldContainer<T>::ValueIterator,
              keep_alive<0, 1>());
    this->def("items", &MapFieldContainer<T>::ItemIterator, keep_alive<0, 1>());
    this->def("update", &MapFieldContainer<T>::UpdateFromDict);
    this->def("update", &MapFieldContainer<T>::UpdateFromKWArgs);
    this->def("clear", &MapFieldContainer<T>::Clear);
    this->def("GetEntryClass", &MapFieldContainer<T>::GetEntryClassFactory,
              "Returns a factory function which can be called with keyword "
              "Arguments to create an instance of the map entry class (ie, "
              "a message with `key` and `value` fields). Used by text_format.");
  }
};

// Class to add python bindings to a map field iterator.
template <typename T>
class MapFieldIteratorBindings
    : public class_<typename MapFieldContainer<T>::Iterator> {
 public:
  MapFieldIteratorBindings(handle scope, const std::string& name)
      : class_<typename MapFieldContainer<T>::Iterator>(
            scope, (name + "Iterator").c_str()) {
    this->def("__iter__", &MapFieldContainer<T>::Iterator::iter);
    this->def("__next__", &MapFieldContainer<T>::Iterator::next);
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
  DefConstantProperty(pyclass, name,
                      std::function<Return(Class, Args...)>(generator), policy);
}

void RegisterProtoBindings(module m) {
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
  class_<proto2::Descriptor> message_desc_c(m, "Descriptor", dynamic_attr());
  DefConstantProperty(&message_desc_c, "fields_by_name", &MessageFieldsByName);
  message_desc_c
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
  class_<proto2::EnumDescriptor> enum_desc_c(m, "EnumDescriptor",
                                             dynamic_attr());
  DefConstantProperty(&enum_desc_c, "values_by_number", &EnumValuesByNumber);
  DefConstantProperty(&enum_desc_c, "values_by_name", &EnumValuesByName);
  enum_desc_c.def_property_readonly("name", &proto2::EnumDescriptor::name);

  // Add bindings for the enum value descriptor class.
  class_<proto2::EnumValueDescriptor>(m, "EnumValueDescriptor")
      .def_property_readonly("name", &proto2::EnumValueDescriptor::name)
      .def_property_readonly("number", &proto2::EnumValueDescriptor::number);

  // Add bindings for the field descriptor class.
  class_<proto2::FieldDescriptor> field_desc_c(m, "FieldDescriptor");
  field_desc_c.def_property_readonly("name", &proto2::FieldDescriptor::name)
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
      .def_property_readonly("label", &proto2::FieldDescriptor::label)
      // Oneof fields are not currently supported.
      .def_property_readonly("containing_oneof", [](void*) { return false; });

  // Add Type enum values.
  enum_<proto2::FieldDescriptor::Type>(field_desc_c, "Type")
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
  enum_<proto2::FieldDescriptor::CppType>(field_desc_c, "CppType")
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
  enum_<proto2::FieldDescriptor::Label>(field_desc_c, "Label")
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
      .def("__getattr__",
           (object(*)(proto2::Message*, const std::string&)) & ProtoGetField)
      .def("__setattr__",
           (void (*)(proto2::Message*, const std::string&, handle)) &
               ProtoSetField)
      .def("SerializeToString", &MessageSerializeAsString)
      .def("ParseFromString", &proto2::Message::ParseFromString, arg("data"))
      .def("MergeFromString", &proto2::Message::MergeFromString, arg("data"))
      .def("ByteSize", &proto2::Message::ByteSizeLong)
      .def("Clear", &proto2::Message::Clear)
      .def("CopyFrom", &MessageCopyFrom)
      .def("MergeFrom", &MessageMergeFrom)
      .def("FindInitializationErrors", &MessageFindInitializationErrors,
           "Slowly build a list of all required fields that are not set.")
      .def("ListFields", &MessageListFields)
      .def("HasField", &MessageHasField)
      // Pickle support is provided only because copy.deepcopy uses it.
      // Do not use it directly; use serialize/parse instead (go/nopickle).
      .def(MakePickler<proto2::Message>())
      .def(
          "SetInParent", [](proto2::Message*) {},
          "No-op. Provided only for compatability with text_format.");

  // Add bindings for the repeated field containers.
  BindEachFieldType<RepeatedFieldBindings>(m, "Repeated");

  // Add bindings for the map field containers.
  BindEachFieldType<MapFieldBindings>(m, "Mapped");

  // Add bindings for the map field iterators.
  BindEachFieldType<MapFieldIteratorBindings>(m, "Mapped");

  // Add bindings for the well-known-types.
  // TODO(rwgk): Consider adding support for other types/methods defined in
  // google3/net/proto2/python/internal/well_known_types.py
  ConcreteProtoMessageBindings<::google::protobuf::Any>(m)
      .def("Is",
           [](const ::google::protobuf::Any& self, handle descriptor) {
             std::string name;
             if (!::google::protobuf::Any::ParseAnyTypeUrl(
                     std::string(self.type_url()), &name))
               return false;
             return name == static_cast<std::string>(
                                str(getattr(descriptor, "full_name")));
           })
      .def("TypeName",
           [](const ::google::protobuf::Any& self) {
             std::string name;
             ::google::protobuf::Any::ParseAnyTypeUrl(
                 std::string(self.type_url()), &name);
             return name;
           })
      .def("Pack",
           [](::google::protobuf::Any* self, handle py_proto) {
             if (!AnyPackFromPyProto(py_proto, self))
               throw std::invalid_argument("Failed to pack Any proto.");
           })
      .def("Unpack", &AnyUnpackToPyProto);
}

}  // namespace google
}  // namespace pybind11

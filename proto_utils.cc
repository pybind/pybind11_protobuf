// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "pybind11_protobuf/proto_utils.h"

#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeindex>

#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"

namespace pybind11 {
namespace google {

// The value of PYBIND11_PROTOBUF_MODULE_PATH will be different depending on
// whether this is being built inside or outside of google3. The value used
// inside of google3 is defined here. Outside of google3, change this value by
// passing "-DPYBIND11_PROTOBUF_MODULE_PATH=..." on the commandline.
#ifndef PYBIND11_PROTOBUF_MODULE_PATH
#define PYBIND11_PROTOBUF_MODULE_PATH google3.third_party.pybind11_protobuf
#endif

void ImportProtoModule() {
  // It's safe to call module::import on an already-imported module, but I'm not
  // sure how efficient that is. This function is called every time a proto is
  // returned from C++ to Python, so use a static bool to ensure this is as
  // fast as possible in the case that the module is already imported.
  static bool imported = false;
  if (!imported) {
    module::import(PYBIND11_TOSTRING(PYBIND11_PROTOBUF_MODULE_PATH) ".proto");
    imported = true;
  }
}

std::optional<std::string> PyProtoFullName(handle py_proto) {
  if (hasattr(py_proto, "DESCRIPTOR")) {
    auto descriptor = py_proto.attr("DESCRIPTOR");
    if (hasattr(descriptor, "full_name"))
      return cast<std::string>(descriptor.attr("full_name"));
  }
  return std::nullopt;
}

bool PyProtoCheckType(handle py_proto, const std::string& expected_type) {
  if (auto optional_name = PyProtoFullName(py_proto))
    return optional_name.value() == expected_type;
  return false;
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
  } else if (auto optional_name = PyProtoFullName(py_proto)) {
    full_type_name = std::move(optional_name).value();
  } else {
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
  auto optional_name = PyProtoFullName(py_proto);
  if (!optional_name) return false;
  any_proto->set_type_url("type.googleapis.com/" + optional_name.value());
  any_proto->set_value(PyProtoSerializeToString(py_proto));
  return true;
}

// Throws an out_of_range exception if the index is bad.
void ProtoFieldContainerBase::CheckIndex(int idx, int allowed_size) const {
  if (allowed_size < 0) allowed_size = Size();
  if (idx < 0 || idx >= allowed_size)
    throw std::out_of_range(("Bad index: " + std::to_string(idx) +
                             " (max: " + std::to_string(allowed_size - 1) + ")")
                                .c_str());
}

template <>
proto2::Message* AddMessage(RepeatedFieldContainer<proto2::Message>* container,
                            kwargs kwargs_in) {
  proto2::Message* message = container->AddDefault();
  ProtoInitFields(message, kwargs_in);
  return message;
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

}  // namespace google
}  // namespace pybind11

// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <memory>
#include <stdexcept>

#include "pybind11_protobuf/proto_casters.h"
#include "pybind11_protobuf/proto_utils.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace py = ::pybind11;

namespace pybind11 {
namespace test {

bool CheckIntMessage(const IntMessage& message, int32_t value) {
  return message.value() == value;
}

bool CheckIntMessagePtr(const IntMessage* message, int32_t value) {
  return message ? (message->value() == value) : false;
}

void MutateIntMessage(int32_t value, IntMessage* message) {
  message->set_value(value);
}

bool CheckAbstractMessage(const ::google::protobuf::Message& message,
                          const std::string& name) {
  return message.GetTypeName() == name;
}

IntMessage& GetIntMessageRef() {
  static IntMessage static_message;
  return static_message;
}

IntMessage* GetIntMessageRawPtr() {
  static IntMessage static_message;
  return &static_message;
}

std::unique_ptr<IntMessage> GetIntMessageUniquePtr() {
  return std::make_unique<IntMessage>();
}

std::shared_ptr<IntMessage> GetIntMessageSharedPtr() {
  return std::make_shared<IntMessage>();
}

::google::protobuf::Message& GetAbstractMessageRef() {
  static IntMessage static_message;
  return static_message;
}

::google::protobuf::Message* GetAbstractMessageRawPtr() {
  static IntMessage static_message;
  return &static_message;
}

std::unique_ptr<::google::protobuf::Message> GetAbstractMessageUniquePtr() {
  return std::make_unique<IntMessage>();
}

std::shared_ptr<::google::protobuf::Message> GetAbstractMessageSharedPtr() {
  return std::make_shared<IntMessage>();
}

PYBIND11_MODULE(proto_example, m) {
  google::ImportProtoModule();

  // Register TestMessage but not IntMessage to demonstrate the effect.
  google::RegisterProtoMessageType<TestMessage>(m);

  m.def("make_test_message", [](py::kwargs kwargs) {
    TestMessage x;
    if (kwargs) {
      py::google::ProtoInitFields(&x, kwargs);
    }
    return x;
  });
  m.def("make_int_message", []() { return IntMessage(); });
  m.def("check_int_message", &CheckIntMessage, arg("message"), arg("value"));
  m.def("check_int_message_ptr", &CheckIntMessagePtr, arg("message"),
        arg("value"));
  m.def("check_int_message_ptr_notnone", &CheckIntMessagePtr,
        arg("message").none(false), arg("value"));

  m.def("mutate_int_message", &MutateIntMessage, arg("value"),
        // Message is an output value, so do not allow conversions.
        arg("message").noconvert());
  m.def("check_abstract_message", &CheckAbstractMessage, arg("message"),
        arg("name"));

  m.def("get_int_message_ref", &GetIntMessageRef,
        return_value_policy::reference);
  m.def("get_int_message_raw_ptr", &GetIntMessageRawPtr,
        return_value_policy::reference);
  m.def("get_int_message_unique_ptr", &GetIntMessageUniquePtr);
  m.def("get_int_message_shared_ptr", &GetIntMessageSharedPtr);

  m.def("get_abstract_message_ref", &GetAbstractMessageRef,
        return_value_policy::reference);
  m.def("get_abstract_message_raw_ptr", &GetAbstractMessageRawPtr,
        return_value_policy::reference);
  m.def("get_abstract_message_unique_ptr", &GetAbstractMessageUniquePtr);
  m.def("get_abstract_message_shared_ptr", &GetAbstractMessageSharedPtr);

  m.def("make_test_message_as_abstract",
        []() -> std::unique_ptr<::google::protobuf::Message> {
          return std::make_unique<TestMessage>();
        });
  m.def("make_int_message_as_abstract",
        []() -> std::unique_ptr<::google::protobuf::Message> {
          return std::make_unique<IntMessage>();
        });
}

}  // namespace test
}  // namespace pybind11

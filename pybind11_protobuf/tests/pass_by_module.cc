// Copyright (c) 2021 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "pybind11_protobuf/native_proto_caster.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace py = ::pybind11;

namespace {

using pybind11::test::IntMessage;

bool CheckIntMessage(const IntMessage* message, int32_t value) {
  return message ? message->value() == value : false;
}

bool CheckMessage(const ::google::protobuf::Message* message, int32_t value) {
  if (!message) return false;
  auto* f = message->GetDescriptor()->FindFieldByName("value");
  if (!f) f = message->GetDescriptor()->FindFieldByName("int_value");
  if (!f) return false;
  return message->GetReflection()->GetInt32(*message, f) == value;
}

IntMessage* GetStatic() {
  static IntMessage* msg = new IntMessage();
  msg->set_value(4);
  return msg;
}

PYBIND11_MODULE(pass_by_module, m) {
  m.attr("PYBIND11_PROTOBUF_UNSAFE") = pybind11::int_(PYBIND11_PROTOBUF_UNSAFE);

  m.def(
      "make_int_message",
      [](int value) -> IntMessage {
        IntMessage msg;
        msg.set_value(value);
        return msg;
      },
      py::arg("value") = 123);

  // constructors
  m.def(
      "make_ptr", []() -> IntMessage* { return new IntMessage(); },
      py::return_value_policy::take_ownership);
  m.def("make_uptr", []() { return std::make_shared<IntMessage>(); });
  m.def("make_sptr",
        []() { return std::unique_ptr<IntMessage>(new IntMessage()); });
  m.def("static_cptr", []() -> const IntMessage* { return GetStatic(); });
  m.def("static_cref", []() -> const IntMessage& { return *GetStatic(); });
  m.def(
      "static_ptr", []() -> IntMessage* { return GetStatic(); },
      py::return_value_policy::copy);
  m.def("static_ref", []() -> IntMessage& { return *GetStatic(); });
  m.def(
      "static_ref_auto", []() -> IntMessage& { return *GetStatic(); },
      py::return_value_policy::automatic_reference);

  // concrete.
  m.def(
      "concrete",
      [](IntMessage message, int value) {
        return CheckIntMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_rval",
      [](IntMessage&& message, int value) {
        return CheckIntMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_cref",
      [](const IntMessage& message, int value) {
        return CheckIntMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_cptr",
      [](const IntMessage* message, int value) {
        return CheckIntMessage(message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_cptr_notnone",
      [](const IntMessage* message, int value) {
        return CheckIntMessage(message, value);
      },
      py::arg("message").none(false), py::arg("value"));
  m.def(
      "concrete_uptr",
      [](std::unique_ptr<IntMessage> message, int value) {
        return CheckIntMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_uptr_ref",
      [](std::unique_ptr<IntMessage>& message, int value) {
        return CheckIntMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_uptr_ptr",
      [](std::unique_ptr<IntMessage>* message, int value) {
        return CheckIntMessage(message->get(), value);
      },
      py::arg("message"), py::arg("value"));

  m.def(
      "concrete_sptr",
      [](std::shared_ptr<IntMessage> message, int value) {
        return CheckIntMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_csptr",
      [](std::shared_ptr<const IntMessage> message, int value) {
        return CheckIntMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_csptr_ref",
      [](std::shared_ptr<const IntMessage>& message, int value) {
        return CheckIntMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));

  m.def(
      "concrete_cwref",
      [](std::reference_wrapper<const IntMessage> message, int value) {
        return CheckMessage(&message.get(), value);
      },
      py::arg("message"), py::arg("value"));

#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "concrete_ref",
      [](IntMessage& message, int value) {
        return CheckIntMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_ptr",
      [](IntMessage* message, int value) {
        return CheckIntMessage(message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "concrete_ptr_notnone",
      [](IntMessage* message, int value) {
        return CheckIntMessage(message, value);
      },
      py::arg("message").none(false), py::arg("value"));
  m.def(
      "concrete_wref",
      [](std::reference_wrapper<IntMessage> message, int value) {
        return CheckMessage(&message.get(), value);
      },
      py::arg("message"), py::arg("value"));
#endif

  // abstract.
  m.def(
      "abstract_rval",
      [](::google::protobuf::Message&& message, int value) {
        return CheckMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_cref",
      [](const ::google::protobuf::Message& message, int value) {
        return CheckMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_cptr",
      [](const ::google::protobuf::Message* message, int value) {
        return CheckMessage(message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_cptr_notnone",
      [](const ::google::protobuf::Message* message, int value) {
        return CheckMessage(message, value);
      },
      py::arg("message").none(false), py::arg("value"));
  m.def(
      "abstract_uptr",
      [](std::unique_ptr<::google::protobuf::Message> message, int value) {
        return CheckMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_sptr",
      [](std::shared_ptr<::google::protobuf::Message> message, int value) {
        return CheckMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_csptr",
      [](std::shared_ptr<const ::google::protobuf::Message> message, int value) {
        return CheckMessage(message.get(), value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_cwref",
      [](std::reference_wrapper<const ::google::protobuf::Message> message, int value) {
        return CheckMessage(&message.get(), value);
      },
      py::arg("message"), py::arg("value"));

#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "abstract_ref",
      [](::google::protobuf::Message& message, int value) {
        return CheckMessage(&message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_ptr",
      [](::google::protobuf::Message* message, int value) {
        return CheckMessage(message, value);
      },
      py::arg("message"), py::arg("value"));
  m.def(
      "abstract_ptr_notnone",
      [](::google::protobuf::Message* message, int value) {
        return CheckMessage(message, value);
      },
      py::arg("message").none(false), py::arg("value"));
  m.def(
      "abstract_wref",
      [](std::reference_wrapper<::google::protobuf::Message> message, int value) {
        return CheckMessage(&message.get(), value);
      },
      py::arg("message"), py::arg("value"));
#endif

  // overloaded functions
  m.def(
      "fn_overload", [](const IntMessage&) -> int { return 2; },
      py::arg("message"));
  m.def(
      "fn_overload", [](const ::google::protobuf::Message&) -> int { return 1; },
      py::arg("message"));
}

}  // namespace

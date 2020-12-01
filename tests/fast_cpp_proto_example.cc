// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "pybind11_protobuf/fast_cpp_proto_casters.h"
#include "pybind11_protobuf/tests/test.proto.h"

namespace pybind11 {
namespace test {

bool CheckIntMessage(const IntMessage& message, int32 value) {
  return message.value() == value;
}

bool CheckMessage(const proto2::Message& message, const std::string& name) {
  return message.GetTypeName() == name;
}

IntMessage* GetIntMessagePtr() {
  static IntMessage static_message;
  return &static_message;
}

std::unique_ptr<IntMessage> GetIntMessageUniquePtr() {
  return std::make_unique<IntMessage>();
}

// pybind11 type_caster<> specialization for std::reference_wrapper<ProtoType>
// does not work correctly unless we allow unsafe conversions.
// See: https://github.com/pybind/pybind11/pull/2705
#if PYBIND11_PROTOBUF_UNSAFE
#define REFERENCE_WRAPPER 1
#else
#define REFERENCE_WRAPPER 0
#endif

PYBIND11_MODULE(fast_cpp_proto_example, m) {
  m.attr("PYBIND11_PROTOBUF_UNSAFE") = pybind11::int_(PYBIND11_PROTOBUF_UNSAFE);
  m.attr("REFERENCE_WRAPPER") = pybind11::int_(REFERENCE_WRAPPER);
  m.def("make_test_message", []() { return TestMessage(); });
  m.def("make_int_message", []() { return IntMessage(); });

  // concrete.
#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "get_int_message_raw_ptr_none", []() -> IntMessage* { return nullptr; },
      return_value_policy::reference);
  m.def(
      "get_int_message_ref",
      []() -> IntMessage& { return *GetIntMessagePtr(); },
      return_value_policy::reference);
  m.def("get_int_message_raw_ptr", &GetIntMessagePtr,
        return_value_policy::reference);
#if REFERENCE_WRAPPER
  m.def(
      "get_int_message_ref_wrapper",
      []() -> std::reference_wrapper<IntMessage> {
        return std::ref(*GetIntMessagePtr());
      },
      return_value_policy::reference);
#endif  // REFERENCE_WRAPPER
#endif  // PYBIND11_PROTOBUF_UNSAFE
  m.def("get_int_message_ptr_copy", &GetIntMessagePtr,
        return_value_policy::copy);
  m.def(
      "get_int_message_ptr_take",
      []() -> IntMessage* { return GetIntMessageUniquePtr().release(); },
      return_value_policy::take_ownership);
  m.def(
      "get_int_message_ref_copy",
      []() -> IntMessage& { return *GetIntMessagePtr(); },
      return_value_policy::copy);
  m.def(
      "get_int_message_const_ref",
      []() -> const IntMessage& { return *GetIntMessagePtr(); },
      return_value_policy::copy);  // can't be reference.
  m.def(
      "get_int_message_const_raw_ptr",
      []() -> const IntMessage* { return GetIntMessagePtr(); },
      return_value_policy::copy);  // can't be reference.
  m.def("get_int_message_unique_ptr", &GetIntMessageUniquePtr);
#if PYBIND11_REFERENCE_WRAPPER
  m.def(
      "get_int_message_ref_wrapper_copy",
      []() -> std::reference_wrapper<IntMessage> {
        return std::ref(*GetIntMessagePtr());
      },
      return_value_policy::copy);
  m.def(
      "get_int_message_const_ref_wrapper",
      []() -> std::reference_wrapper<const IntMessage> {
        return *GetIntMessagePtr();
      },
      return_value_policy::copy);  // can't be reference.
#endif

  // abstract
#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "get_message_ref",
      []() -> proto2::Message& { return *GetIntMessagePtr(); },
      return_value_policy::reference);
  m.def(
      "get_message_raw_ptr",
      []() -> proto2::Message* { return GetIntMessagePtr(); },
      return_value_policy::reference);
#if PYBIND11_REFERENCE_WRAPPER
  m.def(
      "get_message_ref_wrapper",
      []() -> std::reference_wrapper<proto2::Message> {
        return std::ref(*static_cast<proto2::Message*>(GetIntMessagePtr()));
      },
      return_value_policy::reference);
#endif
#endif

  m.def(
      "get_message_const_ref",
      []() -> const proto2::Message& { return *GetIntMessagePtr(); },
      return_value_policy::copy);  // can't be reference.
  m.def(
      "get_message_const_raw_ptr",
      []() -> const proto2::Message* { return GetIntMessagePtr(); },
      return_value_policy::copy);  // can't be reference.
  m.def("get_message_unique_ptr", []() -> std::unique_ptr<proto2::Message> {
    return GetIntMessageUniquePtr();
  });

  // concrete.
  m.def("check_int_message", &CheckIntMessage, arg("message"), arg("value"));
  m.def(
      "check_int_message_const_ptr",
      [](const IntMessage* m, int32 value) {
        return (m == nullptr) ? false : CheckIntMessage(*m, value);
      },
      arg("message"), arg("value"));
  m.def(
      "check_int_message_value",
      [](IntMessage m, int32 value) { return CheckIntMessage(m, value); },
      arg("message"), arg("value"));
  m.def(
      "check_int_message_rvalue",
      [](IntMessage&& m, int32 value) { return CheckIntMessage(m, value); },
      arg("message"), arg("value"));
#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "check_int_message_ptr",
      [](IntMessage* m, int32 value) {  // unsafe
        return (m == nullptr) ? false : CheckIntMessage(*m, value);
      },
      arg("message"), arg("value"));
  m.def(
      "check_int_message_ref",
      [](IntMessage& m, int32 value) {  // unsafe
        return CheckIntMessage(m, value);
      },
      arg("message"), arg("value"));
#endif

  // abstract.
  m.def("check_message", &CheckMessage, arg("message"), arg("name"));
  m.def(
      "check_message_const_ptr",
      [](const proto2::Message* m, const std::string& name) {
        return (m == nullptr) ? false : CheckMessage(*m, name);
      },
      arg("message"), arg("name"));

#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "check_message_ptr",
      [](proto2::Message* m, const std::string& name) {  // unsafe
        return (m == nullptr) ? false : CheckMessage(*m, name);
      },
      arg("message"), arg("name"));
  m.def(
      "check_message_ref",
      [](proto2::Message& m, const std::string& name) {  // unsafe
        return CheckMessage(m, name);
      },
      arg("message"), arg("name"));
#endif

#if PYBIND11_PROTOBUF_UNSAFE
  // in/out
  m.def(
      "mutate_int_message_ptr",
      [](int32 value, IntMessage* m) { m->set_value(value); }, arg("value"),
      arg("message"));

  m.def(
      "mutate_int_message_ref",
      [](int32 value, IntMessage& m) { m.set_value(value); }, arg("value"),
      arg("message"));
#endif

  // std::unique_ptr
  m.def(
      "consume_int_message",
      [](std::unique_ptr<IntMessage> m) { m->set_value(1); }, arg("message"));

  m.def(
      "consume_message",
      [](std::unique_ptr<proto2::Message> m) {
        if (m) m->GetDescriptor();
      },
      arg("message"));

  // overloaded functions
  m.def(
      "fn_overload", [](const IntMessage&) -> int { return 2; },
      arg("message"));
  m.def(
      "fn_overload", [](const proto2::Message&) -> int { return 1; },
      arg("message"));

#if 0
  // std::shared_ptr support does not work yet.
  // Wait for https://github.com/pybind/pybind11/pull/2672

  m.def("get_int_message_shared_ptr", []() -> std::shared_ptr<IntMessage> {
    return GetIntMessageUniquePtr();
  });
  m.def("get_message_shared_ptr", []() -> std::shared_ptr<proto2::Message> {
    return GetIntMessageUniquePtr();
  });

  m.def(
      "mutate_int_message_shared_ptr",
      [](int32 value, std::shared_ptr<IntMessage> message) {
        message->set_value(value);
      },
      arg("value"), arg("message").noconvert());
#endif
}

}  // namespace test
}  // namespace pybind11

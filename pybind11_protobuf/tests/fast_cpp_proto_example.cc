// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "net/proto2/contrib/parse_proto/parse_text_proto.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "pybind11_protobuf/fast_cpp_proto_casters.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace pybind11 {
namespace test {

// This constructs a dynamic message that is wire-compatible with
// with IntMessage; conversion using the fast_cpp_proto api will fail
// as the PyProto_API will not be able to find the proto in the default
// pool.
::google::protobuf::Message* GetDynamicMessagePtr() {
  static ::google::protobuf::DescriptorPool pool;
  static ::google::protobuf::DynamicMessageFactory factory(&pool);

  static ::google::protobuf::Message* dynamic = [&] {
    ::google::protobuf::FileDescriptorProto file_proto =
        ::google::protobuf::contrib::parse_proto::ParseTextProtoOrDie(R"pb(
          name: 'pybind11_protobuf/tests'
          package: 'pybind11.test'
          message_type: {
            name: 'DynamicIntMessage'
            field: { name: 'value' number: 1 type: TYPE_INT32 }
          }
        )pb");

    pool.BuildFile(file_proto);

    auto* descriptor =
        pool.FindMessageTypeByName("pybind11.test.DynamicIntMessage");
    assert(descriptor != nullptr);

    auto* prototype = factory.GetPrototype(descriptor);
    assert(descriptor != nullptr);

    auto* dynamic = prototype->New();
    dynamic->GetReflection()->SetInt32(
        dynamic, dynamic->GetDescriptor()->FindFieldByName("value"), 123);
    return dynamic;
  }();

  return dynamic;
}

bool CheckIntMessage(const IntMessage& message, int32 value) {
  return message.value() == value;
}

bool CheckMessage(const ::google::protobuf::Message& message, int32 value) {
  auto* f = message.GetDescriptor()->FindFieldByName("value");
  if (!f) f = message.GetDescriptor()->FindFieldByName("value_int32");
  return message.GetReflection()->GetInt32(message, f) == value;
}

IntMessage* GetIntMessagePtr() {
  static IntMessage static_message;
  return &static_message;
}

std::unique_ptr<IntMessage> GetIntMessageUniquePtr() {
  return std::make_unique<IntMessage>();
}

std::shared_ptr<IntMessage> GetIntMessageSharedPtr() {
  return std::make_shared<IntMessage>();
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
  m.def("get_int_message_unique_ptr", &GetIntMessageSharedPtr);
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
      []() -> ::google::protobuf::Message& { return *GetIntMessagePtr(); },
      return_value_policy::reference);
  m.def(
      "get_message_raw_ptr",
      []() -> ::google::protobuf::Message* { return GetIntMessagePtr(); },
      return_value_policy::reference);
#if PYBIND11_REFERENCE_WRAPPER
  m.def(
      "get_message_ref_wrapper",
      []() -> std::reference_wrapper<::google::protobuf::Message> {
        return std::ref(*static_cast<::google::protobuf::Message*>(GetIntMessagePtr()));
      },
      return_value_policy::reference);
#endif
#endif
  m.def(
      "get_message_const_ref",
      []() -> const ::google::protobuf::Message& { return *GetIntMessagePtr(); },
      return_value_policy::copy);  // can't be reference.
  m.def(
      "get_message_const_raw_ptr",
      []() -> const ::google::protobuf::Message* { return GetIntMessagePtr(); },
      return_value_policy::copy);  // can't be reference.
  m.def("get_message_unique_ptr", []() -> std::unique_ptr<::google::protobuf::Message> {
    return GetIntMessageUniquePtr();
  });
  m.def("get_message_shared_ptr", []() -> std::shared_ptr<::google::protobuf::Message> {
    return GetIntMessageSharedPtr();
  });

  // dynamic
  m.def(
      "get_dynamic",
      []() -> ::google::protobuf::Message& { return *GetDynamicMessagePtr(); },
      return_value_policy::copy);

  // concrete.
  m.def("check_int_message", &CheckIntMessage, arg("message"), arg("value"));
  m.def(
      "check_int_message_const_ptr",
      [](const IntMessage* m, int32 value) {
        return (m == nullptr) ? false : CheckIntMessage(*m, value);
      },
      arg("message"), arg("value"));

  m.def(
      "check_int_message_const_ptr_notnone",
      [](const IntMessage* m, int32 value) {
        return (m == nullptr) ? false : CheckIntMessage(*m, value);
      },
      arg("message").none(false), arg("value"));

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
  m.def("check_message", &CheckMessage, arg("message"), arg("value"));
  m.def(
      "check_message_const_ptr",
      [](const ::google::protobuf::Message* m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "check_message_ptr",
      [](::google::protobuf::Message* m, int value) {  // unsafe
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));
  m.def(
      "check_message_ref",
      [](::google::protobuf::Message& m, int value) {  // unsafe
        return CheckMessage(m, value);
      },
      arg("message"), arg("value"));
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
      [](std::unique_ptr<IntMessage> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

  m.def(
      "consume_int_message_const",
      [](std::unique_ptr<const IntMessage> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

  m.def(
      "consume_int_message_notnone",
      [](std::unique_ptr<IntMessage> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message").none(false), arg("value"));

  m.def(
      "consume_message",
      [](std::unique_ptr<::google::protobuf::Message> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

  m.def(
      "consume_message_notnone",
      [](std::unique_ptr<::google::protobuf::Message> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message").none(false), arg("value"));

  m.def(
      "consume_shared_int_message",
      [](std::shared_ptr<IntMessage> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

  m.def(
      "consume_shared_int_message_const",
      [](std::shared_ptr<const IntMessage> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

  m.def(
      "consume_shared_message",
      [](std::shared_ptr<IntMessage> m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      arg("message"), arg("value"));

  // overloaded functions
  m.def(
      "fn_overload", [](const IntMessage&) -> int { return 2; },
      arg("message"));
  m.def(
      "fn_overload", [](const ::google::protobuf::Message&) -> int { return 1; },
      arg("message"));


#if 0
  // std::shared_ptr support does not work yet.
  // Wait for https://github.com/pybind/pybind11/pull/2672

  m.def("get_int_message_shared_ptr", []() -> std::shared_ptr<IntMessage> {
    return GetIntMessageUniquePtr();
  });
  m.def("get_message_shared_ptr", []() -> std::shared_ptr<::google::protobuf::Message> {
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

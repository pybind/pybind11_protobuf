// Copyright (c) 2021 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "google/protobuf/dynamic_message.h"
#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "pybind11_protobuf/tests/test.pb.h"
#include "pybind11_protobuf/wrapped_proto_caster.h"

namespace py = ::pybind11;

namespace {

using pybind11::google::WithWrappedProtos;
using pybind11::google::WrappedProto;
using pybind11::google::WrappedProtoKind;
using pybind11::test::IntMessage;
using pybind11::test::TestMessage;

const TestMessage& GetStatic() {
  static TestMessage test_message = [] {
    TestMessage msg;
    msg.set_int_value(123);
    return msg;
  }();

  return test_message;
}

bool CheckMessage(const ::google::protobuf::Message* message, int32_t value) {
  if (!message) return false;
  auto* f = message->GetDescriptor()->FindFieldByName("value");
  if (!f) f = message->GetDescriptor()->FindFieldByName("int_value");
  if (!f) return false;
  return message->GetReflection()->GetInt32(*message, f) == value;
}

bool CheckIntMessage(const IntMessage* message, int32_t value) {
  return CheckMessage(message, value);
}

PYBIND11_MODULE(wrapped_proto_module, m) {
  pybind11_protobuf::ImportWrappedProtoCasters();

  m.def("get_test_message", WithWrappedProtos(GetStatic));

  m.def("make_int_message", WithWrappedProtos([](int value) -> IntMessage {
          IntMessage msg;
          msg.set_value(value);
          return msg;
        }),
        py::arg("value") = 123);

  m.def("fn_overload",
        [](WrappedProto<IntMessage, WrappedProtoKind::kConst> proto) {
          return 1;
        });
  m.def("fn_overload", [](const IntMessage& proto) { return 2; });

  m.def("check_int", WithWrappedProtos(&CheckIntMessage), py::arg("message"),
        py::arg("value"));

  // Check calls.
  m.def("check", WithWrappedProtos(&CheckMessage), py::arg("message"),
        py::arg("value"));

  m.def("check_cref",
        WithWrappedProtos([](const TestMessage& msg, int32_t value) {
          return CheckMessage(&msg, value);
        }),
        py::arg("proto"), py::arg("value"));
  m.def("check_cptr",
        WithWrappedProtos([](const TestMessage* msg, int32_t value) {
          return CheckMessage(msg, value);
        }),
        py::arg("proto"), py::arg("value"));
  m.def("check_val", WithWrappedProtos([](TestMessage msg, int32_t value) {
          return CheckMessage(&msg, value);
        }),
        py::arg("proto"), py::arg("value"));
  m.def("check_rval", WithWrappedProtos([](TestMessage&& msg, int32_t value) {
          return CheckMessage(&msg, value);
        }),
        py::arg("proto"), py::arg("value"));

  // WithWrappedProto does not auto-wrap mutable protos, but constructing a
  // wrapper manually will still work. Note, however, that the proto will be
  // copied.
  m.def(
      "check_mutable",
      [](WrappedProto<TestMessage, WrappedProtoKind::kMutable> msg,
         int32_t value) {
        return CheckMessage(static_cast<TestMessage*>(msg), value);
      },
      py::arg("proto"), py::arg("value"));
}

/// Below here are compile tests for fast_cpp_proto_casters
int GetInt();
const TestMessage& GetConstReference();
const TestMessage* GetConstPtr();
TestMessage GetValue();
TestMessage&& GetRValue();
absl::StatusOr<TestMessage> GetStatusOr();
absl::optional<TestMessage> GetOptional();

void PassInt(int);
void PassConstReference(const TestMessage&);
void PassConstPtr(const TestMessage*);
void PassValue(TestMessage);
void PassRValue(TestMessage&&);
void PassOptional(absl::optional<TestMessage>);

struct Struct {
  TestMessage MemberFn();
  TestMessage ConstMemberFn() const;
};

void test_static_asserts() {
  using pybind11::google::WithWrappedProtos;
  using pybind11::google::WrappedProto;
  using pybind11::google::WrappedProtoKind;
  using pybind11::google::impl::WrapHelper;
  using pybind11::test::IntMessage;
  using pybind11::test::TestMessage;

  static_assert(
      std::is_same_v<WrappedProto<IntMessage, WrappedProtoKind::kConst>,
                     WrapHelper<const IntMessage&>::type>,
      "");

  static_assert(
      std::is_same_v<WrappedProto<IntMessage, WrappedProtoKind::kConst>,
                     WrapHelper<const IntMessage*>::type>,
      "");

  static_assert(
      std::is_same_v<WrappedProto<IntMessage, WrappedProtoKind::kValue>,
                     WrapHelper<IntMessage>::type>,
      "");

  static_assert(
      std::is_same_v<WrappedProto<IntMessage, WrappedProtoKind::kValue>,
                     WrapHelper<IntMessage&&>::type>,
      "");

  // The asserts below ensure that the generated wrappers have the expected type
  // signatures. Return types are checked against invoke_result_t, and parameter
  // types are checked using is_invocable_v.
  static_assert(
      std::is_same_v<
          int, std::invoke_result_t<decltype(WithWrappedProtos(&GetInt))>>,
      "");

  static_assert(
      std::is_same_v<WrappedProto<TestMessage, WrappedProtoKind::kConst>,
                     std::invoke_result_t<decltype(WithWrappedProtos(
                         &GetConstReference))>>,
      "");

  static_assert(
      std::is_same_v<
          WrappedProto<TestMessage, WrappedProtoKind::kConst>,
          std::invoke_result_t<decltype(WithWrappedProtos(&GetConstPtr))>>,
      "");

  static_assert(
      std::is_same_v<
          WrappedProto<TestMessage, WrappedProtoKind::kValue>,
          std::invoke_result_t<decltype(WithWrappedProtos(&GetValue))>>,
      "");

  static_assert(
      std::is_same_v<
          WrappedProto<TestMessage, WrappedProtoKind::kValue>,
          std::invoke_result_t<decltype(WithWrappedProtos(&GetRValue))>>,
      "");

  // members
  static_assert(
      std::is_same_v<
          WrappedProto<TestMessage, WrappedProtoKind::kValue>,
          std::invoke_result_t<decltype(WithWrappedProtos(&Struct::MemberFn)),
                               Struct&>>,
      "");

  static_assert(
      std::is_same_v<
          WrappedProto<TestMessage, WrappedProtoKind::kValue>,
          std::invoke_result_t<
              decltype(WithWrappedProtos(&Struct::ConstMemberFn)), Struct&>>,
      "");

  // statusor
  static_assert(
      std::is_same_v<
          absl::StatusOr<WrappedProto<TestMessage, WrappedProtoKind::kValue>>,
          std::invoke_result_t<decltype(WithWrappedProtos(&GetStatusOr))>>,
      "");

  // optional
  static_assert(
      std::is_same_v<
          absl::optional<WrappedProto<TestMessage, WrappedProtoKind::kValue>>,
          std::invoke_result_t<decltype(WithWrappedProtos(&GetOptional))>>,
      "");

  static_assert(
      std::is_invocable_v<decltype(WithWrappedProtos(&PassOptional)),
                          WrappedProto<TestMessage, WrappedProtoKind::kValue>>,
      "");

  /// calling
  static_assert(std::is_invocable_v<decltype(WithWrappedProtos(&PassInt)), int>,
                "");

  static_assert(
      std::is_invocable_v<decltype(WithWrappedProtos(&PassConstReference)),
                          WrappedProto<TestMessage, WrappedProtoKind::kConst>>,
      "");

  static_assert(
      std::is_invocable_v<decltype(WithWrappedProtos(&PassConstPtr)),
                          WrappedProto<TestMessage, WrappedProtoKind::kConst>>,
      "");

  static_assert(
      std::is_invocable_v<decltype(WithWrappedProtos(&PassValue)),
                          WrappedProto<TestMessage, WrappedProtoKind::kValue>>,
      "");

  static_assert(
      std::is_invocable_v<decltype(WithWrappedProtos(&PassRValue)),
                          WrappedProto<TestMessage, WrappedProtoKind::kValue>>,
      "");
}

#if defined(WRAPPED_PROTO_CASTER_NONCOMPILE_TEST)
// This code could be added as a non-compile test.
//
// It exercises the WithWrappedProtos(...) codepaths when called with mutable
// protos, and is expected to fail with a static_assert.
//
TestMessage& GetReference();
TestMessage* GetPtr();
void PassPtr(TestMessage*);
void PassReference(TestMessage&);

void test_wrapping_disabled() {
  // Automatically wrapping mutable methods fails.

  static_assert(
      std::is_same_v<
          WrappedProto<TestMessage, WrappedProtoKind::kMutable>,
          std::invoke_result_t<decltype(WithWrappedProtos(&GetReference))>>,
      "");

  static_assert(std::is_same_v<
                    WrappedProto<TestMessage, WrappedProtoKind::kMutable>,
                    std::invoke_result_t<decltype(WithWrappedProtos(&GetPtr))>>,
                "");

  static_assert(std::is_invocable_v<
                    decltype(WithWrappedProtos(&PassReference)),
                    WrappedProto<TestMessage, WrappedProtoKind::kMutable>>,
                "");

  static_assert(std::is_invocable_v<
                    decltype(WithWrappedProtos(&PassPtr)),
                    WrappedProto<TestMessage, WrappedProtoKind::kMutable>>,
                "");
}
#endif  // WRAPPED_PROTO_CASTER_NONCOMPILE_TEST

}  // namespace

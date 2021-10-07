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
int GetInt() { return 0; }
static TestMessage kMessage;
const TestMessage& GetConstReference() { return kMessage; }
const TestMessage* GetConstPtr() { return &kMessage; }
TestMessage GetValue() { return TestMessage(); }
// Note that this should never actually run.
TestMessage&& GetRValue() { return std::move(kMessage); }
absl::StatusOr<TestMessage> GetStatusOr() { return TestMessage(); }
absl::optional<TestMessage> GetOptional() { return TestMessage(); }

void PassInt(int) {}
void PassConstReference(const TestMessage&) {}
void PassConstPtr(const TestMessage*) {}
void PassValue(TestMessage) {}
void PassRValue(TestMessage&&) {}
void PassOptional(absl::optional<TestMessage>) {}

struct Struct {
  TestMessage MemberFn() { return kMessage; }
  TestMessage ConstMemberFn() const { return kMessage; }
};

void test_static_asserts() {
  using pybind11::google::WithWrappedProtos;
  using pybind11::google::WrappedProto;
  using pybind11::google::WrappedProtoKind;
  using pybind11::google::impl::WrapHelper;
  using pybind11::test::IntMessage;
  using pybind11::test::TestMessage;

  static_assert(std::is_same<WrappedProto<IntMessage, WrappedProtoKind::kConst>,
                             WrapHelper<const IntMessage&>::type>::value,
                "");

  static_assert(std::is_same<WrappedProto<IntMessage, WrappedProtoKind::kConst>,
                             WrapHelper<const IntMessage*>::type>::value,
                "");

  static_assert(std::is_same<WrappedProto<IntMessage, WrappedProtoKind::kValue>,
                             WrapHelper<IntMessage>::type>::value,
                "");

  static_assert(std::is_same<WrappedProto<IntMessage, WrappedProtoKind::kValue>,
                             WrapHelper<IntMessage&&>::type>::value,
                "");

  // These function refs ensure that the generated wrappers have the expected
  // type signatures.
  // Return types
  absl::FunctionRef<int()>(WithWrappedProtos(GetInt));
  absl::FunctionRef<const TestMessage&()>(WithWrappedProtos(GetConstReference));
  absl::FunctionRef<const TestMessage*()>(WithWrappedProtos(GetConstPtr));
  absl::FunctionRef<TestMessage()>(WithWrappedProtos(GetValue));
  absl::FunctionRef<TestMessage && ()>(WithWrappedProtos(GetRValue));
  absl::FunctionRef<TestMessage(Struct&)>(WithWrappedProtos(&Struct::MemberFn));
  absl::FunctionRef<TestMessage(const Struct&)>(
      WithWrappedProtos(&Struct::ConstMemberFn));
  absl::FunctionRef<absl::StatusOr<TestMessage>()>(
      WithWrappedProtos(GetStatusOr));
  absl::FunctionRef<absl::optional<TestMessage>()>(
      WithWrappedProtos(GetOptional));

  // Passing types
  absl::FunctionRef<void(int)>(WithWrappedProtos(PassInt));
  absl::FunctionRef<void(const TestMessage&)>(
      WithWrappedProtos(PassConstReference));
  absl::FunctionRef<void(const TestMessage*)>(WithWrappedProtos(PassConstPtr));
  absl::FunctionRef<void(TestMessage)>(WithWrappedProtos(PassValue));
  absl::FunctionRef<void(TestMessage &&)>(WithWrappedProtos(PassRValue));
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
  absl::FunctionRef<TestMessage&()>(WithWrappedProtos(GetReference));
  absl::FunctionRef<TestMessage*()>(WithWrappedProtos(GetPtr));
  absl::FunctionRef<void(TestMessage*)>(WithWrappedProtos(PassPtr));
  absl::FunctionRef<void(TestMessage&)>(WithWrappedProtos(PassReference));
}
#endif  // WRAPPED_PROTO_CASTER_NONCOMPILE_TEST

}  // namespace

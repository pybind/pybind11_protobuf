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
#include "google/protobuf/text_format.h"
#include "pybind11_protobuf/native_proto_caster.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace py = ::pybind11;

namespace {

// GetDynamicPool contains a dynamic message that is wire-compatible with
// with IntMessage; conversion using the fast_cpp_proto api will fail
// as the PyProto_API will not be able to find the proto in the default
// pool.
::google::protobuf::DescriptorPool* GetDynamicPool() {
  static ::google::protobuf::DescriptorPool* pool = [] {
    ::google::protobuf::FileDescriptorProto file_proto;
    if (!::google::protobuf::TextFormat::ParseFromString(
            R"pb(
              name: 'pybind11_protobuf/tests'
              package: 'pybind11.test'
              message_type: {
                name: 'DynamicMessage'
                field: { name: 'value' number: 1 type: TYPE_INT32 }
              }
              message_type: {
                name: 'IntMessage'
                field: { name: 'value' number: 1 type: TYPE_INT32 }
              }
            )pb",
            &file_proto)) {
      throw std::invalid_argument("Failed to parse textproto.");
    }

    ::google::protobuf::DescriptorPool* pool = new ::google::protobuf::DescriptorPool();
    pool->BuildFile(file_proto);
    return pool;
  }();

  return pool;
}

void UpdateMessage(::google::protobuf::Message* message, int32_t value) {
  auto* f = message->GetDescriptor()->FindFieldByName("value");
  if (!f) f = message->GetDescriptor()->FindFieldByName("int_value");
  if (!f) return;
  message->GetReflection()->SetInt32(message, f, value);
}

bool CheckMessage(const ::google::protobuf::Message& message, int32_t value) {
  auto* f = message.GetDescriptor()->FindFieldByName("value");
  if (!f) f = message.GetDescriptor()->FindFieldByName("int_value");
  if (!f) return false;
  return message.GetReflection()->GetInt32(message, f) == value;
}

std::unique_ptr<::google::protobuf::Message> GetDynamicMessage(const std::string& full_name,
                                                   int32_t value) {
  static ::google::protobuf::DynamicMessageFactory factory(GetDynamicPool());

  auto* descriptor = GetDynamicPool()->FindMessageTypeByName(full_name);
  if (!descriptor) return nullptr;

  auto* prototype = factory.GetPrototype(descriptor);
  if (!prototype) return nullptr;

  std::unique_ptr<::google::protobuf::Message> dynamic(prototype->New());
  UpdateMessage(dynamic.get(), value);
  return dynamic;
}

PYBIND11_MODULE(dynamic_message_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def(
      "dynamic_message_ptr",
      [](std::string name, int32_t value) -> ::google::protobuf::Message* {
        return GetDynamicMessage(name, value).release();
      },
      py::arg("name") = "pybind11.test.DynamicMessage", py::arg("value") = 123);

  m.def(
      "dynamic_message_unique_ptr",
      [](std::string name, int32_t value) -> std::unique_ptr<::google::protobuf::Message> {
        return GetDynamicMessage(name, value);
      },
      py::arg("name") = "pybind11.test.DynamicMessage", py::arg("value") = 123);

  m.def(
      "dynamic_message_shared_ptr",
      [](std::string name, int32_t value) -> std::shared_ptr<::google::protobuf::Message> {
        return GetDynamicMessage(name, value);
      },
      py::arg("name") = "pybind11.test.DynamicMessage", py::arg("value") = 123);

  m.def("check_message", &CheckMessage, py::arg("message"), py::arg("value"));
  m.def(
      "check_message_const_ptr",
      [](const ::google::protobuf::Message* m, int value) {
        return (m == nullptr) ? false : CheckMessage(*m, value);
      },
      py::arg("message"), py::arg("value"));

#if PYBIND11_PROTOBUF_UNSAFE
  m.def(
      "mutate_message",
      [](::google::protobuf::Message* msg, int value) { UpdateMessage(msg, value); },
      py::arg("message"), py::arg("value"));

  m.def(
      "mutate_message_ref",
      [](::google::protobuf::Message& msg, int value) { UpdateMessage(&msg, value); },
      py::arg("message"), py::arg("value"));
#endif  // PYBIND11_PROTOBUF_UNSAFE

  // overloaded functions
  m.def(
      "fn_overload", [](const ::google::protobuf::Message&) -> int { return 1; },
      py::arg("message"));
  m.def(
      "fn_overload", [](const pybind11::test::IntMessage&) -> int { return 2; },
      py::arg("message"));

  // copies
  m.def(
      "roundtrip",
      [](const ::google::protobuf::Message& inout) -> const ::google::protobuf::Message& {
        return inout;
      },
      py::arg("message"), py::return_value_policy::copy);
}

}  // namespace

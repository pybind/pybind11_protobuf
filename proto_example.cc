// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include "pybind11_protobuf/proto_casters.h"
#include "pybind11_protobuf/test.proto.h"
#include "util/task/status.h"

namespace pybind11 {
namespace test {

bool CheckTestMessage(const TestMessage& message, int32 value) {
  return message.int_value() == value;
}

void MutateTestMessage(int32 value, TestMessage* message) {
  message->set_int_value(value);
}

bool CheckGenericMessage(const proto2::Message& message,
                         const std::string& name) {
  return message.GetTypeName() == name;
}

::google::protobuf::Any MakeAnyMessage(handle py_handle) {
  ::google::protobuf::Any any_proto;
  google::AnyPackFromPyProto(py_handle, &any_proto);
  return any_proto;
}

PYBIND11_MODULE(proto_example, m) {
  m.def("make_test_message", []() { return TestMessage(); });
  m.def("make_int_message", []() { return IntMessage(); });
  m.def("check_test_message", &CheckTestMessage, arg("message"), arg("value"));
  m.def("mutate_test_message", &MutateTestMessage, arg("value"),
        // Message is an output value, so do not allow conversions.
        arg("message").noconvert());
  m.def("check_generic_message", &CheckGenericMessage, arg("message"),
        arg("name"));
  m.def("make_any_message", &MakeAnyMessage, arg("message"));
}

}  // namespace test
}  // namespace pybind11

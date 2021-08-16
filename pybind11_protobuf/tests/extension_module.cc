// Copyright (c) 2021 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "google/protobuf/message.h"
#include "pybind11_protobuf/native_proto_caster.h"
#include "pybind11_protobuf/tests/extension.pb.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace py = ::pybind11;

namespace {

using pybind11::test::BaseMessage;
using pybind11::test::IntMessage;

std::unique_ptr<BaseMessage> GetMessage(int32_t value) {
  std::unique_ptr<BaseMessage> msg(new BaseMessage());
  msg->MutableExtension(pybind11::test::int_message)->set_value(value);
  return msg;
}

const IntMessage& GetExtension(const BaseMessage& msg) {
  return msg.GetExtension(pybind11::test::int_message);
}

PYBIND11_MODULE(extension_module, m) {
  m.def("get_base_message", []() -> BaseMessage { return {}; });

  m.def(
      "get_message",
      [](int32_t value) -> BaseMessage {
        return std::move(*GetMessage(value));
      },
      py::arg("value") = 123);

  m.def("get_extension", &GetExtension, py::arg("message"));

  // copies
  m.def(
      "roundtrip",
      [](const BaseMessage& inout) -> const BaseMessage& { return inout; },
      py::arg("message"), py::return_value_policy::copy);
}

}  // namespace

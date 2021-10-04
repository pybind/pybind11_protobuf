// Copyright (c) 2021 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "pybind11_protobuf/native_proto_caster.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace py = ::pybind11;

using pybind11::test::TestMessage;

namespace {

PYBIND11_MODULE(thread_module, m) {
  pybind11_protobuf::RegisterNativeProtoCasters();

  m.def(
      "make_message",
      [](std::string text) -> TestMessage {
        TestMessage msg;
        msg.set_string_value(std::move(text));
        return msg;
      },
      py::arg("text") = "");

  m.def(
      "make_message_string_view",
      [](std::string_view text) -> TestMessage {
        TestMessage msg;
        msg.set_string_value(std::string(text));
        return msg;
      },
      py::arg("text") = "");

  m.def(
      "make_message_no_gil",
      [](std::string text) -> TestMessage {
        TestMessage msg;
        msg.set_string_value(std::move(text));
        return msg;
      },
      py::arg("text") = "", py::call_guard<py::gil_scoped_release>());

  m.def(
      "make_message_string_view_no_gil",
      [](std::string_view text) -> TestMessage {
        TestMessage msg;
        msg.set_string_value(std::string(text));
        return msg;
      },
      py::arg("text") = "", py::call_guard<py::gil_scoped_release>());
}

}  // namespace

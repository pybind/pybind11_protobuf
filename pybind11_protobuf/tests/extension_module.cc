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
#include "pybind11_protobuf/tests/extension_nest_repeated.pb.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace py = ::pybind11;

namespace {

using pybind11::test::BaseMessage;

std::unique_ptr<BaseMessage> GetMessage(int32_t value) {
  std::unique_ptr<BaseMessage> msg(new BaseMessage());
  msg->MutableExtension(pybind11::test::int_message)->set_value(value);
  return msg;
}

const pybind11::test::IntMessage& GetExtension(const BaseMessage& msg) {
  return msg.GetExtension(pybind11::test::int_message);
}

template <typename ProtoType>
void DefReserialize(py::module_& m, const char* py_name) {
  m.def(
      py_name,
      [](const ProtoType& message) -> ProtoType {
        std::string serialized;
        message.SerializeToString(&serialized);
        ProtoType deserialized;
        deserialized.ParseFromString(serialized);
        return deserialized;
      },
      py::arg("message"));
}

PYBIND11_MODULE(extension_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

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

  DefReserialize<BaseMessage>(m, "reserialize_base_message");
  DefReserialize<pybind11::test::NestLevel2>(m, "reserialize_nest_level2");
  DefReserialize<pybind11::test::NestRepeated>(m, "reserialize_nest_repeated");

  pybind11_protobuf::AllowUnknownFieldsFor("pybind11.test.AllowUnknownInner",
                                           "");
  DefReserialize<pybind11::test::AllowUnknownInner>(
      m, "reserialize_allow_unknown_inner");

  pybind11_protobuf::AllowUnknownFieldsFor("pybind11.test.AllowUnknownOuter",
                                           "inner");
  DefReserialize<pybind11::test::AllowUnknownOuter>(
      m, "reserialize_allow_unknown_outer");
}

}  // namespace

// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <functional>
#include <memory>
#include <stdexcept>

#include "pybind11_protobuf/enum_type_caster.h"
#include "pybind11_protobuf/tests/test.pb.h"

namespace pybind11 {
namespace test {
// Disabled to use py::enum_
constexpr bool pybind11_protobuf_enable_enum_type_caster(
    AnotherMessage::AnotherEnum*) {
  return false;
}
}  // namespace test
}  // namespace pybind11

namespace py = ::pybind11;

namespace {

using pybind11::test::AnotherMessage;
using pybind11::test::TestMessage;

PYBIND11_MODULE(proto_enum_module, m) {
  // TestMessage::TestEnum does use the pybind11_protobuf type_caster, so
  // registering py::enum_ is not allowed. But creating similar enum constants
  // is allowed.
  m.attr("ZERO") = TestMessage::ZERO;
  m.attr("ONE") = TestMessage::ONE;
  m.attr("TWO") = TestMessage::TWO;

  // enum functions
  m.def(
      "adjust_enum",
      [](TestMessage::TestEnum e) -> TestMessage::TestEnum {
        switch (e) {
          case TestMessage::ZERO:
            return TestMessage::ONE;
          case TestMessage::ONE:
            return TestMessage::TWO;
          case TestMessage::TWO:
            return TestMessage::ZERO;
          default:
            /// likely an error
            return TestMessage::ZERO;
        }
      },
      py::arg("enum"));

  /// AnotherProto::AnotherEnum does not use pybind11_protobuf::type_caster,
  /// so it's fair game to define it here.
  py::enum_<AnotherMessage::AnotherEnum>(m, "AnotherEnum")
      .value("ZERO", AnotherMessage::ZERO)
      .value("ONE", AnotherMessage::ONE)
      .value("TWO", AnotherMessage::TWO);

  m.def(
      "adjust_another_enum",
      [](AnotherMessage::AnotherEnum e) -> AnotherMessage::AnotherEnum {
        switch (e) {
          case AnotherMessage::ZERO:
            return AnotherMessage::ONE;
          case AnotherMessage::ONE:
            return AnotherMessage::TWO;
          case AnotherMessage::TWO:
            return AnotherMessage::ZERO;
          default:
            /// likely an error
            return AnotherMessage::ZERO;
        }
      },
      py::arg("enum"));
}

}  // namespace

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

PYBIND11_MODULE(proto_enum_module, m) {
  // Though registering py::enum_ types for ::google::protobuf::Enum classes is not
  // allowed, creating similar enum constants is allowded.
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
      arg("enum"));
}

}  // namespace test
}  // namespace pybind11

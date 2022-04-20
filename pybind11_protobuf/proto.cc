// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

namespace pybind11 {
namespace google {

PYBIND11_MODULE(proto, m) {
  // Return whether the given python object is a wrapped C proto.
  m.def(
      "is_wrapped_c_proto", [](object x) { return false; }, arg("src"));
}

}  // namespace google
}  // namespace pybind11

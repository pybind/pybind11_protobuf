// Copyright (c) 2024 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include "pybind11_protobuf/native_proto_caster.h"
#include "pybind11_protobuf/tests/we-love-dashes.pb.h"

namespace {

PYBIND11_MODULE(we_love_dashes_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def("return_token_effort", [](int score) {
    pybind11::test::TokenEffort msg;
    msg.set_score(score);
    return msg;
  });

  m.def("pass_token_effort",
        [](const pybind11::test::TokenEffort& msg) { return msg.score(); });
}

}  // namespace

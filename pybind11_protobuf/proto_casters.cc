// Copyright (c) 2020 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "pybind11_protobuf/proto_casters.h"

void pybind11_proto_casters_collision() {
  // This symbol intentionally defined to cause ODR violations. It exists in:
  //   * proto_casters.cc
  //   * native_proto_caster.cc

  // Avoid mixing pybind11 type_caster<> specializations for ::google::protobuf::Message
  // types in the same build target. This violates the ODR rule for
  // type_caster<::google::protobuf::Message> as well as other potential types, and can lead
  // to hard to diagnose bugs, crashes, and other mysterious bad behavior.

  // See https://github.com/pybind/pybind11/issues/2695 for more details.
}

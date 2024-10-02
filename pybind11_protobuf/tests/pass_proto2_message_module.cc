// Copyright (c) 2023 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include <cstddef>

#include "google/protobuf/message.h"
#include "pybind11_protobuf/native_proto_caster.h"

PYBIND11_MODULE(pass_proto2_message_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def("get_space_used_estimate",
        [](const ::google::protobuf::Message& msg) -> std::size_t {
          const ::google::protobuf::Reflection* refl = msg.GetReflection();
          return refl != nullptr ? refl->SpaceUsedLong(msg) : 0;
        });
}

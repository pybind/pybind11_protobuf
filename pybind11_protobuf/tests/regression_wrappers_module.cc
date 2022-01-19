// Copyright (c) 2022 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <pybind11/pybind11.h>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "pybind11_protobuf/native_proto_caster.h"

namespace {

PYBIND11_MODULE(regression_wrappers_module, m) {
  pybind11_protobuf::ImportNativeProtoCasters();

  m.def("print_descriptor_unique_ptr",
        [](std::unique_ptr<::google::protobuf::Message> message) -> std::string {
          return message->GetDescriptor()->DebugString();
        });
}

}  // namespace

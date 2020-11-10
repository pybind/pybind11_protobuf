// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef PYBIND11_PROTOBUF_PROTO_CASTERS_H_
#define PYBIND11_PROTOBUF_PROTO_CASTERS_H_

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>

#include <memory>
#include <stdexcept>
#include <type_traits>

#include "net/proto2/public/message.h"
#include "pybind11_protobuf/proto_utils.h"

namespace pybind11 {
namespace google {

// The value of PYBIND11_PROTOBUF_MODULE_PATH will be different depending on
// whether this is being used inside or outside of google3. The value used
// inside of google3 is defined here. Outside of google3, change this value by
// passing "-DPYBIND11_PROTOBUF_MODULE_PATH=..." on the commandline.
#ifndef PYBIND11_PROTOBUF_MODULE_PATH
#define PYBIND11_PROTOBUF_MODULE_PATH \
  google3.third_party.pybind11_protobuf.proto
#endif

// Imports the bindings for the proto base types. This not thread safe and
// should only be called from a PYBIND11_MODULE definition. If modifying this,
// see g3doc/pybind11_protobuf/README.md#importing-the-proto-module
inline module ImportProtoModule() {
#if PY_MAJOR_VERSION >= 3
  auto m = reinterpret_borrow<module>(
      PyImport_AddModule(PYBIND11_TOSTRING(PYBIND11_PROTOBUF_MODULE_PATH)));
  if (!IsProtoModuleImported()) RegisterProtoBindings(m);
  // else no-op because bindings are already loaded.
  return m;
#else
  return module::import(PYBIND11_TOSTRING(PYBIND11_PROTOBUF_MODULE_PATH));
#endif
}

// Registers the given concrete ProtoType with pybind11.
template <typename ProtoType>
void RegisterProtoMessageType(module m = module()) {
  CheckProtoModuleImported();
  // Drop the return value from ConcreteProtoMessageBindings.
  ConcreteProtoMessageBindings<ProtoType>(m);
}

}  // namespace google

// Specialize polymorphic_type_hook for proto message types.
// If ProtoType is a derived type (ie, not proto2::Message), this registers
// it and adds a constructor and concrete fields, to avoid the need to call
// FindFieldByName for every field access.
template <typename ProtoType>
struct polymorphic_type_hook<ProtoType,
                             std::enable_if_t<google::is_proto_v<ProtoType>>> {
  static const void *get(const ProtoType *src, const std::type_info *&type) {
    google::CheckProtoModuleImported();

    // Use RTTI to get the concrete message type.
    const void *out = polymorphic_type_hook_base<ProtoType>::get(src, type);
    if (!out) return nullptr;

    if (!detail::get_type_info(*type)) {
      // Concrete message type is not registered, so cast as a proto2::Message.
      out = static_cast<const proto2::Message *>(src);
      type = &typeid(proto2::Message);
    }

    return out;
  }
};

namespace detail {

// Pybind11 type_caster to enable automatic wrapping and/or converting
// protocol messages with pybind11.
template <typename ProtoType>
struct type_caster<ProtoType, std::enable_if_t<google::is_proto_v<ProtoType>>>
    : public type_caster_base<ProtoType> {
  using IntrinsicProtoType = intrinsic_t<ProtoType>;

 public:
  // Convert Python->C++.
  bool load(handle src, bool convert) {
    google::CheckProtoModuleImported();

    if (!google::PyProtoCheckType<IntrinsicProtoType>(src)) return false;

    if (google::IsWrappedCProto(src)) {  // Just remove the wrapper.
      // Concrete ProtoType may not be registered, so load as a proto2::Message.
      type_caster_base<proto2::Message> base_caster;
      if (!base_caster.load(src, convert))
        throw type_error(
            "Proto message passed type checks yet failed to be loaded as a "
            "proto2::Message base class. This should not be possible.");
      // Since we already checked the type, static cast is safe.
      type_caster_base<ProtoType>::value =
          static_cast<ProtoType *>(static_cast<proto2::Message *>(base_caster));
      return true;
    }

    // This is not a wrapped C proto and we are not allowed to do conversion.
    if (!convert) return false;

    // Convert this proto from a native python proto.
    owned_value_ =
        google::PyProtoAllocateAndCopyMessage<IntrinsicProtoType>(src);
    type_caster_base<ProtoType>::value = owned_value_.get();
    return true;
  }

 private:
  std::unique_ptr<proto2::Message> owned_value_;
};

// copybara:strip_begin(core pybind11 patch required)
// A specialization of move_only_holder_caster in proto_utils.h converts
// std::unique_ptrs to protobuffers into std::shared_ptrs automatically.
// copybara:strip_end

}  // namespace detail
}  // namespace pybind11

#endif  // PYBIND11_PROTOBUF_PROTO_CASTERS_H_

// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_
#define THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_

#include <pybind11/pybind11.h>

#include <memory>
#include <stdexcept>
#include <type_traits>

#include "net/proto2/public/message.h"
#include "pybind11_protobuf/proto_utils.h"

namespace pybind11 {

// Specialize polymorphic_type_hook for proto message types.
// If ProtoType is a derived type (ie, not proto2::Message), this registers
// it and adds a constructor and concrete fields, to avoid the need to call
// FindFieldByName for every field access.
template <typename ProtoType>
struct polymorphic_type_hook<ProtoType,
                             std::enable_if_t<google::is_proto_v<ProtoType>>> {
  static const void *get(const ProtoType *src, const std::type_info *&type) {
    // Use RTTI to get the concrete message type.
    const void *out = polymorphic_type_hook_base<ProtoType>::get(src, type);
    if (!out) return nullptr;

    // Register ProtoType if is a derived type and has not been registered yet.
    // This is a no-op if ProtoType == proto2::Message (not a derived type).
    if (!detail::get_type_info(*type))
      google::RegisterProtoMessageType<ProtoType>();

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
  // Conversion part 1 (Python->C++).
  bool load(handle src, bool convert) {
    if (!google::PyProtoCheckType<IntrinsicProtoType>(src)) return false;

    if (google::IsWrappedCProto(src))  // Just remove the wrapper.
      return type_caster_base<ProtoType>::load(src, convert);

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

}  // namespace detail
}  // namespace pybind11

#endif  // THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_

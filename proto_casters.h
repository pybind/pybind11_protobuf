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

// Specialize polymorphic_type_hook to cause all proto message types to be cast
// using the bindings for the base class (proto2::Message).
template <typename itype>
struct polymorphic_type_hook<itype,
                             std::enable_if_t<google::is_proto_v<itype>>> {
  static const void *get(const itype *src, const std::type_info *&type) {
    // Make sure the proto2::Message bindings are available.
    google::ImportProtoModule();
    // Use the type info corresponding to the proto2::Message base class.
    type = &typeid(proto2::Message);
    // Upcast the src pointer to the proto2::Message base class.
    return dynamic_cast<const void *>(
        static_cast<const proto2::Message *>(src));
  }
};

namespace detail {

struct generic_proto_caster : type_caster_base<proto2::Message> {
  // Elevate value to public visibility.
  using type_caster_base<proto2::Message>::value;
};

// Pybind11 type_caster to enable automatic wrapping and/or converting
// protocol messages with pybind11.
template <typename ProtoType>
struct type_caster<ProtoType, std::enable_if_t<google::is_proto_v<ProtoType>>>
    : public type_caster_base<ProtoType> {
  using IntrinsicProtoType = intrinsic_t<ProtoType>;

 public:
  // Conversion part 1 (Python->C++).
  bool load(handle src, bool convert) {
    if (!google::PyProtoCheckType<IntrinsicProtoType>(src))
      return false;

    if (google::IsWrappedCProto(src)) {
      // Just remove the wrapper.
      if (!generic_proto_caster_.load(src, convert)) return false;
      type_caster_base<ProtoType>::value = static_cast<IntrinsicProtoType *>(
          static_cast<proto2::Message *>(generic_proto_caster_.value));
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
  generic_proto_caster generic_proto_caster_;
  std::unique_ptr<proto2::Message> owned_value_;
};

}  // namespace detail
}  // namespace pybind11

#endif  // THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_

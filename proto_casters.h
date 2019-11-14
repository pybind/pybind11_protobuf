#ifndef THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_
#define THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_

#include <pybind11/pybind11.h>

#include <memory>
#include <stdexcept>
#include <type_traits>

#include "net/proto2/public/message.h"
#include "pybind11_protobuf/proto_utils.h"

namespace pybind11 {
namespace detail {

// Alias for checking whether a c++ type is a proto.
template <typename T>
using is_proto = std::is_base_of<proto2::Message, T>;

template <typename T>
T* GetPointer(T& val) {
  return &val;
}
template <typename T>
T* GetPointer(T&& val) {
  return &val;
}
template <typename T>
T* GetPointer(T* val) {
  return val;
}

// Pybind11 type_caster to enable automatic wrapping and/or converting
// protocol messages with pybind11.
template <typename ProtoType>
struct type_caster<
    ProtoType, typename std::enable_if<is_proto<ProtoType>::value, void>::type>
    : public type_caster_base<ProtoType> {
 public:
  // Conversion part 1 (Python->C++).
  bool load(handle src, bool convert) {
    if (google::IsWrappedCProto(src))
      // Just remove the wrapper.
      return type_caster_base<ProtoType>::load(src, convert);

    // This is not a wrapped C proto and we are not allowed to do conversion.
    if (!convert) return false;

    // Attempt to convert it from a native python proto.
    try {
      owned_value_ = google::PyProtoAllocateAndCopyMessage<ProtoType>(src);
    } catch (std::invalid_argument) {
      return false;
    }
    type_caster_base<ProtoType>::value = owned_value_.get();
    return true;
  }

  // Conversion part 2 (C++ -> Python)
  template <typename CType>
  static handle cast(CType src, return_value_policy policy, handle handle) {
    google::RegisterMessageType(GetPointer(src));
    return type_caster_base<ProtoType>::cast(src, policy, handle);
  }

 private:
  std::unique_ptr<proto2::Message> owned_value_;
};

}  // namespace detail
}  // namespace pybind11

#endif  // THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_CASTERS_H_

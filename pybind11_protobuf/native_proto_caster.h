#ifndef PYBIND11_PROTOBUF_NATIVE_PROTO_CASTERS_H_
#define PYBIND11_PROTOBUF_NATIVE_PROTO_CASTERS_H_

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/message.h"
#include "pybind11_protobuf/enum_type_caster.h"
#include "pybind11_protobuf/proto_caster_impl.h"

// NOTE: This directly currently contains 3 mutually incompatible
// implementations of pybind11::type_caster<> for ::google::protobuf::Message types due to
// C++ ODR violations.  Only one of the following is allowed:
//   * fast_cpp_proto_casters.h
//   * proto_casters.h
//   * native_proto_caster.h

// pybind11::type_caster<> specialization for ::google::protobuf::Message types that
// that convets protocol buffer objects between C++ and python representations.
// This binder supports binaries linked with both native python protos
// and fast cpp python protos.
//
// When passing a proto from python to C++, if possible, an underlying C++
// object is referenced. If not, the object is serialized and deserialized into
// a native C++ protocol buffer object.
//
// When passing a proto from C++ to python a copy to a python-native protocol
// buffer is made, either using a pure python proto or a fast cpp proto.
//
// Dynamically generated protos are only partially supported at present.
//
// To use native_proto_caster, include this file in the binding definition
// file.
//
// Example:
//
// #include <pybind11/pybind11.h>
// #include "pybind11_protobuf/native_proto_caster.h"
//
// MyMessage GetMessage() { ... }
// PYBIND11_MODULE(my_module, m) {
//  m.def("get_message", &GetMessage);
// }
//

namespace pybind11::detail {

// pybind11 type_caster<> specialization for c++ protocol buffer types using
// inheritance from google::proto_caster<>.
template <typename ProtoType>
struct type_caster<
    ProtoType, std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, ProtoType>>>
    : public google::proto_caster<ProtoType, google::native_cast_impl> {};

// NOTE: If smart_holders becomes the default we will need to change this to
//    type_caster<std::unique_ptr<ProtoType, D>, ...
// Until then using that form is ambiguous due to the existing specialization
// that does *not* forward a sfinae clause. Or we could add an sfinae clause
// to the existing specialization, but that's a *much* larger change.
// Anyway, the existing specializations fully forward to these.

// move_only_holder_caster enables using move-only holder types such as
// std::unique_ptr. It uses type_caster<Proto> to manage the conversion
// and construct a holder type.
template <typename ProtoType, typename HolderType>
struct move_only_holder_caster<
    ProtoType, HolderType,
    std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, ProtoType>>> {
 private:
  using Base = type_caster<intrinsic_t<ProtoType>>;
  static constexpr bool const_element =
      std::is_const_v<typename HolderType::element_type>;

 public:
  static constexpr auto name = Base::name;

  // C++->Python.
  static handle cast(HolderType &&src, return_value_policy, handle p) {
    auto *ptr = holder_helper<HolderType>::get(src);
    if (!ptr) return none().release();
    return Base::cast(std::move(*ptr), return_value_policy::move, p);
  }

  // Convert Python->C++.
  bool load(handle src, bool convert) {
    Base base;
    if (!base.load(src, convert)) {
      return false;
    }
    holder = base.as_unique_ptr();
    return true;
  }

  // PYBIND11_TYPE_CASTER
  explicit operator HolderType *() { return &holder; }
  explicit operator HolderType &() { return holder; }
  explicit operator HolderType &&() && { return std::move(holder); }

  template <typename T_>
  using cast_op_type = pybind11::detail::movable_cast_op_type<T_>;

 protected:
  HolderType holder;
};

// copyable_holder_caster enables using copyable holder types such as
// std::shared_ptr. It uses type_caster<Proto> to manage the conversion
// and construct a copy of the proto, then returns the shared_ptr.
//
// NOTE: When using pybind11 bindings, std::shared_ptr<Proto> is almost
// never correct, as it always makes a copy. It's mostly useful for handling
// methods that return a shared_ptr<const T>, which the caller never intends
// to mutate and where copy semantics will work just as well.
//
template <typename ProtoType, typename HolderType>
struct copyable_holder_caster<
    ProtoType, HolderType,
    std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, ProtoType>>> {
 private:
  using Base = type_caster<intrinsic_t<ProtoType>>;
  static constexpr bool const_element =
      std::is_const_v<typename HolderType::element_type>;

 public:
  static constexpr auto name = Base::name;

  // C++->Python.
  static handle cast(const HolderType &src, return_value_policy, handle p) {
    // The default path calls into cast_holder so that the holder/deleter
    // gets added to the proto. Here we just make a copy
    const auto *ptr = holder_helper<HolderType>::get(src);
    if (!ptr) return none().release();
    return Base::cast(*ptr, return_value_policy::copy, p);
  }

  // Convert Python->C++.
  bool load(handle src, bool convert) {
    Base base;
    if (!base.load(src, convert)) {
      return false;
    }
    // This always makes a copy, but it could, in some cases, grab a reference
    // and construct a shared_ptr, since the intention is clearly to mutate
    // the existing object...
    holder = base.as_unique_ptr();
    return true;
  }

  explicit operator ProtoType *() { return holder.get(); }
  explicit operator ProtoType &() { return *holder.get(); }
  explicit operator HolderType &() { return holder; }

  template <typename>
  using cast_op_type = HolderType &;

 protected:
  HolderType holder;
};

// NOTE: We also need to add support and/or test classes:
//
//  ::google::protobuf::Descriptor
//  ::google::protobuf::EnumDescriptor
//  ::google::protobuf::EnumValueDescriptor
//  ::google::protobuf::FieldDescriptor
//

}  // namespace pybind11::detail

#endif  // PYBIND11_PROTOBUF_FAST_CPP_PROTO_CASTERS_H_

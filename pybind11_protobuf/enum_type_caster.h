#ifndef PYBIND11_PROTOBUF_ENUM_TYPE_CASTER_H_
#define PYBIND11_PROTOBUF_ENUM_TYPE_CASTER_H_

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <string>
#include <type_traits>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"

// TODO(lar): Replace descriptor.proto.h with generated_enum_util.h

// pybind11 type_caster specialization which translates Proto::Enum types
// to/from ints. This will have ODR conflicts when users specify wrappers for
// enums using py::enum_<T>.
//
// ::google::protobuf::is_proto_enum and ::google::protobuf::GetEnumDescriptor are require
//
// NOTE: The protobuf compiler does not generate ::google::protobuf::is_proto_enum traits
// for enumerations of oneof fields.
//
// Example:
//  #include <pybind11/pybind11.h>
//  #include "pybind11_protobuf/proto_enum_casters.h"
//
//  MyMessage::EnumType GetMessageEnum() { ... }
//  PYBIND11_MODULE(my_module, m) {
//    m.def("get_message_enum", &GetMessageEnum);
//  }
namespace pybind11::google {

// Implementation details for pybind11 enum casting.
template <typename EnumType>
struct enum_type_caster {
 private:
  using T = std::underlying_type_t<EnumType>;
  using base_caster = detail::make_caster<T>;

 public:
  static constexpr auto name = pybind11::detail::_<EnumType>();

  // cast converts from C++ -> Python
  static handle cast(EnumType src, return_value_policy policy, handle p) {
    return base_caster::cast(static_cast<T>(src), policy, p);
  }

  // load converts from Python -> C++
  bool load(handle src, bool convert) {
    base_caster base;
    if (base.load(src, convert)) {
      T v = static_cast<T>(base);
      if (::google::protobuf::GetEnumDescriptor<EnumType>()->FindValueByNumber(v) ==
          nullptr) {
        throw value_error("Invalid value " + std::to_string(v) +
                          " for ::google::protobuf::Enum " + std::string(name.text));
      }
      value = static_cast<EnumType>(v);
      return true;
    }
    return false;
  }

  explicit operator EnumType() { return value; }

  template <typename>
  using cast_op_type = EnumType;

 private:
  EnumType value;
};

}  // namespace pybind11::google
namespace pybind11::detail {

// Specialization of pybind11::detail::type_caster<T> for types satisfying
// ::google::protobuf::is_proto_enum.
template <typename EnumType>
struct type_caster<EnumType,
                   std::enable_if_t<::google::protobuf::is_proto_enum<EnumType>::value>>
    : public google::enum_type_caster<EnumType> {};

}  // namespace pybind11::detail

#endif  // PYBIND11_PROTOBUF_ENUM_TYPE_CASTER_H_

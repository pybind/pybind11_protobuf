#ifndef PYBIND11_PROTOBUF_PROTO_CASTER_IMPL_H_
#define PYBIND11_PROTOBUF_PROTO_CASTER_IMPL_H_

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

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "pybind11_protobuf/proto_cast_util.h"

// Enables unsafe conversions; currently these are a work in progress.
#if !defined(PYBIND11_PROTOBUF_UNSAFE)
#define PYBIND11_PROTOBUF_UNSAFE 0
#endif

namespace pybind11::google {

// pybind11 constructs c++ references using the following mechanism, for
// example:
//
// type_caster<T> caster;
// caster.load(handle, /*convert=*/ false);
// call(pybind11::detail::cast_op<const T&>(caster));
//
template <typename ProtoType>
struct proto_caster_load_impl {
  static_assert(
      std::is_same<ProtoType, pybind11::detail::intrinsic_t<ProtoType>>::value,
      "");

  // load converts from Python -> C++
  bool load(handle src, bool convert) {
    // When given a none, treat it as a nullptr.
    if (src.is_none()) {
      value = nullptr;
      return true;
    }

    // Use the PyProto_API to get an underlying C++ message pointer from the
    // object, which returns non-null when the incoming proto message
    // is a fast_cpp_proto instance.
    if (const ::google::protobuf::Message *message =
            pybind11_protobuf::PyProtoGetCppMessagePointer(src);
        message != nullptr) {
      if (ProtoType::default_instance().GetReflection() !=
          message->GetReflection()) {
        // Reflection type mismatch; from a different pool?
        return false;
      }
      // NOTE: We might need to know whether the proto has extensions that
      // are python-only here.
      //
      // If the capability were available, then we could probe PyProto_API and
      // allow c++ mutability based on the python reference count.

      value = static_cast<const ProtoType *>(message);
      return true;
    }

    // The incoming object is not a fast_cpp_proto, so attempt to
    // serialize it and deserialize into a native C++ proto type.
    auto descriptor_name = pybind11_protobuf::PyProtoDescriptorName(src);
    if (!descriptor_name ||
        *descriptor_name != ProtoType::descriptor()->full_name()) {
      // type mismatch.
      return false;
    }
    // TODO(amauryfa): Only accept Python protos from descriptor_pool.Default().
    owned = std::unique_ptr<ProtoType>(new ProtoType());
    value = owned.get();
    return pybind11_protobuf::PyProtoCopyToCProto(src, owned.get());
  }

  // ensure_owned ensures that the owned member contains a copy of the
  // ::google::protobuf::Message.
  void ensure_owned() {
    if (value && !owned) {
      owned = std::unique_ptr<ProtoType>(value->New());
      *owned = *value;
      value = owned.get();
    }
  }

  const ProtoType *value;
  std::unique_ptr<ProtoType> owned;
};

template <>
struct proto_caster_load_impl<::google::protobuf::Message> {
  using ProtoType = ::google::protobuf::Message;

  bool load(handle src, bool convert) {
    if (src.is_none()) {
      value = nullptr;
      return true;
    }

    // Use the PyProto_API to get an underlying C++ message pointer from the
    // object, which returns non-null when the incoming proto message
    // is a fast_cpp_proto instance.
    if (value = pybind11_protobuf::PyProtoGetCppMessagePointer(src);
        value != nullptr) {
      return true;
    }

    // The incoming object is not a fast_cpp_proto, so find or create a native
    // C++ proto which has the same descriptors.
    auto descriptor_name = pybind11_protobuf::PyProtoDescriptorName(src);
    if (!descriptor_name) {
      return false;
    }
    owned.reset(static_cast<ProtoType *>(
        pybind11_protobuf::AllocateCProtoFromPythonSymbolDatabase(
            src, *descriptor_name)
            .release()));
    value = owned.get();
    return pybind11_protobuf::PyProtoCopyToCProto(src, owned.get());
  }

  // ensure_owned ensures that the owned member contains a copy of the
  // ::google::protobuf::Message.
  void ensure_owned() {
    if (value && !owned) {
      owned = std::unique_ptr<ProtoType>(value->New());
      owned->CopyFrom(*value);
      value = owned.get();
    }
  }

  const ::google::protobuf::Message *value;
  std::unique_ptr<::google::protobuf::Message> owned;
};

struct fast_cpp_cast_impl {
  inline static handle cast_impl(::google::protobuf::Message *src,
                                 return_value_policy policy, handle parent,
                                 bool is_const) {
    if (src == nullptr) return none().release();

    if (is_const && (policy == return_value_policy::reference ||
                     policy == return_value_policy::reference_internal)) {
      throw type_error(
          "Cannot return a const reference to a ::google::protobuf::Message derived "
          "type.  Consider setting return_value_policy::copy in the "
          "pybind11 def().");
    }

    return pybind11_protobuf::GenericFastCppProtoCast(src, policy, parent,
                                                      is_const);
  }
};

struct native_cast_impl {
  inline static handle cast_impl(::google::protobuf::Message *src,
                                 return_value_policy policy, handle parent,
                                 bool is_const) {
    if (src == nullptr) return none().release();

    // When using native casters, always copy the proto.
    return pybind11_protobuf::GenericProtoCast(src, return_value_policy::copy,
                                               parent, false);
  }
};

// pybind11 type_caster specialization for c++ protocol buffer types.
template <typename ProtoType, typename CastBase>
struct proto_caster : public proto_caster_load_impl<ProtoType>,
                      protected CastBase {
 private:
  using Loader = proto_caster_load_impl<ProtoType>;
  using CastBase::cast_impl;
  using Loader::ensure_owned;
  using Loader::owned;
  using Loader::value;

 public:
  static constexpr auto name = pybind11::detail::_<ProtoType>();

  // cast converts from C++ -> Python
  //
  // return_value_policy handling differs from the behavior for
  // py::class_-wrapped objects because because protocol buffer objects usually
  // need to be copied across the C++/python boundary as they contain internal
  // pointers which are unsafe to modify. See:
  // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies
  static handle cast(ProtoType &&src, return_value_policy policy,
                     handle parent) {
    return cast_impl(&src, return_value_policy::copy, parent, false);
  }

  static handle cast(const ProtoType *src, return_value_policy policy,
                     handle parent) {
    std::unique_ptr<const ProtoType> wrapper;
    if (policy == return_value_policy::take_ownership) {
      wrapper.reset(src);
      policy = return_value_policy::copy;
    } else if (policy == return_value_policy::automatic ||
               policy == return_value_policy::automatic_reference) {
      policy = return_value_policy::copy;
    }
    return cast_impl(const_cast<ProtoType *>(src), policy, parent, true);
  }

  static handle cast(ProtoType *src, return_value_policy policy,
                     handle parent) {
    std::unique_ptr<ProtoType> wrapper;
    if (policy == return_value_policy::take_ownership ||
        policy == return_value_policy::automatic) {
      wrapper.reset(src);
      policy = return_value_policy::copy;
    } else if (policy == return_value_policy::automatic_reference) {
      policy = return_value_policy::copy;
    }
    return cast_impl(src, policy, parent, false);
  }

  static handle cast(ProtoType const &src, return_value_policy policy,
                     handle parent) {
    if (policy == return_value_policy::automatic ||
        policy == return_value_policy::automatic_reference) {
      policy = return_value_policy::copy;
    }
    return cast_impl(const_cast<ProtoType *>(&src), return_value_policy::copy,
                     parent, true);
  }

  static handle cast(ProtoType &src, return_value_policy policy,
                     handle parent) {
    if (policy == return_value_policy::automatic ||
        policy == return_value_policy::automatic_reference) {
      policy = return_value_policy::copy;
    }
    return cast_impl(&src, policy, parent, false);
  }

  std::unique_ptr<ProtoType> as_unique_ptr() {
    ensure_owned();
    return std::move(owned);
  }

  // PYBIND11_TYPE_CASTER
  explicit operator const ProtoType *() { return value; }
  explicit operator const ProtoType &() {
    if (!value) throw reference_cast_error();
    return *value;
  }
  explicit operator ProtoType &&() && {
    if (!value) throw reference_cast_error();
    ensure_owned();
    return std::move(*owned);
  }

#if PYBIND11_PROTOBUF_UNSAFE
  // The following unsafe conversions are not enabled:
  explicit operator ProtoType *() { return const_cast<ProtoType *>(value); }
  explicit operator ProtoType &() {
    if (!value) throw reference_cast_error();
    return *const_cast<ProtoType *>(value);
  }
#endif

  // cast_op_type determines which operator overload to call for a given c++
  // input parameter type.
  // clang-format off
  template <typename T_>
  using cast_op_type =
      std::conditional_t<
          std::is_same_v<std::remove_reference_t<T_>, const ProtoType *>,
              const ProtoType *,
      std::conditional_t<
          std::is_same_v<std::remove_reference_t<T_>, ProtoType *>, ProtoType *,
      std::conditional_t<
          std::is_same_v<T_, const ProtoType &>, const ProtoType &,
      std::conditional_t<std::is_same_v<T_, ProtoType &>, ProtoType &,
      /*default is T&&*/ T_>>>>;
  // clang-format on
};

}  // namespace pybind11::google

#endif  // PYBIND11_PROTOBUF_PROTO_CASTER_IMPL_H_

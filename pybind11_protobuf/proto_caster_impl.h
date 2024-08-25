// IWYU pragma: private, include "pybind11_protobuf/native_proto_caster.h"

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

namespace pybind11_protobuf {

namespace internal {

template <
    typename To, typename Proto2Message = ::google::protobuf::Message,
    std::enable_if<std::is_polymorphic<Proto2Message>::value, int>::type = 0>
const To *dynamic_cast_proxy(const ::google::protobuf::Message *message) {
  return dynamic_cast<const To *>(message);
}

template <
    typename To, typename Proto2Message = ::google::protobuf::Message,
    std::enable_if<!std::is_polymorphic<Proto2Message>::value, int>::type = 0>
const To *dynamic_cast_proxy(const ::google::protobuf::Message *message) {
  return ::google::protobuf::DynamicCastToGenerated<To>(message);
}

}  // namespace internal

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
  bool load(pybind11::handle src, bool convert) {
    // When given a none, treat it as a nullptr.
    if (src.is_none()) {
      value = nullptr;
      return true;
    }
    // NOTE: We might need to know whether the proto has extensions that
    // are python-only.

    // Attempt to use the PyProto_API to get an underlying C++ message pointer
    // from the object.
    const ::google::protobuf::Message *message =
        pybind11_protobuf::PyProtoGetCppMessagePointer(src);
    if (message) {
      value = internal::dynamic_cast_proxy<ProtoType>(message);
      if (value) {
        // If the capability were available, then we could probe PyProto_API and
        // allow c++ mutability based on the python reference count.
        return true;
      }
    }

    if (!PyProtoHasMatchingFullName(src, ProtoType::GetDescriptor())) {
      return false;
    }
    pybind11::bytes serialized_bytes =
        PyProtoSerializePartialToString(src, convert);
    if (!serialized_bytes) {
      return false;
    }

    owned = std::unique_ptr<ProtoType>(new ProtoType());
    value = owned.get();
    return owned.get()->ParsePartialFromString(
        PyBytesAsStringView(serialized_bytes));
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

  bool load(pybind11::handle src, bool convert) {
    if (src.is_none()) {
      value = nullptr;
      return true;
    }

    // Attempt to use the PyProto_API to get an underlying C++ message pointer
    // from the object.
    value = pybind11_protobuf::PyProtoGetCppMessagePointer(src);
    if (value) {
      return true;
    }

    // `src` is not a C++ proto instance from the generated_pool,
    // so create a compatible native C++ proto.
    auto descriptor_name = pybind11_protobuf::PyProtoDescriptorFullName(src);
    if (!descriptor_name) {
      return false;
    }
    pybind11::bytes serialized_bytes =
        PyProtoSerializePartialToString(src, convert);
    if (!serialized_bytes) {
      return false;
    }

    owned.reset(static_cast<ProtoType *>(
        pybind11_protobuf::AllocateCProtoFromPythonSymbolDatabase(
            src, *descriptor_name)
            .release()));
    value = owned.get();
    return owned.get()->ParsePartialFromString(
        PyBytesAsStringView(serialized_bytes));
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
  inline static pybind11::handle cast_impl(::google::protobuf::Message *src,
                                           pybind11::return_value_policy policy,
                                           pybind11::handle parent,
                                           bool is_const) {
    if (src == nullptr) return pybind11::none().release();

#if PYBIND11_PROTOBUF_UNSAFE
    if (is_const &&
        (policy == pybind11::return_value_policy::reference ||
         policy == pybind11::return_value_policy::reference_internal)) {
      throw pybind11::type_error(
          "Cannot return a const reference to a ::google::protobuf::Message derived "
          "type.  Consider setting return_value_policy::copy in the "
          "pybind11 def().");
    }
#else
    // references are inherently unsafe, so convert them to copies.
    if (policy == pybind11::return_value_policy::reference ||
        policy == pybind11::return_value_policy::reference_internal) {
      policy = pybind11::return_value_policy::copy;
    }
#endif

    return pybind11_protobuf::GenericFastCppProtoCast(src, policy, parent,
                                                      is_const);
  }
};

struct native_cast_impl {
  inline static pybind11::handle cast_impl(::google::protobuf::Message *src,
                                           pybind11::return_value_policy policy,
                                           pybind11::handle parent,
                                           bool is_const) {
    if (src == nullptr) return pybind11::none().release();

    // references are inherently unsafe, so convert them to copies.
    if (policy == pybind11::return_value_policy::reference ||
        policy == pybind11::return_value_policy::reference_internal) {
      policy = pybind11::return_value_policy::copy;
    }

    return pybind11_protobuf::GenericProtoCast(src, policy, parent, false);
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
  static constexpr auto name = pybind11::detail::const_name<ProtoType>();

  // cast converts from C++ -> Python
  //
  // return_value_policy handling differs from the behavior for
  // py::class_-wrapped objects because because protocol buffer objects usually
  // need to be copied across the C++/python boundary as they contain internal
  // pointers which are unsafe to modify. See:
  // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies
  static pybind11::handle cast(ProtoType &&src,
                               pybind11::return_value_policy policy,
                               pybind11::handle parent) {
    return cast_impl(&src, pybind11::return_value_policy::move, parent, false);
  }

  static pybind11::handle cast(const ProtoType *src,
                               pybind11::return_value_policy policy,
                               pybind11::handle parent) {
    std::unique_ptr<const ProtoType> wrapper;
    if (policy == pybind11::return_value_policy::automatic ||
        policy == pybind11::return_value_policy::automatic_reference) {
      policy = pybind11::return_value_policy::copy;
    } else if (policy == pybind11::return_value_policy::take_ownership) {
      wrapper.reset(src);
    }
    return cast_impl(const_cast<ProtoType *>(src), policy, parent, true);
  }

  static pybind11::handle cast(ProtoType *src,
                               pybind11::return_value_policy policy,
                               pybind11::handle parent) {
    std::unique_ptr<ProtoType> wrapper;
    if (policy == pybind11::return_value_policy::automatic_reference) {
      policy = pybind11::return_value_policy::copy;
    } else if (policy == pybind11::return_value_policy::automatic ||
               policy == pybind11::return_value_policy::take_ownership) {
      policy = pybind11::return_value_policy::take_ownership;
      wrapper.reset(src);
    }
    return cast_impl(src, policy, parent, false);
  }

  static pybind11::handle cast(ProtoType const &src,
                               pybind11::return_value_policy policy,
                               pybind11::handle parent) {
    if (policy == pybind11::return_value_policy::automatic ||
        policy == pybind11::return_value_policy::automatic_reference) {
      policy = pybind11::return_value_policy::copy;
    }
    return cast_impl(const_cast<ProtoType *>(&src), policy, parent, true);
  }

  static pybind11::handle cast(ProtoType &src,
                               pybind11::return_value_policy policy,
                               pybind11::handle parent) {
    if (policy == pybind11::return_value_policy::automatic ||
        policy == pybind11::return_value_policy::automatic_reference) {
      policy = pybind11::return_value_policy::copy;
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
    if (!value) throw pybind11::reference_cast_error();
    return *value;
  }
  explicit operator ProtoType &&() && {
    if (!value) throw pybind11::reference_cast_error();
    ensure_owned();
    return std::move(*owned);
  }
  explicit operator const ProtoType &&() {
    if (!value) throw pybind11::reference_cast_error();
    ensure_owned();
    return std::move(*owned);
  }
#if PYBIND11_PROTOBUF_UNSAFE
  // The following unsafe conversions are not enabled:
  explicit operator ProtoType *() { return const_cast<ProtoType *>(value); }
  explicit operator ProtoType &() {
    if (!value) throw pybind11::reference_cast_error();
    return *const_cast<ProtoType *>(value);
  }
#endif

  // cast_op_type determines which operator overload to call for a given c++
  // input parameter type.
  // clang-format off
  template <typename T_>
  using cast_op_type =
      std::conditional_t<
          std::is_same<std::remove_reference_t<T_>, const ProtoType *>::value,
              const ProtoType *,
      std::conditional_t<
          std::is_same<std::remove_reference_t<T_>, ProtoType *>::value,
              ProtoType *,
      std::conditional_t<
          std::is_same<T_, const ProtoType &>::value,
              const ProtoType &,
      std::conditional_t<
          std::is_same<T_, ProtoType &>::value,
              ProtoType &,
      T_>>>>;  // Fall back to `T_` (assumed `T&&`).
  // clang-format on
};

// move_only_holder_caster enables using move-only holder types such as
// std::unique_ptr. It uses type_caster<Proto> to manage the conversion
// and construct a holder type.
template <typename MessageType, typename HolderType>
struct move_only_holder_caster_impl {
 private:
  using Base =
      pybind11::detail::type_caster<pybind11::detail::intrinsic_t<MessageType>>;
  static constexpr bool const_element =
      std::is_const<typename HolderType::element_type>::value;

 public:
  static constexpr auto name = Base::name;

  // C++->Python.
  static pybind11::handle cast(HolderType &&src, pybind11::return_value_policy,
                               pybind11::handle p) {
    auto *ptr = pybind11::detail::holder_helper<HolderType>::get(src);
    if (!ptr) return pybind11::none().release();
    return Base::cast(std::move(*ptr), pybind11::return_value_policy::move, p);
  }

  // Convert Python->C++.
  bool load(pybind11::handle src, bool convert) {
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

// copyable_holder_caster enables using copyable holder types such
// as std::shared_ptr. It uses type_caster<Proto> to manage the
// conversion and construct a copy of the proto, then returns the
// shared_ptr.
//
// NOTE: When using pybind11 bindings, std::shared_ptr<Proto> is
// almost never correct, as it always makes a copy. It's mostly
// useful for handling methods that return a shared_ptr<const T>,
// which the caller never intends to mutate and where copy semantics
// will work just as well.
//
template <typename MessageType, typename HolderType>
struct copyable_holder_caster_impl {
 private:
  using Base =
      pybind11::detail::type_caster<pybind11::detail::intrinsic_t<MessageType>>;
  static constexpr bool const_element =
      std::is_const<typename HolderType::element_type>::value;

 public:
  static constexpr auto name = Base::name;

  // C++->Python.
  static pybind11::handle cast(const HolderType &src,
                               pybind11::return_value_policy,
                               pybind11::handle p) {
    // The default path calls into cast_holder so that the
    // holder/deleter gets added to the proto. Here we just make a
    // copy
    const auto *ptr = pybind11::detail::holder_helper<HolderType>::get(src);
    if (!ptr) return pybind11::none().release();
    return Base::cast(*ptr, pybind11::return_value_policy::copy, p);
  }

  // Convert Python->C++.
  bool load(pybind11::handle src, bool convert) {
    Base base;
    if (!base.load(src, convert)) {
      return false;
    }
    // This always makes a copy, but it could, in some cases, grab a
    // reference and construct a shared_ptr, since the intention is
    // clearly to mutate the existing object...
    holder = base.as_unique_ptr();
    return true;
  }

  explicit operator HolderType &() { return holder; }

  template <typename>
  using cast_op_type = HolderType &;

 protected:
  HolderType holder;
};
}  // namespace pybind11_protobuf

#endif  // PYBIND11_PROTOBUF_PROTO_CASTER_IMPL_H_

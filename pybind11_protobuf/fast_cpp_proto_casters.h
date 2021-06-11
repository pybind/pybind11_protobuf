#ifndef PYBIND11_PROTOBUF_FAST_CPP_PROTO_CASTERS_H_
#define PYBIND11_PROTOBUF_FAST_CPP_PROTO_CASTERS_H_

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

// pybind11 type_caster that works in conjunction with python
// use_fast_cpp_protos to convert protocol buffers between C++ and python.
// (Users must link: //net/proto2/python/public:use_fast_cpp_protos)
//
// Supports by value, by const reference and unique_ptr types.
// TODO: Enable std::shared_ptr of proto types.
//
// Casting to `const ProtoType&` works without copying in most cases,
// no submessage references should be held across language callsites,
// since those submessage references may become invalid.
//
// When returning a ::google::protobuf::Message (or derived class), python sees a
// concrete type based on the message descriptor.
//
//
// To use fast_cpp_proto_casters, include this file in the binding definition
// file and link the appropriate build dependency.
//
// Example:
//
// #include <pybind11/pybind11.h>
// #include "pybind11_protobuf/fast_cpp_proto_casters.h"
//
// MyMessage GetMessage() { ... }
// PYBIND11_MODULE(my_module, m) {
//  m.def("get_message", &GetMessage);
// }

// WARNING   WARNING   WARNING   WARNING   WARNING   WARNING  WARNING
//
// This is still a work in progress.
//
// Sharing the same C++ protocol buffers with python is dangerous. They are
// currently permitted when return_value_policy::reference is used in a def(),
// def_property(), and similar pybind11 constructs. Such usage may lead multiple
// python objects pointing to the same C++ object (there is ongoing work to
// address this), conflicting mutations from python and C++, C++ code deleting
// an in-use python object, other potentially unsafe practices.

// NOTE: This is is incompatible with proto_casters.h in the same directory.
#if defined(PYBIND11_PROTOBUF_PROTO_CASTERS_H_)
#error "fast_cpp_proto_casters.h and proto_casters.h conflict."
#endif

// Enables unsafe conversions; currently these are a work in progress.
#if !defined(PYBIND11_PROTOBUF_UNSAFE)
#define PYBIND11_PROTOBUF_UNSAFE 0
#endif

namespace pybind11_protobuf::fast_cpp_protos {

// Alias for checking whether a c++ type is a proto.
template <typename T>
constexpr auto is_proto_v = std::is_base_of_v<::google::protobuf::Message, T>;

// Returns the type name of the python object as would be returned
// by `repr(type(obj))`
std::string PyObjectTypeString(PyObject *obj);

// Returns a ::google::protobuf::Message* from a cpp_fast_proto.
const ::google::protobuf::Message *GetCppProtoMessagePointer(const pybind11::handle &src);

struct cpp_to_py_impl {
 public:
  // allocates a fast cpp proto python object, also returning
  // the embedded c++ proto2 message type. The returned message
  // pointer cannot be null.
  static std::pair<pybind11::object, ::google::protobuf::Message *> allocate(
      const ::google::protobuf::Descriptor *src_descriptor);

  // reference returns a python object that references an existing c++ proto
  // message. This is potentially unsafe.
  static pybind11::object reference(::google::protobuf::Message *src);

  // make allocates an returns a new python proto object which has the contents
  // of the passed in message.
  template <typename ProtoType,
            std::enable_if_t<(!std::is_same_v<ProtoType, ::google::protobuf::Message>)> * =
                nullptr>
  static pybind11::object make(ProtoType &&src) {
    using P = pybind11::detail::intrinsic_t<ProtoType>;
    auto [result, result_message] =
        cpp_to_py_impl::allocate(src.GetDescriptor());
    if (auto *cast = ::google::protobuf::DynamicCastToGenerated<P>(result_message);
        cast != nullptr) {
      *cast = std::forward<ProtoType>(src);
    } else {
      result_message->CopyFrom(src);
    }
    return result;
  }
  static pybind11::object make(const ::google::protobuf::Message &src) {
    // We can't dynamic-cast to a concrete type, so no moving allowed.
    auto [result, result_message] =
        cpp_to_py_impl::allocate(src.GetDescriptor());
    result_message->CopyFrom(src);
    return result;
  }
};

}  // namespace pybind11_protobuf::fast_cpp_protos

namespace pybind11::detail {

// pybind11 type_caster specialization for c++ protocol buffer types.
template <typename ProtoType>
struct type_caster<
    ProtoType, std::enable_if_t<
                   pybind11_protobuf::fast_cpp_protos::is_proto_v<ProtoType>>> {
 private:
  using cpp_to_py_impl = ::pybind11_protobuf::fast_cpp_protos::cpp_to_py_impl;
  static_assert(std::is_same<ProtoType, intrinsic_t<ProtoType>>::value, "");

  // returns whether the return_value_policy should create a reference.
  // By default, only creates references when explicitly requested.
  static bool is_reference_policy(return_value_policy policy) {
    if (policy == return_value_policy::reference_internal) {
      throw type_error(
          "Policy reference_internal not allowed for protocol buffer type " +
          std::string(name.text) + ". (Is this a def_property field?");
    }

#if PYBIND11_PROTOBUF_UNSAFE
    return policy == return_value_policy::reference ||
           policy == return_value_policy::automatic_reference;
#else
    return policy == return_value_policy::reference;
#endif
  }

 public:
  static constexpr auto name = _<ProtoType>();

  // cast converts from C++ -> Python
  static handle cast(const ProtoType *src, return_value_policy policy,
                     handle parent) {
    if (!src) return none().release();
    if (is_reference_policy(policy)) {
      // There are situations where we'd like to return a single const
      // object to Python (immutable_reference).
      // TODO(b/174120876): Follow up and enable immutable references.
      throw type_error("Cannot return an immutable pointer to type " +
                       std::string(name.text) +
                       ". Consider setting return_value_policy::copy in the "
                       "pybind11 def().");
    }
    return cpp_to_py_impl::make(*src).release();
  }

  static handle cast(ProtoType *src, return_value_policy policy,
                     handle parent) {
    if (!src) return none().release();
    if (policy == return_value_policy::take_ownership) {
      std::unique_ptr<ProtoType> wrapper(src);
      return cpp_to_py_impl::make(std::move(*src)).release();
    } else if (is_reference_policy(policy)) {
      return cpp_to_py_impl::reference(src).release();
    }
    return cpp_to_py_impl::make(static_cast<const ProtoType &>(*src)).release();
  }

  static handle cast(const ProtoType &src, return_value_policy policy, handle) {
    if (is_reference_policy(policy)) {
      // There are situations where we'd like to return a single const
      // object to Python (immutable_reference).
      // TODO(b/174120876): Follow up and enable immutable references.
      throw type_error("Cannot return an immutable reference to type " +
                       std::string(name.text) +
                       ". Consider setting return_value_policy::copy in the "
                       "pybind11 def().");
    }
    return cpp_to_py_impl::make(src).release();
  }

  static handle cast(ProtoType &src, return_value_policy policy, handle) {
    if (is_reference_policy(policy)) {
      return cpp_to_py_impl::reference(&src).release();
    }
    return cpp_to_py_impl::make(static_cast<const ProtoType &>(src)).release();
  }

  static handle cast(ProtoType &&src, return_value_policy policy, handle) {
    return cpp_to_py_impl::make(std::move(src)).release();
  }

  // load converts from Python -> C++
  bool load(handle src, bool convert) {
    // pybind11 constructs c++ references using the following mechanism, for
    // example:
    //
    // type_caster<T> caster;
    // caster.load(handle, /*convert=*/ false);
    // call(pybind11::detail::cast_op<const T&>(caster));
    //

    // When given a none, treat it as a nullptr.
    if (src.is_none()) {
      value = nullptr;
      return true;
    }

    // Try to extract the C++ ::google::protobuf::Message using they python API.
    if (try_load_as_fast_cpp_proto(
            src, std::is_same<::google::protobuf::Message, ProtoType>{})) {
      return true;
    }

    // This was not a fast_cpp_proto message, perhaps it is a pybind messge?
    // TODO(lar): Add try_load_as_pybind_message()

    if (convert) {
      // TODO(lar): The incoming object is not a fast_cpp proto, so attempt to
      // serialize it and deserialize it as a native C++ proto type.
    }

    // This is not a supported message type.
    // TODO: Remove the PyErr_Clear.
    PyErr_Clear();
    return false;
  }

  // steal_copy returns a copy of the object suitable
  // for our specialization of move_only_holder_caster.
  std::unique_ptr<ProtoType> steal_copy() {
    if (!value) return nullptr;
    if (!owned) {
      owned = std::unique_ptr<ProtoType>(value->New());
      owned->CopyFrom(*value);
    }
    return std::move(owned);
  }

  // PYBIND11_TYPE_CASTER
  operator const ProtoType *() { return value; }
  operator const ProtoType &() {
    if (!value) throw reference_cast_error();
    return *value;
  }
  operator ProtoType &&() && {
    if (!value) throw reference_cast_error();
    if (!owned) {
      owned.reset(value->New());
      owned->CopyFrom(*value);
    }
    return std::move(*owned);
  }
#if PYBIND11_PROTOBUF_UNSAFE
  // The following unsafe conversions are not enabled:
  operator ProtoType *() { return const_cast<ProtoType *>(value); }
  operator ProtoType &() {
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

  // try_load_as_fast_cpp_proto attempts to reference a C++ native protocol
  // buffer that is generated via python bindings.
  bool try_load_as_fast_cpp_proto(handle src,
                                  /*::google::protobuf::Message*/ std::false_type) {
    const ::google::protobuf::Message *message =
        pybind11_protobuf::fast_cpp_protos::GetCppProtoMessagePointer(src);
    if (!message) {
      return false;
    }

    // This is not a ::google::protobuf::Message, so validate the incoming proto type
    // via descriptor comparisons..
    if (ProtoType::descriptor() != message->GetDescriptor() &&
        ProtoType::descriptor()->full_name() !=
            message->GetDescriptor()->full_name()) {
      // passed proto type does not match.
      return false;
    }

    // If the capability were available, then we could probe PyProto_API and
    // allow c++ mutability based on the python reference count.

    // Use our const reference, even though we don't know whether
    // the callee is const or not...
    value = ::google::protobuf::DynamicCastToGenerated<ProtoType>(
        const_cast<::google::protobuf::Message *>(message));
    if (value != nullptr) {
      return true;
    }

    // Mutable case, however there are submessages referenced.
    // Consider the jbms@ proposed optimization to copy all the
    // referenced submessages, then detach the existing references.

    // Just make a copy. This could be generic serialization/deserialization,
    // but we already use fast python protos, so it should work.
    owned = std::unique_ptr<ProtoType>(new ProtoType());
    owned->CopyFrom(*message);
    value = owned.get();
    return true;
  }

  bool try_load_as_fast_cpp_proto(handle src,
                                  /*::google::protobuf::Message*/ std::true_type) {
    // Pass a reference to reference the internal pointer to C++.
    value = pybind11_protobuf::fast_cpp_protos::GetCppProtoMessagePointer(src);
    return value != nullptr;
  }

 protected:
  const ProtoType *value;
  std::unique_ptr<ProtoType> owned;
};

// move_only_holder_caster enables using move-only holder types such as
// std::unique_ptr. It uses type_caster<Proto> to manage the conversion
// and construct a holder type.
template <typename ProtoType, typename HolderType>
struct move_only_holder_caster<
    ProtoType, HolderType,
    std::enable_if_t<
        pybind11_protobuf::fast_cpp_protos::is_proto_v<ProtoType>>> {
 private:
  using Base = type_caster<ProtoType>;

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
    std::unique_ptr<ProtoType> stolen = base.steal_copy();
    holder = HolderType(stolen.release());
    return true;
  }

  // PYBIND11_TYPE_CASTER
  operator HolderType *() { return &holder; }
  operator HolderType &() { return holder; }
  operator HolderType &&() && { return std::move(holder); }

  template <typename T_>
  using cast_op_type = pybind11::detail::movable_cast_op_type<T_>;

 protected:
  HolderType holder;
};

}  // namespace pybind11::detail

#endif  // PYBIND11_PROTOBUF_FAST_CPP_PROTO_CASTERS_H_

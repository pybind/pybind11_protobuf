#ifndef PYBIND11_PROTOBUF_WRAPPED_PROTO_CASTER_H_
#define PYBIND11_PROTOBUF_WRAPPED_PROTO_CASTER_H_

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
#include "absl/status/statusor.h"
#include "pybind11_protobuf/proto_cast_util.h"
#include "pybind11_protobuf/proto_caster_impl.h"

// pybind11::type_caster<> specialization for ::google::protobuf::Message types using
// pybind11::google::WrappedProto<> wrappers. When passing from C++ to
// python a copy is always made. When passing from python to C++ a copy is
// made for mutable and by-value types, but not for const reference types.
//
// This is intended as a migration aid until such a time as proto_casters.h
// can be replaced with native_proto_caster.h.
//
// Example:
//
// #include <pybind11/pybind11.h>
// #include "pybind11_protobuf/wrapped_proto_casters.h"
//
// MyMessage GetMessage() { ... }
//
// PYBIND11_MODULE(my_module, m) {
//   using ::pybind11::google::WithWrappedProtos;
//   m.def("get_message", WithWrappedProtos(&GetMessage));
// }
//

namespace pybind11::google {

/// Tag types for WrappedProto specialization.
enum WrappedProtoKind : int { kValue, kConst, kMutable };

/// WrappedProto<T, kind> wraps a ::google::protobuf::Message subtype, exposing implicit
/// constructors and implicit cast operators. The ConstTag variant allows
/// conversion to const; the MutableTag allows mutable conversion and enables
/// the type-caster specialization to enforce that a copy is made.
template <typename ProtoType, WrappedProtoKind Kind>
struct WrappedProto;

template <typename ProtoType>
struct WrappedProto<ProtoType, WrappedProtoKind::kMutable> {
  using type = ProtoType;
  static constexpr WrappedProtoKind kind = WrappedProtoKind::kMutable;
  ProtoType* proto;

  WrappedProto(ProtoType* p) : proto(p) {}
  WrappedProto(ProtoType& p) : proto(&p) {}
  ProtoType* get() noexcept { return proto; }

  operator ProtoType*() noexcept { return proto; }
  operator ProtoType&() noexcept {
    if (!proto) throw reference_cast_error();
    return *proto;
  }
};

template <typename ProtoType>
struct WrappedProto<ProtoType, WrappedProtoKind::kConst> {
  using type = ProtoType;
  static constexpr WrappedProtoKind kind = WrappedProtoKind::kConst;
  const ProtoType* proto;

  WrappedProto(const ProtoType* p) : proto(p) {}
  WrappedProto(const ProtoType& p) : proto(&p) {}
  ProtoType* get() noexcept { return const_cast<ProtoType*>(proto); }

  operator const ProtoType*() const noexcept { return proto; }
  operator const ProtoType&() const noexcept {
    if (!proto) throw reference_cast_error();
    return *proto;
  }
};

template <typename ProtoType>
struct WrappedProto<ProtoType, WrappedProtoKind::kValue> {
  using type = ProtoType;
  static constexpr WrappedProtoKind kind = WrappedProtoKind::kValue;
  ProtoType proto;

  WrappedProto(ProtoType&& p) : proto(std::move(p)) {}
  ProtoType* get() noexcept { return &proto; }

  operator ProtoType&&() && noexcept { return std::move(proto); }
};

// Implementation details for WithWrappedProtos
namespace impl {

template <typename T>
using intrinsic_t =
    typename pybind11::detail::intrinsic_t<std::remove_reference_t<T>>;

template <typename T>
constexpr WrappedProtoKind DetectKind() {
  using no_ref_t = std::remove_reference_t<T>;
  using base_t = intrinsic_t<no_ref_t>;

  if constexpr (std::is_same_v<no_ref_t, const base_t*>) {
    return WrappedProtoKind::kConst;
  } else if constexpr (std::is_same_v<no_ref_t, base_t*>) {
    return WrappedProtoKind::kMutable;
  } else if constexpr (std::is_same_v<T, const base_t&>) {
    return WrappedProtoKind::kConst;
  } else if constexpr (std::is_same_v<T, base_t&>) {
    return WrappedProtoKind::kMutable;
  } else {
    return WrappedProtoKind::kValue;
  }
}

template <typename T, typename SFINAE = void>
struct WrapHelper {
  using type = T;
};

template <typename ProtoType>
struct WrapHelper<ProtoType,  //
                  std::enable_if_t<std::is_base_of_v<::google::protobuf::Message,
                                                     intrinsic_t<ProtoType>>>> {
  using proto = intrinsic_t<ProtoType>;
  using type = WrappedProto<proto, DetectKind<ProtoType>()>;
};

// absl::StatusOr is a common wrapper so also propagate WrappedProto into
// StatusOr types.  When wrapped by WithWrappedProtos, ensure that
// pybind11_abseil/status_casters.h is included and follow the instructions
// there.
template <typename ProtoType>
struct WrapHelper<absl::StatusOr<ProtoType>,  //
                  std::enable_if_t<std::is_base_of_v<::google::protobuf::Message,
                                                     intrinsic_t<ProtoType>>>> {
  using proto = intrinsic_t<ProtoType>;
  using type = absl::StatusOr<WrappedProto<proto, DetectKind<ProtoType>()>>;
};

/// WrappedProtoInvoker is an internal function object that exposes a call
/// with an equivalent signature as F, replacing ::google::protobuf::Message derived types
/// with WrappedProto<T, ...> types.
template <typename F, typename>
struct WrappedInvoker;

template <typename F, typename R, typename... Args>
struct WrappedInvoker<F, std::function<R(Args...)>> {
  F f;
  typename WrapHelper<R>::type operator()(
      typename WrapHelper<Args>::type... args) const {
    return f(std::forward<decltype(args)>(args)...);
  }
};

template <typename C, typename R, typename... Args>
struct ConstMemberInvoker {
  R (C::*f)(Args...) const;
  typename WrapHelper<R>::type operator()(
      const C& c, typename WrapHelper<Args>::type... args) const {
    return std::invoke(f, &c, std::forward<decltype(args)>(args)...);
  }
};

template <typename C, typename R, typename... Args>
struct MemberInvoker {
  R (C::*f)(Args...);
  typename WrapHelper<R>::type operator()(
      C& c, typename WrapHelper<Args>::type... args) const {
    return std::invoke(f, &c, std::forward<decltype(args)>(args)...);
  }
};

}  // namespace impl

/// WithWrappedProtos(...) wraps a function type and converts any
/// ::google::protobuf::Message derived types into a WrappedProto<...> of the same
/// underlying proto2 type.
template <typename F>
auto WithWrappedProtos(F f)
    -> impl::WrappedInvoker<F, decltype(std::function(f))> {
  return {std::move(f)};
}

template <typename R, typename C, typename... Args>
auto WithWrappedProtos(R (C::*f)(Args...)) {
  return impl::MemberInvoker<C, R, Args...>{f};
}

template <typename R, typename C, typename... Args>
auto WithWrappedProtos(R (C::*f)(Args...) const) {
  return impl::ConstMemberInvoker<C, R, Args...>{f};
}

// pybind11 type_caster specialization for pybind11::google::WrappedProto<T>
// c++ protocol buffer types.
template <typename WrappedProtoType>
struct wrapped_proto_caster
    : public proto_caster_load_impl<typename WrappedProtoType::type>,
      protected native_cast_impl {
  using Base = proto_caster_load_impl<typename WrappedProtoType::type>;
  using ProtoType = typename WrappedProtoType::type;
  using Base::ensure_owned;
  using Base::owned;
  using Base::value;
  using native_cast_impl::cast_impl;

 public:
  static constexpr auto name = pybind11::detail::_<WrappedProtoType>();

  // cast converts from C++ -> Python
  static handle cast(WrappedProtoType src, return_value_policy policy,
                     handle parent) {
    if (src.get() == nullptr) return none().release();
    std::unique_ptr<ProtoType> owned;
    if (policy == return_value_policy::take_ownership) {
      owned.reset(const_cast<ProtoType*>(src.get()));
    }
    return cast_impl(src.get(), return_value_policy::copy, parent,
                     WrappedProtoKind::kConst == WrappedProtoType::kind);
  }

  // PYBIND11_TYPE_CASTER
  explicit operator WrappedProtoType() {
    if constexpr (WrappedProtoKind::kValue == WrappedProtoType::kind) {
      if (owned) {
        return std::move(*owned);
      } else {
        return ProtoType(*value);
      }
    } else if constexpr (WrappedProtoKind::kMutable == WrappedProtoType::kind) {
      ensure_owned();
      return owned.get();
    } else {
      return value;
    }
  }

  template <typename T_>
  using cast_op_type = WrappedProtoType;
};

}  // namespace pybind11::google
namespace pybind11::detail {

// pybind11 type_caster<> specialization for c++ protocol buffer types using
// inheritance from google::wrapped_proto_caster<>.
template <typename ProtoType, ::pybind11::google::WrappedProtoKind Kind>
struct type_caster<
    pybind11::google::WrappedProto<ProtoType, Kind>,
    std::enable_if_t<std::is_base_of_v<::google::protobuf::Message, ProtoType>>>
    : public google::wrapped_proto_caster<
          pybind11::google::WrappedProto<ProtoType, Kind>> {};

}  // namespace pybind11::detail

#endif  // PYBIND11_PROTOBUF_WRAPPED_PROTO_CASTER_H_

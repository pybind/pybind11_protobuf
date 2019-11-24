// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_UTILS_H_
#define THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_UTILS_H_

#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <cstddef>
#include <stdexcept>
#include <string>

#include "google/protobuf/any.proto.h"
#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"
#include "net/proto2/public/reflection.h"
#include "absl/memory/memory.h"

namespace pybind11 {
namespace google {

// Use a macro so this path can be set from the commandline when building.
#ifndef PYBIND11_PROTOBUF_IMPORT_PATH
#define PYBIND11_PROTOBUF_IMPORT_PATH google3.third_party.pybind11_protobuf
#endif

constexpr char kGoogle3UtilsProtoModule[] =
    PYBIND11_TOSTRING(PYBIND11_PROTOBUF_IMPORT_PATH) ".proto";

// Imports the proto module.
inline void ImportProtoModule() { module::import(kGoogle3UtilsProtoModule); }

// Name of the property which indicates whether a proto is a wrapped or native.
constexpr char kIsWrappedCProtoAttr[] = "_is_wrapped_c_proto";

// Returns true if the given python object is a wrapped C proto.
inline bool IsWrappedCProto(handle handle) {
  return hasattr(handle, kIsWrappedCProtoAttr);
}

// Returns the full name of the given (native or wrapped) python proto.
std::string PyProtoFullName(handle py_proto);

// Throws an invalid argument error if the given py_proto is not a proto or
// does not match the expected name.
void PyProtoCheckType(handle py_proto, const std::string& expected_type);

// Returns the serialized version of the given (native or wrapped) python proto.
std::string PyProtoSerializeToString(handle py_proto);

// Check that the type of py_proto matches the template argument, then
// allocate and return a proto of that type.
template <typename ProtoType>
std::unique_ptr<ProtoType> PyProtoAllocateMessage(handle py_proto) {
  PyProtoCheckType(py_proto, ProtoType::descriptor()->full_name());
  return absl::make_unique<ProtoType>();
}

// Specialization for the case that the template argument is a generic message.
// The type is pulled from the py_proto.
template <>
std::unique_ptr<proto2::Message> PyProtoAllocateMessage(handle py_proto);

// Allocate a C++ proto of the same type as py_proto and copy the contents
// from py_proto. This works whether py_proto is a native or wrapped proto.
// If expected_type is given and the type in py_proto does not match it, an
// invalid argument error will be thrown.
template <typename ProtoType>
std::unique_ptr<ProtoType> PyProtoAllocateAndCopyMessage(handle py_proto) {
  auto new_msg = PyProtoAllocateMessage<ProtoType>(py_proto);
  if (!new_msg->ParseFromString(google::PyProtoSerializeToString(py_proto)))
    throw std::runtime_error("Error copying message.");
  return new_msg;
}

// Pack an any proto from a proto, regardless of whether it is a native python
// or wrapped c proto. Using the converter on a native python proto would
// require serializing-deserializing-serializing again, while this always
// requires only 1 serialization operation.
void AnyPackFromPyProto(handle py_proto, ::google::protobuf::Any* any_proto);

// A type used with DispatchFieldDescriptor to represent a generic enum value.
struct GenericEnum;

// Call Handler<Type>::HandleField(field_desc, ...) with the Type that
// corresponds to the value of the cpp_type enum in the field descriptor.
// Returns the result of that function. The return type of that function
// cannot depend on the Type template parameter (ie, all instantiations of
// Handler must return the same type).
//
// This works by instantiating a template class for each possible type of proto
// field. That template class is a template parameter (look up 'template
// template class'). Ideally this would be a template function, but template
// template functions don't exist in C++, so you must define a class with a
// static member function (HandleField) which will be called.
template <template <typename> class Handler, typename... Args>
auto DispatchFieldDescriptor(const proto2::FieldDescriptor* field_desc,
                             Args... args)
    -> decltype(Handler<int32>::HandleField(field_desc, args...)) {
  switch (field_desc->cpp_type()) {
    case proto2::FieldDescriptor::CPPTYPE_INT32:
      return Handler<int32>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_INT64:
      return Handler<int64>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_UINT32:
      return Handler<uint32>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_UINT64:
      return Handler<uint64>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_FLOAT:
      return Handler<float>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_DOUBLE:
      return Handler<double>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_BOOL:
      return Handler<bool>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_STRING:
      return Handler<std::string>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_MESSAGE:
      return Handler<proto2::Message>::HandleField(field_desc, args...);
    case proto2::FieldDescriptor::CPPTYPE_ENUM:
      return Handler<GenericEnum>::HandleField(field_desc, args...);
    default:
      throw std::runtime_error("Unknown cpp_type: " +
                               std::to_string(field_desc->cpp_type()));
  }
}

// Convert the cpp_type used by DispatchFieldDescriptor to the type that is
// used as as argument/ return values in the ProtoFieldContainer below.
// This basically maps:
// proto2::Message -> proto2::Message*
// GenericEnum -> int
// T -> T for all other types.
template <typename T>
struct ProtoFieldAccess {
  using Type = T;
};
template <>
struct ProtoFieldAccess<proto2::Message> {
  using Type = proto2::Message*;
};
template <>
struct ProtoFieldAccess<GenericEnum> {
  using Type = int;
};

// Calls pybind11::cast, but change the exception type to type_error.
template <typename T>
typename ProtoFieldAccess<T>::Type CastArg(handle arg) {
  try {
    return cast<typename ProtoFieldAccess<T>::Type>(arg);
  } catch (const cast_error& e) {
    throw type_error(e.what());
  }
}

// Registers ProtoType in pybind11 if it is not already registered.
template <typename ProtoType>
void RegisterMessageType(const ProtoType* message);

// Specialization for abstract messages. This most commonly happens with
// sub-messages, which are accessed through the reflection interface.
// pybind11::class_ can only register classes based a template parameter.
// Therefore, we will register it to a placeholder and re-registered it under
// the correct typeid based on RTTI.
template <>
void RegisterMessageType(const proto2::Message* message);

class ProtoFieldContainerBase {
 public:
  ProtoFieldContainerBase(proto2::Message* proto,
                          const proto2::FieldDescriptor* field_desc)
      : proto_(proto),
        field_desc_(field_desc),
        reflection_(proto->GetReflection()) {}

  // Return the size of a repeated field.
  int Size() const { return reflection_->FieldSize(*proto_, field_desc_); }

  // Clear the field.
  void Clear() { this->reflection_->ClearField(proto_, field_desc_); }

 protected:
  // Throws an out_of_range exception if the index is bad.
  void CheckIndex(int idx, int allowed_size = -1) const;

  proto2::Message* proto_;
  const proto2::FieldDescriptor* field_desc_;
  const proto2::Reflection* reflection_;
};

// Create a template-specialization based reflection interface,
// which can be used with template meta-programming.
//
// Proto ProtoFieldContainer should be the only thing in this file that
// that directly uses the native reflection interface. All other accesses
// into the proto should go through the ProtoFieldContainer.
//
// Specializations should implement the following functions:
// cpp_type Get(int idx) const: Returns the value of the field, at the given
//   index if it is a repeated field (idx is ignored for singular fields).
// cpp_type or handle GetPython(int idx) const: Converts the return value of
//   of Get(idx) to a python handle if that conversion is non-trivial.
//   If it is trivial, this can just be an alias for Get(idx).
// void Set(int idx, cpp_type value): Sets the value of the field, at the given
//   index if it is a repeated field (idx is ignored for singular fields).
// void Add(handle value): Converts and adds the value to a repeated field.
// std::string ElementRepr(int idx) const: Convert the element to a string.
//
// Note: cpp_type may not be exactly the same as the template argument type-
// it could be a reference or a pointer to that type. Use ProtoFieldAccess<T>
// To get the exact type that is returned by the Get() method.
template <typename T>
class ProtoFieldContainer {};

// Create specializations of that template class for each numeric type.
// Unfortunately the type name is in the function names used to access the
// field, so the only way to do this is with a macro.
#define NUMERIC_FIELD_REFLECTION_SPECIALIZATION(func_type, cpp_type)           \
  template <>                                                                  \
  class ProtoFieldContainer<cpp_type> : public ProtoFieldContainerBase {       \
   public:                                                                     \
    using ProtoFieldContainerBase::ProtoFieldContainerBase;                    \
    cpp_type Get(int idx) const {                                              \
      if (field_desc_->is_repeated()) {                                        \
        CheckIndex(idx);                                                       \
        return reflection_->GetRepeated##func_type(*proto_, field_desc_, idx); \
      } else {                                                                 \
        return reflection_->Get##func_type(*proto_, field_desc_);              \
      }                                                                        \
    }                                                                          \
    cpp_type GetPython(int idx) const { return Get(idx); }                     \
    void Set(int idx, cpp_type value) {                                        \
      if (field_desc_->is_repeated()) {                                        \
        CheckIndex(idx);                                                       \
        reflection_->SetRepeated##func_type(proto_, field_desc_, idx, value);  \
      } else {                                                                 \
        reflection_->Set##func_type(proto_, field_desc_, value);               \
      }                                                                        \
    }                                                                          \
    void Add(handle value) {                                                   \
      reflection_->Add##func_type(proto_, field_desc_,                         \
                                  CastArg<cpp_type>(value));                   \
    }                                                                          \
    std::string ElementRepr(int idx) const {                                   \
      return std::to_string(Get(idx));                                         \
    }                                                                          \
  }

NUMERIC_FIELD_REFLECTION_SPECIALIZATION(Int32, int32);
NUMERIC_FIELD_REFLECTION_SPECIALIZATION(Int64, int64);
NUMERIC_FIELD_REFLECTION_SPECIALIZATION(UInt32, uint32);
NUMERIC_FIELD_REFLECTION_SPECIALIZATION(UInt64, uint64);
NUMERIC_FIELD_REFLECTION_SPECIALIZATION(Float, float);
NUMERIC_FIELD_REFLECTION_SPECIALIZATION(Double, double);
NUMERIC_FIELD_REFLECTION_SPECIALIZATION(Bool, bool);
#undef NUMERIC_FIELD_REFLECTION_SPECIALIZATION

// Specialization for strings.
template <>
class ProtoFieldContainer<std::string> : public ProtoFieldContainerBase {
 public:
  using ProtoFieldContainerBase::ProtoFieldContainerBase;
  const std::string& Get(int idx) const {
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      return reflection_->GetRepeatedStringReference(*proto_, field_desc_, idx,
                                                     &scratch);
    } else {
      return reflection_->GetStringReference(*proto_, field_desc_, &scratch);
    }
  }
  handle GetPython(int idx) const {
    // Convert the given value to a python string or bytes object.
    // If byte fields are returned as standard strings, pybind11 will attempt
    // to decode them as utf-8. However, some byte sequences are illegal in
    // utf-8, and will result in an error. To allow any arbitrary byte sequence
    // for a bytes field, we have to convert it to a python bytes type.
    if (field_desc_->type() == proto2::FieldDescriptor::TYPE_BYTES)
      return bytes(Get(idx)).release();
    else
      return str(Get(idx)).release();
  }
  void Set(int idx, const std::string& value) {
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      reflection_->SetRepeatedString(proto_, field_desc_, idx, value);
    } else {
      reflection_->SetString(proto_, field_desc_, value);
    }
  }
  void Add(handle value) {
    reflection_->AddString(proto_, field_desc_, CastArg<std::string>(value));
  }
  std::string ElementRepr(int idx) const {
    if (field_desc_->type() == proto2::FieldDescriptor::TYPE_BYTES)
      return "<Binary String>";
    else
      return "'" + Get(idx) + "'";
  }

 private:
  mutable std::string scratch;
};

// Specialization for messages.
// Any methods in here which return a Message should register that message.
template <>
class ProtoFieldContainer<proto2::Message> : public ProtoFieldContainerBase {
 public:
  using ProtoFieldContainerBase::ProtoFieldContainerBase;
  proto2::Message* Get(int idx) const {
    proto2::Message* message;
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      message = reflection_->MutableRepeatedMessage(proto_, field_desc_, idx);
    } else {
      message = reflection_->MutableMessage(proto_, field_desc_);
    }
    RegisterMessageType(message);
    return message;
  }
  proto2::Message* GetPython(int idx) const { return Get(idx); }
  void Set(int idx, proto2::Message* value) {
    if (value->GetTypeName() != field_desc_->message_type()->full_name())
      throw type_error("Cannot set field from invalid type.");
    Get(idx)->CopyFrom(*value);
  }
  void Add(handle value) {
    PyProtoCheckType(value, field_desc_->message_type()->full_name());
    reflection_->AddAllocatedMessage(
        proto_, field_desc_,
        PyProtoAllocateAndCopyMessage<proto2::Message>(value).release());
  }
  proto2::Message* AddDefault() {
    proto2::Message* new_msg =
        reflection_
            ->GetMutableRepeatedFieldRef<proto2::Message>(proto_, field_desc_)
            .NewMessage();
    RegisterMessageType(new_msg);
    reflection_->AddAllocatedMessage(proto_, field_desc_, new_msg);
    return new_msg;
  }
  std::string ElementRepr(int idx) const {
    return Get(idx)->ShortDebugString();
  }
};

// Specialization for enums.
template <>
class ProtoFieldContainer<GenericEnum> : public ProtoFieldContainerBase {
 public:
  using ProtoFieldContainerBase::ProtoFieldContainerBase;
  const proto2::EnumValueDescriptor* GetDesc(int idx) const {
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      return reflection_->GetRepeatedEnum(*proto_, field_desc_, idx);
    } else {
      return reflection_->GetEnum(*proto_, field_desc_);
    }
  }
  int Get(int idx) const { return GetDesc(idx)->number(); }
  int GetPython(int idx) const { return Get(idx); }
  void Set(int idx, int value) {
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      reflection_->SetRepeatedEnumValue(proto_, field_desc_, idx, value);
    } else {
      reflection_->SetEnumValue(proto_, field_desc_, value);
    }
  }
  void Add(handle value) {
    reflection_->AddEnumValue(proto_, field_desc_, value.cast<int>());
  }
  std::string ElementRepr(int idx) const { return GetDesc(idx)->name(); }
};

// A container for a repeated field.
template <typename T>
class RepeatedFieldContainer : public ProtoFieldContainer<T> {
 public:
  using ProtoFieldContainer<T>::ProtoFieldContainer;
  void Extend(handle src) {
    if (!isinstance<sequence>(src))
      throw std::invalid_argument("Extend: Passed value is not a sequence.");
    auto values = reinterpret_borrow<sequence>(src);
    for (auto value : values) this->Add(value);
  }
  void Insert(int idx, handle value) {
    this->CheckIndex(idx, this->Size() + 1);
    // Add a new element to the end.
    this->Add(value);
    // Slide all existing values down one index.
    for (int dst = this->Size() - 1; dst > idx; --dst)
      SwapElements(dst, dst - 1);
  }
  void Remove(int idx) {
    // TODO(kenoslund): Get this to work for repeated messages.
    // Current it gives a 'use of uninitialized value' error.
    if (std::is_same<T, proto2::Message>::value)
      throw std::runtime_error("Remove does not work for repeated messages.");
    this->CheckIndex(idx);
    // Slide all existing values up one index.
    for (int dst = idx; dst < this->Size(); ++dst) SwapElements(dst, dst + 1);
    // Remove the last value
    this->reflection_->RemoveLast(this->proto_, this->field_desc_);
  }
  std::string Repr() const {
    if (this->Size() == 0) return "[]";
    std::string repr = "[";
    for (int i = 0; i < this->Size(); ++i) repr += this->ElementRepr(i) + ", ";
    repr.pop_back();
    repr.back() = ']';
    return repr;
  }

 protected:
  void SwapElements(int i1, int i2) {
    this->reflection_->SwapElements(this->proto_, this->field_desc_, i1, i2);
  }
};

template <typename KeyT>
struct FindMapPair {
  static proto2::Message* HandleField(const proto2::FieldDescriptor* key_desc,
                                      proto2::Message* proto,
                                      const proto2::FieldDescriptor* map_desc,
                                      handle key, bool add_key = true) {
    // When using the proto reflection API, maps are represented as repeated
    // message fields (messages with 2 elements: 'key' and 'value'). If protocol
    // buffers guarrantee a consistent (and useful) ordering of elements,
    // it should be possible to do this search in O(log(n)) time. However, that
    // requires more knowledge of protobuf internals than I have, so for now
    // assume a random ordering of elements, in which case a O(n) search is
    // the best you can do.
    RepeatedFieldContainer<proto2::Message> map_field(proto, map_desc);
    auto target_key = CastArg<KeyT>(key);
    for (int i = 0; i < map_field.Size(); ++i) {
      proto2::Message* kv_pair = map_field.Get(i);
      if (ProtoFieldContainer<KeyT>(kv_pair, key_desc).Get(-1) == target_key)
        return kv_pair;
    }
    // Key not found
    if (!add_key) return nullptr;
    proto2::Message* new_kv_pair = map_field.AddDefault();
    ProtoFieldContainer<KeyT>(new_kv_pair, key_desc).Set(-1, target_key);
    return new_kv_pair;
  }
};

template <typename KeyT, typename ValueT>
struct MapRepr {
  static std::string HandleField(const proto2::FieldDescriptor* key_desc,
                                 proto2::Message* proto,
                                 const proto2::FieldDescriptor* map_desc,
                                 const proto2::FieldDescriptor* value_desc) {
    RepeatedFieldContainer<proto2::Message> map_field(proto, map_desc);
    if (map_field.Size() == 0) return "{}";
    std::string repr = "{";
    for (int i = 0; i < map_field.Size(); ++i) {
      proto2::Message* kv_pair = map_field.Get(i);
      repr += ProtoFieldContainer<KeyT>(kv_pair, key_desc).ElementRepr(-1);
      repr += ": ";
      repr += ProtoFieldContainer<ValueT>(kv_pair, value_desc).ElementRepr(-1);
      repr += ", ";
    }
    repr.pop_back();
    repr.back() = '}';
    return repr;
  }
};

// Container for a map field.
template <typename MappedType>
class MapFieldContainer : public RepeatedFieldContainer<proto2::Message> {
 public:
  MapFieldContainer(proto2::Message* proto,
                    const proto2::FieldDescriptor* map_desc,
                    const proto2::FieldDescriptor* key_desc,
                    const proto2::FieldDescriptor* value_desc)
      : RepeatedFieldContainer<proto2::Message>(proto, map_desc),
        key_desc_(key_desc),
        value_desc_(value_desc) {}
  using RepeatedFieldContainer<proto2::Message>::RepeatedFieldContainer;
  typename ProtoFieldAccess<MappedType>::Type Get(handle key) const {
    // This automatically inserts missing keys, which matches native python
    // protos, not dicts (see http://go/pythonprotobuf#undefined).
    return GetValueContainer(key).Get(-1);
  }
  void Set(handle key, typename ProtoFieldAccess<MappedType>::Type value) {
    return GetValueContainer(key).Set(-1, value);
  }
  bool Contains(handle key) const {
    return DispatchFieldDescriptor<FindMapPair>(key_desc_, proto_, field_desc_,
                                                key, false) != nullptr;
  }
  std::string Repr() const {
    return DispatchFieldDescriptor<_MapRepr>(key_desc_, proto_, field_desc_,
                                             value_desc_);
  }

 protected:
  template <typename KeyT>
  using _MapRepr = MapRepr<KeyT, MappedType>;

  ProtoFieldContainer<MappedType> GetValueContainer(handle key) const {
    proto2::Message* kv_pair = DispatchFieldDescriptor<FindMapPair>(
        key_desc_, proto_, field_desc_, key);
    return ProtoFieldContainer<MappedType>(kv_pair, value_desc_);
  }

  const proto2::FieldDescriptor* key_desc_;
  const proto2::FieldDescriptor* value_desc_;
};

// Add bindings for a singular field (other than a sub-message).
template <typename ProtoType, typename FieldType>
struct AddProtoSingularField {
  static void HandleField(const proto2::FieldDescriptor* field_desc,
                          class_<ProtoType, proto2::Message>* pybind_class_) {
    pybind_class_->def_property(
        field_desc->name().c_str(),
        [field_desc](proto2::Message* proto) {
          return ProtoFieldContainer<FieldType>(proto, field_desc)
              .GetPython(-1);
        },
        [field_desc](proto2::Message* proto,
                     const typename ProtoFieldAccess<FieldType>::Type& value) {
          return ProtoFieldContainer<FieldType>(proto, field_desc)
              .Set(-1, value);
        });
  }
};

// Add bindings for a singular sub-message field.
template <typename ProtoType>
struct AddProtoSingularField<ProtoType, proto2::Message> {
  static void HandleField(const proto2::FieldDescriptor* field_desc,
                          class_<ProtoType, proto2::Message>* pybind_class_) {
    pybind_class_->def_property_readonly(
        field_desc->name().c_str(),
        [field_desc](proto2::Message* proto) {
          return ProtoFieldContainer<proto2::Message>(proto, field_desc)
              .GetPython(-1);
        },
        return_value_policy::reference_internal);
  }
};

// Add bindings for a repeated field.
template <typename ProtoType, typename ValueType>
struct AddProtoRepeatedField {
  static void HandleField(const proto2::FieldDescriptor* field_desc,
                          class_<ProtoType, proto2::Message>* pybind_class_) {
    pybind_class_->def_property_readonly(
        field_desc->name().c_str(),
        [field_desc](proto2::Message* proto) {
          return RepeatedFieldContainer<ValueType>(proto, field_desc);
        },
        return_value_policy::reference_internal);
  }
};

// Add bindings for a map field.
template <typename ProtoType, typename ValueType>
struct AddProtoMapField {
  static void HandleField(const proto2::FieldDescriptor* value_descriptor,
                          const proto2::FieldDescriptor* key_descriptor,
                          const proto2::FieldDescriptor* map_descriptor,
                          class_<ProtoType, proto2::Message>* pybind_class_) {
    pybind_class_->def_property_readonly(
        map_descriptor->name().c_str(),
        [map_descriptor, key_descriptor,
         value_descriptor](proto2::Message* proto) {
          return MapFieldContainer<ValueType>(proto, map_descriptor,
                                              key_descriptor, value_descriptor);
        },
        return_value_policy::reference_internal);
  }
};

// Class to add python bindings to a RepeatedFieldContainer.
template <typename T>
class RepeatedFieldBindings : public class_<RepeatedFieldContainer<T>> {
 public:
  RepeatedFieldBindings(handle scope, const std::string& name)
      : class_<RepeatedFieldContainer<T>>(scope, name.c_str()) {
    return_value_policy get_policy = return_value_policy::automatic;
    if (std::is_same<T, proto2::Message>::value)
      get_policy = return_value_policy::reference_internal;
    else
      this->def("__setitem__", &RepeatedFieldContainer<T>::Set);

    this->def("__repr__", &RepeatedFieldContainer<T>::Repr);
    this->def("__len__", &RepeatedFieldContainer<T>::Size);
    this->def("__getitem__", &RepeatedFieldContainer<T>::Get, get_policy);
    this->def("MergeFrom", &RepeatedFieldContainer<T>::Extend);
    this->def("extend", &RepeatedFieldContainer<T>::Extend);
    this->def("append", &RepeatedFieldContainer<T>::Add);
    this->def("insert", &RepeatedFieldContainer<T>::Insert);
    this->def("remove", &RepeatedFieldContainer<T>::Remove);
    this->def("clear", &RepeatedFieldContainer<T>::Clear);
  }
};

// Class to add python bindings to a MapFieldContainer.
template <typename T>
class MapFieldBindings : public class_<MapFieldContainer<T>> {
 public:
  MapFieldBindings(handle scope, const std::string& name)
      : class_<MapFieldContainer<T>>(scope, name.c_str()) {
    return_value_policy get_policy = return_value_policy::automatic;
    if (std::is_same<T, proto2::Message>::value)
      get_policy = return_value_policy::reference_internal;
    else
      this->def("__setitem__", &MapFieldContainer<T>::Set);

    this->def("__repr__", &MapFieldContainer<T>::Repr);
    this->def("__len__", &MapFieldContainer<T>::Size);
    this->def("__contains__", &MapFieldContainer<T>::Contains);
    this->def("__getitem__", &MapFieldContainer<T>::Get, get_policy);
    this->def("clear", &RepeatedFieldContainer<T>::Clear);
  }
};

// Class to add python bindings to a protocol buffer message.
template <typename ProtoType>
class ProtoBindings : public class_<ProtoType, proto2::Message> {
 public:
  explicit ProtoBindings(const proto2::Descriptor* descriptor)
      : class_<ProtoType, proto2::Message>(nullptr,
                                           descriptor->name().c_str()) {
    for (int i = 0; i < descriptor->field_count(); ++i) {
      auto* field_desc = descriptor->field(i);
      if (field_desc->is_map()) {
        auto* map_pair_descriptor = field_desc->message_type();
        auto* map_value_field_desc =
            map_pair_descriptor->FindFieldByName("value");
        auto* map_key_field_desc = map_pair_descriptor->FindFieldByName("key");
        DispatchFieldDescriptor<_AddProtoMapField>(
            map_value_field_desc, map_key_field_desc, field_desc, this);
      } else if (field_desc->is_repeated()) {
        DispatchFieldDescriptor<_AddProtoRepeatedField>(field_desc, this);
      } else {
        DispatchFieldDescriptor<_AddProtoSingularField>(field_desc, this);
      }
    }
  }

 protected:
  template <typename FieldType>
  using _AddProtoSingularField = AddProtoSingularField<ProtoType, FieldType>;

  template <typename FieldType>
  using _AddProtoRepeatedField = AddProtoRepeatedField<ProtoType, FieldType>;

  template <typename FieldType>
  using _AddProtoMapField = AddProtoMapField<ProtoType, FieldType>;
};

template <template <typename> class Bindings>
void BindEachFieldType(module& module, const std::string& name) {
  Bindings<int32>(module, name + "Int32");
  Bindings<int64>(module, name + "Int64");
  Bindings<uint32>(module, name + "UInt32");
  Bindings<uint64>(module, name + "UInt64");
  Bindings<float>(module, name + "Float");
  Bindings<double>(module, name + "Double");
  Bindings<bool>(module, name + "Bool");
  Bindings<std::string>(module, name + "String");
  Bindings<proto2::Message>(module, name + "Message");
  Bindings<GenericEnum>(module, name + "Enum");
}

// Returns whether we should register the type of the message.
// We should register it if 1) it is not currently registered or 2) it was
// registered with an abstract type and now we have a conrete type.
bool RegistrationNeeded(const proto2::Message* message,
                        bool have_concrete_type);

// Registers ProtoType in pybind11 if it is not already registered.
template <typename ProtoType>
void RegisterMessageType(const ProtoType* message) {
  ImportProtoModule();
  if (message->GetDescriptor() != ProtoType::GetDescriptor())
    throw std::invalid_argument(
        "Passed message type does not match template type.");
  if (RegistrationNeeded(message, true))
    ProtoBindings<ProtoType>{message->GetDescriptor()};
}

}  // namespace google
}  // namespace pybind11

#endif  // THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_UTILS_H_

// Copyright (c) 2019 The Pybind Development Team. All rights reserved.
//
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_UTILS_H_
#define THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_UTILS_H_

#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

#include "google/protobuf/any.proto.h"
#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"
#include "net/proto2/public/reflection.h"

namespace pybind11 {
namespace google {

// Alias for checking whether a c++ type is a proto.
template <typename T>
inline constexpr bool is_proto_v = std::is_base_of_v<proto2::Message, T>;

// Imports the proto module.
void ImportProtoModule();

// Name of the property which indicates whether a proto is a wrapped or native.
constexpr char kIsWrappedCProtoAttr[] = "_is_wrapped_c_proto";

// Returns true if the given python object is a wrapped C proto.
inline bool IsWrappedCProto(handle handle) {
  return hasattr(handle, kIsWrappedCProtoAttr);
}

// Gets the field with the given name from the given message as a python object.
object ProtoGetField(proto2::Message* message, const std::string& name);
object ProtoGetField(proto2::Message* message,
                     const proto2::FieldDescriptor* field_desc);

// Sets the field with the given name in the given message from a python object.
// As in the native API, message, repeated, and map fields cannot be set.
void ProtoSetField(proto2::Message* message, const std::string& name,
                   handle value);
void ProtoSetField(proto2::Message* message,
                   const proto2::FieldDescriptor* field_desc, handle value);

// Initializes the fields in the given message from the the keyword args.
// Unlike ProtoSetField, this allows setting message, map and repeated fields.
void ProtoInitFields(proto2::Message* message, kwargs kwargs_in);

// If py_proto is a native or wrapped python proto, extract and return its name.
// Otherwise, return std::nullopt.
std::optional<std::string> PyProtoFullName(handle py_proto);

// Returns whether py_proto is a proto and matches the expected_type.
bool PyProtoCheckType(handle py_proto, const std::string& expected_type);
// Throws a type error if py_proto is not a proto or the wrong message type.
void PyProtoCheckTypeOrThrow(handle py_proto, const std::string& expected_type);

// Returns whether py_proto is a proto and matches the ProtoType.
template <typename ProtoType>
bool PyProtoCheckType(handle py_proto) {
  return PyProtoCheckType(py_proto, ProtoType::descriptor()->full_name());
}

// Returns whether py_proto is a proto.
template <>
inline bool PyProtoCheckType<proto2::Message>(handle py_proto) {
  return PyProtoFullName(py_proto).has_value();
}

// Returns the serialized version of the given (native or wrapped) python proto.
std::string PyProtoSerializeToString(handle py_proto);

// Allocate and return the ProtoType given by the template argument.
// py_proto is not used in this version, but is used by a specialization below.
template <typename ProtoType>
std::unique_ptr<ProtoType> PyProtoAllocateMessage(handle py_proto = handle(),
                                                  kwargs kwargs_in = kwargs()) {
  auto message = std::make_unique<ProtoType>();
  ProtoInitFields(message.get(), kwargs_in);
  return message;
}

// Specialization for the case that the template argument is a generic message.
// The type is pulled from the py_proto, which can be a native python proto,
// a wrapped C proto, or a string with the full type name.
template <>
std::unique_ptr<proto2::Message> PyProtoAllocateMessage(handle py_proto,
                                                        kwargs kwargs_in);

// Allocate and return a message instance for the given descriptor.
std::unique_ptr<proto2::Message> PyProtoAllocateMessage(
    const proto2::Descriptor* descriptor, kwargs kwargs_in);

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
// requires only 1 serialization operation. Returns true on success.
bool AnyPackFromPyProto(handle py_proto, ::google::protobuf::Any* any_proto);

// A type used with DispatchFieldDescriptor to represent a generic enum value.
struct GenericEnum {};

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
  // If this field is a map, the field_desc always describes a message with
  // 2 fields: key and value. In that case, dispatch based on the value type.
  // In all other cases, the value type is the same as the field type.
  const proto2::FieldDescriptor* field_value_desc = field_desc;
  if (field_desc->is_map())
    field_value_desc = field_desc->message_type()->FindFieldByName("value");
  switch (field_value_desc->cpp_type()) {
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

// Calls pybind11::cast, but change the exception type to type_error.
template <typename T>
T CastOrTypeError(handle arg) {
  try {
    return cast<T>(arg);
  } catch (const cast_error& e) {
    throw type_error(e.what());
  }
}

// Base class for type-agnostic functionality and data shared between all
// specializations of the ProtoFieldContainer (see below).
class ProtoFieldContainerBase {
 public:
  // Accesses the field indicated by `field_desc` in the given `proto`. `parent`
  // should be passed if and only if `proto` is a map key-value pair message.
  // In that case, it should point to the message which contains the map field.
  // (ie, the parent of the key-value pair). In all other cases, `proto` is
  // also the parent, which is indicated by passing nullptr for `parent`.
  ProtoFieldContainerBase(proto2::Message* proto,
                          const proto2::FieldDescriptor* field_desc,
                          proto2::Message* parent = nullptr)
      : proto_(proto),
        parent_(parent ? parent : proto),
        field_desc_(field_desc),
        reflection_(proto->GetReflection()) {}

  // Return the size of a repeated field.
  int Size() const { return reflection_->FieldSize(*proto_, field_desc_); }

  // Clear the field.
  void Clear() { this->reflection_->ClearField(proto_, field_desc_); }

 protected:
  // Throws an out_of_range exception if the index is bad.
  void CheckIndex(int idx, int allowed_size = -1) const;

  // Cast `inst` and keep its parent alive until `inst` is no longer referenced.
  // Use this when (and only when) casting message, map and repeated fields.
  template <typename T>
  object CastAndKeepAlive(T* inst, return_value_policy policy) const {
    object py_inst = cast(inst, policy);
    object py_parent = cast(parent_, return_value_policy::reference);
    detail::keep_alive_impl(py_inst, py_parent);
    return py_inst;
  }

  proto2::Message* proto_;
  proto2::Message* parent_;
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
// object GetPython(int idx) const: Converts the return value of
//   of Get(idx) to a python object.
// void Set(int idx, cpp_type value): Sets the value of the field, at the given
//   index if it is a repeated field (idx is ignored for singular fields).
// void SetPython(int idx, handle value): Sets the value of the field from the
//   given python value.
// void Append(handle value): Converts and adds the value to a repeated field.
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
    object GetPython(int idx) const { return cast(Get(idx)); }                 \
    void Set(int idx, cpp_type value) {                                        \
      if (field_desc_->is_repeated()) {                                        \
        CheckIndex(idx);                                                       \
        reflection_->SetRepeated##func_type(proto_, field_desc_, idx, value);  \
      } else {                                                                 \
        reflection_->Set##func_type(proto_, field_desc_, value);               \
      }                                                                        \
    }                                                                          \
    void SetPython(int idx, handle value) {                                    \
      Set(idx, CastOrTypeError<cpp_type>(value));                              \
    }                                                                          \
    void Append(handle value) {                                                \
      reflection_->Add##func_type(proto_, field_desc_,                         \
                                  CastOrTypeError<cpp_type>(value));           \
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
  object GetPython(int idx) const {
    // Convert the given value to a python string or bytes object.
    // If byte fields are returned as standard strings, pybind11 will attempt
    // to decode them as utf-8. However, some byte sequences are illegal in
    // utf-8, and will result in an error. To allow any arbitrary byte sequence
    // for a bytes field, we have to convert it to a python bytes type.
    if (field_desc_->type() == proto2::FieldDescriptor::TYPE_BYTES)
      return bytes(Get(idx));
    else
      return str(Get(idx));
  }
  void Set(int idx, const std::string& value) {
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      reflection_->SetRepeatedString(proto_, field_desc_, idx, value);
    } else {
      reflection_->SetString(proto_, field_desc_, value);
    }
  }
  void SetPython(int idx, handle value) {
    Set(idx, CastOrTypeError<std::string>(value));
  }
  void Append(handle value) {
    reflection_->AddString(proto_, field_desc_,
                           CastOrTypeError<std::string>(value));
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
    return message;
  }
  object GetPython(int idx) const {
    return this->CastAndKeepAlive(Get(idx), return_value_policy::reference);
  }
  void Set(int idx, proto2::Message* value) {
    if (value->GetTypeName() != field_desc_->message_type()->full_name())
      throw type_error("Cannot set field from invalid type.");
    Get(idx)->CopyFrom(*value);
  }
  void SetPython(int idx, handle value) {
    // Value could be a native or wrapped C++ proto.
    CheckValueType(value);
    Get(idx)->ParseFromString(PyProtoSerializeToString(value));
  }
  void Append(handle value) {
    CheckValueType(value);
    reflection_->AddAllocatedMessage(
        proto_, field_desc_,
        PyProtoAllocateAndCopyMessage<proto2::Message>(value).release());
  }
  proto2::Message* AddDefault() {
    proto2::Message* new_msg =
        reflection_
            ->GetMutableRepeatedFieldRef<proto2::Message>(proto_, field_desc_)
            .NewMessage();
    reflection_->AddAllocatedMessage(proto_, field_desc_, new_msg);
    return new_msg;
  }
  std::string ElementRepr(int idx) const {
    return Get(idx)->ShortDebugString();
  }

 private:
  void CheckValueType(handle value) {
    if (!PyProtoCheckType(value, field_desc_->message_type()->full_name()))
      throw type_error("Cannot set field from invalid type.");
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
  object GetPython(int idx) const { return cast(Get(idx)); }
  void Set(int idx, int value) {
    if (field_desc_->is_repeated()) {
      CheckIndex(idx);
      reflection_->SetRepeatedEnumValue(proto_, field_desc_, idx, value);
    } else {
      reflection_->SetEnumValue(proto_, field_desc_, value);
    }
  }
  void SetPython(int idx, handle value) {
    Set(idx, CastOrTypeError<int>(value));
  }
  void Append(handle value) {
    reflection_->AddEnumValue(proto_, field_desc_, CastOrTypeError<int>(value));
  }
  std::string ElementRepr(int idx) const { return GetDesc(idx)->name(); }
};

// A container for a repeated field.
template <typename T>
class RepeatedFieldContainer : public ProtoFieldContainer<T> {
 public:
  using ProtoFieldContainer<T>::ProtoFieldContainer;
  object GetPythonContainer() const {
    return this->CastAndKeepAlive(this, return_value_policy::copy);
  }
  void Extend(handle src) {
    if (!isinstance<sequence>(src))
      throw std::invalid_argument("Extend: Passed value is not a sequence.");
    auto values = reinterpret_borrow<sequence>(src);
    for (auto value : values) this->Append(value);
  }
  void Insert(int idx, handle value) {
    this->CheckIndex(idx, this->Size() + 1);
    // Append a new element to the end.
    this->Append(value);
    // Slide all existing values down one index.
    for (int dst = this->Size() - 1; dst > idx; --dst)
      SwapElements(dst, dst - 1);
  }
  void Delete(int idx) {
    // TODO(b/145687965): Get this to work for repeated messages.
    // Current it gives a 'use of uninitialized value' error.
    if (std::is_same_v<T, proto2::Message>)
      throw std::runtime_error("Remove does not work for repeated messages.");
    this->CheckIndex(idx);
    // Slide all existing values up one index.
    for (int dst = idx; dst < this->Size(); ++dst) SwapElements(dst, dst + 1);
    // Remove the last value
    this->reflection_->RemoveLast(this->proto_, this->field_desc_);
  }
  // TODO(b/145687883): Support the case that indices is a slice.
  object GetItem(handle indices) { return this->GetPython(cast<int>(indices)); }
  void DelItem(handle indices) { Delete(cast<int>(indices)); }
  void SetItem(handle indices, handle values) {
    this->SetPython(cast<int>(indices), values);
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

// Implement the Add method for repeated fields, which only exists for repeated
// message fields. Therefore only the specialization should ever be called.
template <typename T>
T* AddMessage(RepeatedFieldContainer<T>* container, kwargs kwargs_in) {
  std::abort();
}
template <>
proto2::Message* AddMessage(RepeatedFieldContainer<proto2::Message>* container,
                            kwargs kwargs_in);

// Struct which can be used with DispatchFieldDescriptor to find (or add)
// the map pair (key, value) message with the given key.
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
    for (int i = 0; i < map_field.Size(); ++i) {
      proto2::Message* kv_pair = map_field.Get(i);
      // TODO(kenoslund, rwgk): This probably doesn't work if KeyT is a message.
      if (ProtoFieldContainer<KeyT>(kv_pair, key_desc).GetPython(-1).equal(key))
        return kv_pair;
    }
    // Key not found
    if (!add_key) return nullptr;
    proto2::Message* new_kv_pair = map_field.AddDefault();
    ProtoFieldContainer<KeyT>(new_kv_pair, key_desc).SetPython(-1, key);
    return new_kv_pair;
  }
};

// Struct which can be used with DispatchFieldDescriptor to get the value of
// the map key.
template <typename KeyType>
struct GetMapKey {
  static object HandleField(const proto2::FieldDescriptor* key_desc,
                            proto2::Message* kv_pair, proto2::Message* parent) {
    return ProtoFieldContainer<KeyType>(kv_pair, key_desc, parent)
        .GetPython(-1);
  }
};

// Struct which can be used with DispatchFieldDescriptor to get the string
// representation of the map key.
template <typename KeyType>
struct GetMapKeyRepr {
  static std::string HandleField(const proto2::FieldDescriptor* key_desc,
                                 proto2::Message* kv_pair) {
    return ProtoFieldContainer<KeyType>(kv_pair, key_desc).ElementRepr(-1);
  }
};

// Container for a map field.
template <typename MappedType>
class MapFieldContainer : public RepeatedFieldContainer<proto2::Message> {
 public:
  MapFieldContainer(proto2::Message* proto,
                    const proto2::FieldDescriptor* map_desc)
      : RepeatedFieldContainer<proto2::Message>(proto, map_desc),
        key_desc_(map_desc->message_type()->FindFieldByName("key")),
        value_desc_(map_desc->message_type()->FindFieldByName("value")) {}
  using RepeatedFieldContainer<proto2::Message>::RepeatedFieldContainer;
  object GetPythonContainer() const {
    return this->CastAndKeepAlive(this, return_value_policy::copy);
  }
  // GetItem automatically inserts missing keys, which matches native
  // python protos, not dicts (see http://go/pythonprotobuf#undefined).
  object GetItem(handle key) const {
    return GetValueContainer(key).GetPython(-1);
  }
  void SetItem(handle key, handle value) {
    GetValueContainer(key).SetPython(-1, value);
  }
  void UpdateFromDict(dict values) {
    for (auto& item : values) SetItem(item.first, item.second);
  }
  void UpdateFromKWArgs(kwargs values) { UpdateFromDict(values); }
  void UpdateFromHandle(handle values) {
    if (!isinstance<dict>(values))
      throw std::invalid_argument("Update: Passed value is not a dictionary.");
    UpdateFromDict(reinterpret_borrow<dict>(values));
  }
  bool Contains(handle key) const {
    return DispatchFieldDescriptor<FindMapPair>(key_desc_, proto_, field_desc_,
                                                key, false) != nullptr;
  }
  std::string Repr() const {
    if (Size() == 0) return "{}";
    std::string repr = "{";
    for (int i = 0; i < Size(); ++i) {
      proto2::Message* kv_pair = Get(i);
      repr += DispatchFieldDescriptor<GetMapKeyRepr>(key_desc_, kv_pair) +
              ": " +
              ProtoFieldContainer<MappedType>(kv_pair, value_desc_)
                  .ElementRepr(-1) +
              ", ";
    }
    repr.pop_back();
    repr.back() = '}';
    return repr;
  }
  std::function<std::unique_ptr<proto2::Message>(kwargs)>
  GetEntryClassFactory() {
    return [descriptor = field_desc_->message_type()](kwargs kwargs_in) {
      return PyProtoAllocateMessage(descriptor, kwargs_in);
    };
  }

  // Iterator class for maps.
  class Iterator {
   public:
    // First argument is the container, second is a member function to extract
    // the appropriate value from a key-value pair message (ie, this function
    // should return either the key, the value, or a (key, value) tuple).
    Iterator(MapFieldContainer* container,
             object (MapFieldContainer::*value_helper)(proto2::Message*))
        : container_(container), value_helper_(value_helper) {}

    auto* iter() { return this; }
    object next() {
      if (idx_ >= container_->Size()) throw stop_iteration();
      return (container_->*value_helper_)(container_->Get(idx_++));
    }

   private:
    MapFieldContainer* container_;
    object (MapFieldContainer::*value_helper_)(proto2::Message*);
    int idx_ = 0;
  };

  Iterator KeyIterator() { return Iterator(this, &MapFieldContainer::GetKey); }
  Iterator ValueIterator() {
    return Iterator(this, &MapFieldContainer::GetValue);
  }
  Iterator ItemIterator() {
    return Iterator(this, &MapFieldContainer::GetTuple);
  }

 protected:
  // Note that the helper functions bellow all pass the message which contains
  // the key-value pair to the ProtoFieldContainer as the 3rd argument.

  // Get the ProtoFieldContainer for the value corresponding to the given key.
  ProtoFieldContainer<MappedType> GetValueContainer(handle key) const {
    proto2::Message* kv_pair = DispatchFieldDescriptor<FindMapPair>(
        key_desc_, proto_, field_desc_, key);
    return ProtoFieldContainer<MappedType>(kv_pair, value_desc_, proto_);
  }

  // Get the key out of the given key-value message.
  object GetKey(proto2::Message* kv_pair) {
    return DispatchFieldDescriptor<GetMapKey>(key_desc_, kv_pair, proto_);
  }

  // Get the value out of the given key-value message.
  object GetValue(proto2::Message* kv_pair) {
    return ProtoFieldContainer<MappedType>(kv_pair, value_desc_, proto_)
        .GetPython(-1);
  }

  // Get the given key-value pair as a message.
  object GetTuple(proto2::Message* kv_pair) {
    return make_tuple(GetKey(kv_pair), GetValue(kv_pair));
  }

  const proto2::FieldDescriptor* key_desc_;
  const proto2::FieldDescriptor* value_desc_;
};

// Struct used with DispatchFieldDescriptor to get the value of a field.
template <typename ValueType>
struct TemplatedProtoGetField {
  static object HandleField(const proto2::FieldDescriptor* field_desc,
                            proto2::Message* proto) {
    if (field_desc->is_map()) {
      return MapFieldContainer<ValueType>(proto, field_desc)
          .GetPythonContainer();
    } else if (field_desc->is_repeated()) {
      return RepeatedFieldContainer<ValueType>(proto, field_desc)
          .GetPythonContainer();
    } else {  // Singular field.
      return ProtoFieldContainer<ValueType>(proto, field_desc).GetPython(-1);
    }
  }
};

// Struct used with DispatchFieldDescriptor to set the value of a field.
template <typename ValueType>
struct TemplatedProtoSetField {
  static void HandleField(const proto2::FieldDescriptor* field_desc,
                          proto2::Message* proto, handle value) {
    if (field_desc->is_map()) {
      MapFieldContainer<ValueType> map_field(proto, field_desc);
      map_field.Clear();
      map_field.UpdateFromHandle(value);
    } else if (field_desc->is_repeated()) {
      RepeatedFieldContainer<ValueType> repeated_field(proto, field_desc);
      repeated_field.Clear();
      repeated_field.Extend(value);
    } else {  // Singular field.
      ProtoFieldContainer<ValueType>(proto, field_desc).SetPython(-1, value);
    }
  }
};

// Wrapper around proto2::Message::FindInitializationErrors.
std::vector<std::string> MessageFindInitializationErrors(
    proto2::Message* message);

// Wrapper around proto2::Message::ListFields.
std::vector<tuple> MessageListFields(proto2::Message* message);

// Wrapper around proto2::Message::HasField.
bool MessageHasField(proto2::Message* message, const std::string& field_name);

// Wrapper around proto2::Message::Copy/MergeFrom.
void MessageCopyFrom(proto2::Message* msg, handle other);
void MessageMergeFrom(proto2::Message* msg, handle other);

// Wrapper to generate the python message Descriptor.fields_by_name property.
dict MessageFieldsByName(const proto2::Descriptor* descriptor);

// Wrapper to generate the python EnumDescriptor.values_by_* properties.
dict EnumValuesByNumber(const proto2::EnumDescriptor* enum_descriptor);
dict EnumValuesByName(const proto2::EnumDescriptor* enum_descriptor);

// Returns the pickle bindings (passed to class_::def) for ProtoType.
template <typename ProtoType>
decltype(auto) MakePickler() {
  return pybind11::pickle(
      [](ProtoType* message) {
        return dict("serialized"_a = bytes(message->SerializeAsString()),
                    "type_name"_a = message->GetTypeName());
      },
      [](dict d) {
        auto message = PyProtoAllocateMessage<ProtoType>(d["type_name"]);
        // TODO(b/145925674): Use ParseFromStringPiece once str
        // supports string_view casting.
        message->ParseFromString(str(d["serialized"]));
        return message;
      });
}

// Register the given concrete ProtoType with pybind11.
template <typename ProtoType>
void RegisterProtoMessageType(handle module = nullptr) {
  // Make sure the proto2::Message bindings are available.
  google::ImportProtoModule();
  // Register the type.
  auto* descriptor = ProtoType::GetDescriptor();
  const char* registered_name = descriptor->name().c_str();
  class_<ProtoType, proto2::Message> message_c(module, registered_name);
  // Add a constructor.
  message_c.def(init([](kwargs kwargs_in) {
    return google::PyProtoAllocateMessage<ProtoType>(handle(), kwargs_in);
  }));
  // Create a pickler for this type (the base class pickler will fail).
  message_c.def(google::MakePickler<ProtoType>());
  // Add the descriptor as a static field.
  message_c.def_readonly_static("DESCRIPTOR", descriptor);
  // Add bindings for each field, to avoid the FindFieldByName lookup.
  for (int i = 0; i < descriptor->field_count(); ++i) {
    auto* field_desc = descriptor->field(i);
    message_c.def_property(
        field_desc->name().c_str(),
        [field_desc](proto2::Message* message) {
          return ProtoGetField(message, field_desc);
        },
        [field_desc](proto2::Message* message, handle value) {
          ProtoSetField(message, field_desc, value);
        });
  }
}

// The base class is already registered by the proto module, so this is a no-op.
template <>
inline void RegisterProtoMessageType<proto2::Message>(handle) {}

}  // namespace google
}  // namespace pybind11

#endif  // THIRD_PARTY_PYBIND11_GOOGLE3_UTILS_PROTO_UTILS_H_

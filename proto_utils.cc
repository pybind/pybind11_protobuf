#include "pybind11_protobuf/proto_utils.h"

#include <pybind11/cast.h>
#include <pybind11/detail/class.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>

#include "net/proto2/public/message.h"

namespace pybind11 {
namespace google {

string PyProtoFullName(handle py_proto) {
  if (hasattr(py_proto, "DESCRIPTOR")) {
    auto descriptor = py_proto.attr("DESCRIPTOR");
    if (hasattr(descriptor, "full_name"))
      return cast<string>(descriptor.attr("full_name"));
  }
  throw std::invalid_argument("Passed python object is not a proto.");
}

void PyProtoCheckType(handle py_proto, const string& expected_type) {
  const string& type_in = PyProtoFullName(py_proto);
  if (type_in != expected_type)
    // We were passed a proto of the wrong type.
    throw std::invalid_argument("Cannot convert a proto of type " + type_in +
                                " to type " + expected_type);
}

string PyProtoSerializeToString(handle py_proto) {
  if (hasattr(py_proto, "SerializeToString"))
    return cast<string>(py_proto.attr("SerializeToString")());
  throw std::invalid_argument("Passed python object is not a proto.");
}

template <>
std::unique_ptr<proto2::Message> PyProtoAllocateMessage(handle py_proto) {
  string full_type_name = PyProtoFullName(py_proto);
  const proto2::Descriptor* descriptor =
      proto2::DescriptorPool::generated_pool()->FindMessageTypeByName(
          full_type_name);
  if (!descriptor)
    throw std::runtime_error("Proto Descriptor not found: " + full_type_name);
  const proto2::Message* prototype =
      proto2::MessageFactory::generated_factory()->GetPrototype(descriptor);
  if (!prototype)
    throw std::runtime_error(
        "Not able to generate prototype for descriptor of: " + full_type_name);
  return absl::WrapUnique(prototype->New());
}

void AnyPackFromPyProto(handle py_proto, ::google::protobuf::Any* any_proto) {
  any_proto->set_type_url("type.googleapis.com/" + PyProtoFullName(py_proto));
  any_proto->set_value(PyProtoSerializeToString(py_proto));
}

// Throws an out_of_range exception if the index is bad.
void ProtoFieldContainerBase::CheckIndex(int idx, int allowed_size) const {
  if (allowed_size < 0) allowed_size = Size();
  if (idx < 0 || idx >= allowed_size)
    throw std::out_of_range(("Bad index: " + std::to_string(idx) +
                             " (max: " + std::to_string(allowed_size - 1) + ")")
                                .c_str());
}

// A version of class_::init_instance which works for abstract messages.
// For abstract messages, we register them under a placeholder typeid then
// move the registration to the correct place. However, the version in
// class_::init_instance is not aware of this and would attempt to lookup the
// type info based on the placeholder typeid, and therefore fails.
// This version does not look up the typeid at all.
void init_abstract_proto(detail::instance* inst, const void* holder_ptr) {
  auto v_h = inst->get_value_and_holder();
  if (!v_h.instance_registered()) {
    // For simple ancestors, register_instance == register_instance_impl.
    register_instance_impl(v_h.value_ptr(), inst);
    v_h.set_instance_registered();
  }
  if (holder_ptr || inst->owned) {
    // If you want to implement this, you basically need to replicate the
    // functionality of pybind11::class_::init_holder here.
    throw std::runtime_error(
        "Transferring ownership of an abstract proto from C to python is not "
        "supported. Theoretically this is possible, but it's not implemented.");
  }
}

// Returns the type registry containing the given message.
detail::type_map<detail::type_info*>& GetTypeRegistry() {
  return detail::get_internals().registered_types_cpp;
}

bool RegistrationNeeded(const proto2::Message* message,
                        bool have_concrete_type) {
  auto& type_registry = GetTypeRegistry();
  auto type_info = type_registry.find(typeid(*message));
  if (type_info == type_registry.end()) {
    // This message type has not been registered.
    return true;
  } else if (have_concrete_type &&
             type_info->second->init_instance == &init_abstract_proto) {
    // This message type has been registered, but only as an abstract type.
    // We now have the concrete type, so we should re-register it.
    type_registry.erase(type_info);
    return true;
  } else {
    // This message has already been registered.
    return false;
  }
}

class PlaceholderMessage : public proto2::Message {};
template <>
void RegisterMessageType(const proto2::Message* message) {
  ImportProtoModule();
  if (RegistrationNeeded(message, false)) {
    // Register this type under a placeholder typeid.
    ProtoBindings<PlaceholderMessage> registration{message->GetDescriptor()};

    // Move the registration to the correct typeid.
    auto& type_registry = GetTypeRegistry();
    auto* type_info = type_registry[typeid(PlaceholderMessage)];
    type_registry[typeid(*message)] = type_info;
    type_registry.erase(typeid(PlaceholderMessage));

    // Modify the init_instance function so it works with abstract protos.
    type_info->init_instance = &init_abstract_proto;
  }
}

}  // namespace google
}  // namespace pybind11

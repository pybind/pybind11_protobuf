#include "pybind11_protobuf/fast_cpp_proto_casters.h"

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "glog/logging.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "python/google/protobuf/proto_api.h"

void pybind11_proto_casters_collision() {
  // This symbol intentionally defined in both fast_cpp_proto_casters.cc
  // and proto_utils.cc to detect potential ODR violations in libraries.

  // Avoid mixing fast_cpp_proto_casters.h and proto_casters.h in the same
  // build target; this violates the ODR rule for type_caster<::google::protobuf::Message>
  // as well as other potential types, and can lead to hard to diagnose bugs,
  // crashes, and other mysterious bad behavior.

  // See https://github.com/pybind/pybind11/issues/2695 for more details.
}

namespace py = pybind11;

using ::google::protobuf::python::PyProto_API;

namespace pybind11_protobuf::fast_cpp_protos {

const ::google::protobuf::python::PyProto_API* GetPyProtoApi() {
  static auto* result = [&]() -> const PyProto_API* {
    try {
      // When we're not using fast_cpp_protos, we can force the capsule
      // to be loaded by importing the correct module.
      py::module_::import("google3.net.proto2.python.internal.cpp._message");
    } catch (...) {
      // Ignore any errors; they will appear immediately when the capsule
      // is requested below.
      PyErr_Clear();
    }

    return static_cast<const ::google::protobuf::python::PyProto_API*>(
        PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
  }();

  return result;
}

const ::google::protobuf::Message* GetCppProtoMessagePointer(py::handle src) {
  auto* ptr = GetPyProtoApi()->GetMessagePointer(src.ptr());
  if (ptr == nullptr) {
    // GetMessagePointer sets a type_error when the object is not a
    // c++ wrapped proto message.
    PyErr_Clear();
    return nullptr;
  }
  return ptr;
}

std::optional<std::string> PyDescriptorName(py::handle py_proto) {
  if (!py::hasattr(py_proto, "DESCRIPTOR")) {
    return std::nullopt;
  }
  auto py_descriptor = py_proto.attr("DESCRIPTOR");
  if (!py::hasattr(py_descriptor, "full_name")) {
    return std::nullopt;
  }
  auto py_full_name = py_descriptor.attr("full_name");

  pybind11::detail::make_caster<std::string> c;
  if (!c.load(py_full_name, false)) {
    return std::nullopt;
  }
  return static_cast<std::string>(c);
}

std::unique_ptr<::google::protobuf::Message> PyAllocateCProtoByName(
    const std::string& full_name) {
  // Lookup the name in the PyProto_API descriptor pool, which uses the default
  // pool as a fallback. Ideally we get the right message.
  const auto* py_proto_api = fast_cpp_protos::GetPyProtoApi();
  const auto* descriptor =
      py_proto_api->GetDefaultDescriptorPool()->FindMessageTypeByName(
          full_name);
  if (!descriptor) {
    return nullptr;
  }
  const ::google::protobuf::Message* prototype =
      py_proto_api->GetDefaultMessageFactory()->GetPrototype(descriptor);
  if (!prototype) {
    return nullptr;
  }
  return std::unique_ptr<::google::protobuf::Message>(prototype->New());
}

bool PyCopyProtoToCProto(py::handle py_proto, ::google::protobuf::Message* message) {
  py::object wire = py_proto.attr("SerializePartialToString")();
  const char* bytes =
      (wire == nullptr) ? nullptr : PYBIND11_BYTES_AS_STRING(wire.ptr());
  if (!bytes) {
    throw py::type_error("Object.SerializePartialToString failed; is this a " +
                         message->GetDescriptor()->full_name());
  }
  return message->ParsePartialFromArray(bytes, PYBIND11_BYTES_SIZE(wire.ptr()));
}

std::pair<py::object, ::google::protobuf::Message*> cpp_to_py_impl::allocate(
    const ::google::protobuf::Descriptor* src_descriptor) {
  CHECK(src_descriptor != nullptr);
  const auto* py_proto_api = fast_cpp_protos::GetPyProtoApi();
  auto* descriptor =
      py_proto_api->GetDefaultDescriptorPool()->FindMessageTypeByName(
          src_descriptor->full_name());

  if (descriptor == nullptr) {
    // This typically indicates a descriptor from a non-default pool, such as
    // a dynamically generated pool. It may be possible to resolve the type
    // by adding a DescriptorProto to the python pool for this type.
    throw py::type_error(
        "Cannot construct python proto; descriptor not found in pool for " +
        src_descriptor->full_name());
  }

  py::object result = py::reinterpret_steal<py::object>(
      py_proto_api->NewMessage(descriptor, nullptr));
  if (result.ptr() == nullptr) {
    throw py::error_already_set();
  }
  ::google::protobuf::Message* message =
      py_proto_api->GetMutableMessagePointer(result.ptr());
  return {std::move(result), message};
}

py::object cpp_to_py_impl::reference(::google::protobuf::Message* src) {
  const auto* py_proto_api = fast_cpp_protos::GetPyProtoApi();
  return py::reinterpret_steal<py::object>(
      py_proto_api->NewMessageOwnedExternally(src, nullptr));
}

}  // namespace pybind11_protobuf::fast_cpp_protos

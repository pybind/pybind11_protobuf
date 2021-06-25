#include "pybind11_protobuf/proto_cast_util.h"

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
#include "absl/strings/str_replace.h"

namespace py = pybind11;

using ::google::protobuf::python::PyProto_API;

namespace pybind11_protobuf {
namespace {

struct GlobalState {
  bool using_fast_cpp = false;
  const ::google::protobuf::python::PyProto_API* py_proto_api = nullptr;
  py::object global_pool;
  py::object factory;
};

const GlobalState* GetGlobalState() {
  static GlobalState state = [&]() {
    GlobalState state;
    state.py_proto_api = static_cast<const ::google::protobuf::python::PyProto_API*>(
        PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
    if (state.py_proto_api) {
      state.using_fast_cpp = true;
    } else {
      // The module implementing fast cpp protos is not loaded, so evidently
      // they are not the default. Fast protos are still available as a fallback
      // by importing the capsule.
      state.using_fast_cpp = false;

      PyErr_Clear();
      try {
        py::module_::import("google3.net.proto2.python.internal.cpp._message");
      } catch (...) {
        // TODO(pybind11-infra): narrow down to expected exception(s).
        // Ignore any errors; they will appear immediately when the capsule
        // is requested below.
        PyErr_Clear();
      }
      state.py_proto_api = static_cast<const ::google::protobuf::python::PyProto_API*>(
          PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
    }

    if (!state.using_fast_cpp) {
      // When not using fast protos, we may construct protos from the default
      // pool.
      try {
        auto m = py::module_::import("google3.net.proto2.python.public");
        state.global_pool = m.attr("descriptor_pool").attr("Default")();
        state.factory =
            m.attr("message_factory").attr("MessageFactory")(state.global_pool);
      } catch (...) {
        // TODO(pybind11-infra): narrow down to expected exception(s).
        PyErr_Print();
        PyErr_Clear();
        state.global_pool = {};
        state.factory = {};
      }
    }
    return state;
  }();

  return &state;
}

const ::google::protobuf::python::PyProto_API* GetPyProtoApi() {
  return GetGlobalState()->py_proto_api;
}

std::string PythonPackageForDescriptor(const ::google::protobuf::FileDescriptor* file) {
  std::vector<std::pair<const absl::string_view, std::string>> replacements;
  replacements.emplace_back("/", ".");
  replacements.emplace_back(".proto", "_pb2");

  std::string name = "google3/" + file->name();
  return absl::StrReplaceAll(name, replacements);
}

}  // namespace

const ::google::protobuf::Message* PyProtoGetCppMessagePointer(py::handle src) {
  auto* ptr = GetPyProtoApi()->GetMessagePointer(src.ptr());
  if (ptr == nullptr) {
    // GetMessagePointer sets a type_error when the object is not a
    // c++ wrapped proto message.
    PyErr_Clear();
    return nullptr;
  }
  return ptr;
}

std::optional<std::string> PyProtoDescriptorName(py::handle py_proto) {
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

std::unique_ptr<::google::protobuf::Message> AllocateCProtoByName(
    const std::string& full_name) {
  // Lookup the name in the PyProto_API descriptor pool, which uses the default
  // pool as a fallback. Ideally we get the right message.
  const auto* py_proto_api = GetPyProtoApi();
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

bool PyProtoCopyToCProto(py::handle py_proto, ::google::protobuf::Message* message) {
  py::object wire = py_proto.attr("SerializePartialToString")();
  const char* bytes =
      (wire == nullptr) ? nullptr : PYBIND11_BYTES_AS_STRING(wire.ptr());
  if (!bytes) {
    throw py::type_error("Object.SerializePartialToString failed; is this a " +
                         message->GetDescriptor()->full_name());
  }
  return message->ParsePartialFromArray(bytes, PYBIND11_BYTES_SIZE(wire.ptr()));
}

std::pair<py::object, ::google::protobuf::Message*> AllocatePyFastCppProto(
    const ::google::protobuf::Descriptor* descriptor) {
  CHECK(descriptor != nullptr);
  const auto* py_proto_api = GetPyProtoApi();
  auto* found_descriptor =
      py_proto_api->GetDefaultDescriptorPool()->FindMessageTypeByName(
          descriptor->full_name());

  if (found_descriptor == nullptr) {
    // This typically indicates a descriptor from a non-default pool, such as
    // a dynamically generated pool. It may be possible to resolve the type
    // by adding a DescriptorProto to the python pool for this type.
    throw py::type_error(
        "Cannot construct python proto; descriptor not found in pool for " +
        descriptor->full_name());
  }

  py::object result = py::reinterpret_steal<py::object>(
      py_proto_api->NewMessage(found_descriptor, nullptr));
  if (result.ptr() == nullptr) {
    throw py::error_already_set();
  }
  ::google::protobuf::Message* message =
      py_proto_api->GetMutableMessagePointer(result.ptr());
  return {std::move(result), message};
}

py::object ReferencePyFastCppProto(::google::protobuf::Message* message) {
  const auto* py_proto_api = GetPyProtoApi();
  return py::reinterpret_steal<py::object>(
      py_proto_api->NewMessageOwnedExternally(message, nullptr));
}

namespace {

// Resolves the class name of a descriptor via d->containing_type()
py::object ResolveDescriptor(py::object p, const ::google::protobuf::Descriptor* d) {
  return d->containing_type() ? ResolveDescriptor(p, d->containing_type())
                                    .attr(d->name().c_str())
                              : p.attr(d->name().c_str());
}

}  // namespace

py::handle GenericPyProtoCast(::google::protobuf::Message* src,
                              py::return_value_policy policy, py::handle parent,
                              bool is_const) {
  assert(src != nullptr);
  auto py_proto = [src]() -> py::object {
    auto* descriptor = src->GetDescriptor();
    auto* state = GetGlobalState();

    // First attempt to construct the proto from the global pool.
    if (state->global_pool) {
      try {
        auto d = state->global_pool.attr("FindMessageTypeByName")(
            descriptor->full_name());
        auto p = state->factory.attr("GetPrototype")(d);
        return p();
      } catch (...) {
        // TODO(pybind11-infra): narrow down to expected exception(s).
        PyErr_Clear();
      }
    }

    // If that fails, attempt to import the module.
    auto module_name = PythonPackageForDescriptor(descriptor->file());
    if (!module_name.empty()) {
      try {
        return ResolveDescriptor(py::module_::import(module_name.c_str()),
                                 descriptor)();
      } catch (...) {
        // TODO(pybind11-infra): narrow down to expected exception(s).
        PyErr_Clear();
      }
    }

    throw py::type_error(
        "Cannot construct a protocol buffer message type " +
        descriptor->full_name() +
        " in python. Is there a missing dependency on module " + module_name +
        "?");
  }();

  auto serialized = src->SerializePartialAsString();
  py::object buffer = py::reinterpret_steal<py::object>(PyMemoryView_FromMemory(
      const_cast<char*>(serialized.data()), serialized.size(), PyBUF_READ));
  py_proto.attr("MergeFromString")(buffer);
  return py_proto.release();
}

py::handle GenericFastCppProtoCast(::google::protobuf::Message* src,
                                   py::return_value_policy policy,
                                   py::handle parent, bool is_const) {
  assert(src != nullptr);
  switch (policy) {
    case py::return_value_policy::automatic:
    case py::return_value_policy::copy: {
      auto [result, result_message] =
          pybind11_protobuf::AllocatePyFastCppProto(src->GetDescriptor());

      if (result_message->GetDescriptor() == src->GetDescriptor()) {
        // Only protos which actually have the same descriptor are copyable.
        result_message->CopyFrom(*src);
      } else {
        auto serialized = src->SerializePartialAsString();
        if (!result_message->ParseFromString(serialized)) {
          throw py::type_error(
              "Failed to copy protocol buffer with mismatched descriptor");
        }
      }
      return result.release();
    } break;

    case py::return_value_policy::reference:
    case py::return_value_policy::reference_internal: {
      // NOTE: Reference to const are currently unsafe to return.
      pybind11::object result = pybind11_protobuf::ReferencePyFastCppProto(src);
      if (policy == py::return_value_policy::reference_internal) {
        py::detail::keep_alive_impl(result, parent);
      }
      return result.release();
    } break;

    default:
      throw py::cast_error("unhandled return_value_policy: should not happen!");
  }
}

py::handle GenericProtoCast(::google::protobuf::Message* src,
                            py::return_value_policy policy, py::handle parent,
                            bool is_const) {
  assert(src != nullptr);
  if (src->GetDescriptor()->file()->pool() ==
          ::google::protobuf::DescriptorPool::generated_pool() &&
      !GetGlobalState()->using_fast_cpp) {
    // Return a native python-allocated proto when the C++ proto is from
    // the default pool, and the binary is not using fast_cpp_protos.
    return GenericPyProtoCast(src, policy, parent, is_const);
  }
  // If this is a dynamically generated proto, then we're going to need to
  // construct a mapping between C++ pool() and python pool(), and then
  // use the PyProto_API to make it work.

  return GenericFastCppProtoCast(src, policy, parent, is_const);
}

}  // namespace pybind11_protobuf

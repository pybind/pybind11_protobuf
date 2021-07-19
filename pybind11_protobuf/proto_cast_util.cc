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
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "python/google/protobuf/proto_api.h"
#include "absl/container/flat_hash_map.h"
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
  // Global state intentionally leaks at program termination.
  // If destructed along with other static variables, it causes segfaults
  // due to order of destruction conflict with python threads. See
  // https://github.com/pybind/pybind11/issues/1598
  static GlobalState* state = [&]() {
    auto state = new GlobalState();
    state->py_proto_api = static_cast<const ::google::protobuf::python::PyProto_API*>(
        PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
    if (state->py_proto_api) {
      state->using_fast_cpp = true;
    } else {
      // The module implementing fast cpp protos is not loaded, so evidently
      // they are not the default. Fast protos are still available as a fallback
      // by importing the capsule.
      state->using_fast_cpp = false;

      PyErr_Clear();
      try {
        py::module_::import("google.protobuf.pyext._message");
      } catch (...) {
        // TODO(pybind11-infra): narrow down to expected exception(s).
        // Ignore any errors; they will appear immediately when the capsule
        // is requested below.
        PyErr_Clear();
      }
      state->py_proto_api = static_cast<const ::google::protobuf::python::PyProto_API*>(
          PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
    }

    if (!state->using_fast_cpp) {
      // When not using fast protos, we may construct protos from the default
      // pool.
      try {
        auto m = py::module_::import("google.protobuf");
        state->global_pool = m.attr("descriptor_pool").attr("Default")();
        state->factory = m.attr("message_factory")
                             .attr("MessageFactory")(state->global_pool);
      } catch (...) {
        // TODO(pybind11-infra): narrow down to expected exception(s).
        PyErr_Print();
        PyErr_Clear();
        state->global_pool = {};
        state->factory = {};
      }
    }
    return state;
  }();

  return state;
}

const ::google::protobuf::python::PyProto_API* GetPyProtoApi() {
  return GetGlobalState()->py_proto_api;
}

std::string PythonPackageForDescriptor(const ::google::protobuf::FileDescriptor* file) {
  std::vector<std::pair<const absl::string_view, std::string>> replacements;
  replacements.emplace_back("/", ".");
  replacements.emplace_back(".proto", "_pb2");
  std::string name = file->name();
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

// Create C++ DescriptorPools based on Python DescriptorPools.
// The Python pool will provide message definitions when they are needed.
// This gives an efficient way to create C++ Messages from Python definitions.
class PythonDescriptorPoolWrapper {
 public:
  // The singleton which handles multiple wrapped pools.
  // It is never deallocated, but data corresponding to a Python pool
  // is cleared when the pool is destroyed.
  static PythonDescriptorPoolWrapper* instance() {
    static auto instance = new PythonDescriptorPoolWrapper();
    return instance;
  }

  // To build messages these 3 objects often come together:
  // - a DescriptorDatabase provides the representation of .proto files.
  // - a DescriptorPool manages the live descriptors with cross-linked pointers.
  // - a MessageFactory manages the proto instances and their memory layout.
  struct Data {
    std::unique_ptr<::google::protobuf::DescriptorDatabase> database;
    std::unique_ptr<const ::google::protobuf::DescriptorPool> pool;
    std::unique_ptr<::google::protobuf::MessageFactory> factory;
  };

  // Return (and maybe create) a C++ DescriptorPool that corresponds to the
  // given Python DescriptorPool.
  // The returned pointer has the same lifetime as the Python DescriptorPool:
  // its data will be deleted when the Python object is deleted.
  const Data* GetPoolFromPythonPool(py::handle python_pool) {
    PyObject* key = python_pool.ptr();
    // Get or create an entry for this key.
    auto& pool_entry = pools_map[key];
    if (pool_entry.database) {
      // Found in cache, return it.
      return &pool_entry;
    }

    // We retain a weak reference to the python_pool.
    // On deletion, the callback will remove the entry from the map,
    // which will delete the Data instance.
    auto pool_weakref = py::weakref(
        python_pool,
        py::cpp_function([this, key](py::handle weakref) {
          pools_map.erase(key);
        }));

    auto database = absl::make_unique<DescriptorPoolDatabase>(pool_weakref);
    auto pool = absl::make_unique<::google::protobuf::DescriptorPool>(database.get());
    auto factory = absl::make_unique<::google::protobuf::DynamicMessageFactory>(pool.get());
    // When wrapping the Python descriptor_poool.Default(), apply an important
    // optimization:
    // - the pool is based on the C++ generated_pool(), so that
    //   compiled C++ modules can be found without using the DescriptorDatabase
    //   and the Python DescriptorPool.
    // - the MessageFactory returns instances of C++ compiled messages when
    //   possible: some methods are much more optimized, and the created
    //   ::google::protobuf::Message can be cast to the C++ class.  We use this last
    //   property in the proto_caster class.
    // This is done only for the Default pool, because generated C++ modules
    // and generated Python modules are built from the same .proto sources.
    if (python_pool.is(GetGlobalState()->global_pool)) {
      pool->internal_set_underlay(::google::protobuf::DescriptorPool::generated_pool());
      factory->SetDelegateToGeneratedFactory(true);
    }

    // Cache the created objects.
    pool_entry = Data{std::move(database), std::move(pool), std::move(factory)};
    return &pool_entry;
  }

 private:
  PythonDescriptorPoolWrapper() = default;

  // Similar to ::google::protobuf::DescriptorPoolDatabase: wraps a Python DescriptorPool
  // as a DescriptorDatabase.
  class DescriptorPoolDatabase : public ::google::protobuf::DescriptorDatabase {
   public:
    DescriptorPoolDatabase(py::weakref python_pool)
        : pool_(python_pool) {
    }

    // These 3 methods implement ::google::protobuf::DescriptorDatabase and delegate to
    // the Python DescriptorPool.

    // Find a file by file name.
    bool FindFileByName(const std::string& filename,
                        ::google::protobuf::FileDescriptorProto* output) {
      return CallActionAndFillProto("FindFileByName", output, [&]() {
        return pool_().attr("FindFileByName")(filename);
      });
    }

    // Find the file that declares the given fully-qualified symbol name.
    bool FindFileContainingSymbol(
        const std::string& symbol_name,
        ::google::protobuf::FileDescriptorProto* output) override {
      return CallActionAndFillProto("FindFileContainingSymbol", output, [&]() {
        return pool_().attr("FindFileContainingSymbol")(symbol_name);
      });
    }

    // Find the file which defines an extension extending the given message type
    // with the given field number.
    bool FindFileContainingExtension(
        const std::string& containing_type, int field_number,
        ::google::protobuf::FileDescriptorProto* output) override {
      return CallActionAndFillProto(
          "FindFileContainingExtension", output, [&]() {
            auto descriptor =
                pool_().attr("FindMessageTypeByName")(containing_type);
            return pool_()
                .attr("FindExtensionByNymber")(descriptor, field_number)
                .attr("file");
          });
    }

   private:
    // The inner pool yields FileDescriptors, but this Database must return
    // FileDescriptorProtos. This function does the conversion.
    static bool GetFileDescriptorProto(py::handle py_descriptor,
                                       ::google::protobuf::FileDescriptorProto* output) {
      // This code is mostly useful when using the pure-python protos. Yet we
      // can use the fast_cpp_protos api to wrap the output pointer in a Python
      // object.
      const auto* py_proto_api = GetPyProtoApi();
      auto proto = py::reinterpret_steal<py::object>(
          py_proto_api->NewMessageOwnedExternally(output, nullptr));
      if (!proto) {
        throw py::error_already_set();
      }
      py_descriptor.attr("CopyToProto")(proto);
      return true;
    }

    // Helper method to handle exceptions.
    template <typename F>
    static bool CallActionAndFillProto(const char* method,
                                       ::google::protobuf::FileDescriptorProto* output,
                                       F f) {
      try {
        return GetFileDescriptorProto(f(), output);
      } catch (py::error_already_set& e) {
        LOG(ERROR) << "DescriptorDatabase::" << method << " raised an error";
        // This prints and clears the error.
        e.restore();
        PyErr_Print();
        return false;
      }
    }

    py::weakref pool_;
  };

  // This map caches the wrapped objects, indexed by DescriptorPool address.
  absl::flat_hash_map<PyObject*, Data> pools_map;
};

std::unique_ptr<::google::protobuf::Message> AllocateCProtoFromPythonSymbolDatabase(
    py::handle src, const std::string& full_name) {
  py::object pool;
  try {
    pool = src.attr("DESCRIPTOR").attr("file").attr("pool");
  } catch (py::error_already_set& e) {
    if (e.matches(PyExc_AttributeError)) {
      throw py::type_error("Object is not a valid protobuf");
    } else {
      throw;
    }
  }
  auto pool_data =
      PythonDescriptorPoolWrapper::instance()->GetPoolFromPythonPool(pool);
  // The following call will query the DescriptorDatabase, which fetches the
  // necessary Python descriptors and feeds them into the C++ pool.
  // The result stays cached as long as the Python pool stays alive.
  const ::google::protobuf::Descriptor* descriptor =
      pool_data->pool->FindMessageTypeByName(full_name);
  if (!descriptor) {
    throw py::type_error("Could not find descriptor: " + full_name);
  }
  const ::google::protobuf::Message* prototype =
      pool_data->factory->GetPrototype(descriptor);
  if (!prototype) {
    throw py::type_error("Unable to get prototype for " + full_name);
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

  // Create a PyDescriptorPool, temporarily, it will be used by the NewMessage
  // API call which will store it in the classes it creates.
  //
  // Note: Creating Python classes is a bit expensive, it might be a good idea
  // for client code to create the pool once, and store it somewhere along with
  // the C++ pool; then Python pools and classes are cached and reused.
  // Otherwise, consecutives calls to this function may or may not reuse
  // previous classes, depending on whether the returned instance has been
  // kept alive.
  //
  // IMPORTANT CAVEAT: The C++ DescriptorPool must not be deallocated while
  // there are any messages using it.
  // Furthermore, since the cache uses the DescriptorPool address, allocating
  // a new DescriptorPool with the same address is likely to use dangling
  // pointers.
  // It is probably better for client code to keep the C++ DescriptorPool alive
  // until the end of the process.
  // TODO(amauryfa): Add weakref or on-deletion callbacks to C++ DescriptorPool.
  py::object descriptor_pool = py::reinterpret_steal<py::object>(
      py_proto_api->DescriptorPool_FromPool(descriptor->file()->pool()));
  if (descriptor_pool.ptr() == nullptr) {
    throw py::error_already_set();
  }

  py::object result = py::reinterpret_steal<py::object>(
      py_proto_api->NewMessage(descriptor, nullptr));
  if (result.ptr() == nullptr) {
    throw py::error_already_set();
  }
  ::google::protobuf::Message* message =
      py_proto_api->GetMutableMessagePointer(result.ptr());
  if (message == nullptr) {
    throw py::error_already_set();
  }
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

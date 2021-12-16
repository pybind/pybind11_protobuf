#include "pybind11_protobuf/proto_cast_util.h"

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "python/google/protobuf/proto_api.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_replace.h"
#include "absl/types/optional.h"

namespace py = pybind11;

using ::google::protobuf::python::PyProto_API;

namespace pybind11_protobuf {
namespace {

std::string PythonPackageForDescriptor(const ::google::protobuf::FileDescriptor* file) {
  std::vector<std::pair<const absl::string_view, std::string>> replacements;
  replacements.emplace_back("/", ".");
  replacements.emplace_back(".proto", "_pb2");
  std::string name = file->name();
  return absl::StrReplaceAll(name, replacements);
}

// Resolves the class name of a descriptor via d->containing_type()
py::object ResolveDescriptor(py::object p, const ::google::protobuf::Descriptor* d) {
  return d->containing_type() ? ResolveDescriptor(p, d->containing_type())
                                    .attr(d->name().c_str())
                              : p.attr(d->name().c_str());
}

// Resolves a sequence of python attrs starting from obj.
// If any does not exist, returns nullopt.
inline absl::optional<py::object> ResolveAttrs(
    py::handle obj, std::initializer_list<const char*> names) {
  py::object tmp;
  for (const char* name : names) {
    PyObject* attr = PyObject_GetAttrString(obj.ptr(), name);
    if (attr == nullptr) {
      PyErr_Clear();
      return absl::nullopt;
    }
    tmp = py::reinterpret_steal<py::object>(attr);
    obj = py::handle(attr);
  }
  return tmp;
}

class GlobalState {
 public:
  // Global state singleton intentionally leaks at program termination.
  // If destructed along with other static variables, it causes segfaults
  // due to order of destruction conflict with python threads. See
  // https://github.com/pybind/pybind11/issues/1598
  static GlobalState* instance() {
    static auto instance = new GlobalState();
    return instance;
  }

  py::handle global_pool() { return global_pool_; }
  const ::google::protobuf::python::PyProto_API* py_proto_api() { return py_proto_api_; }
  bool using_fast_cpp() const { return using_fast_cpp_; }

  // Allocate a python proto message instance using the native python
  // allocations.
  py::object PyMessageInstance(const ::google::protobuf::Descriptor* descriptor);

  // Allocates a fast cpp proto python object, also returning
  // the embedded c++ proto2 message type. The returned message
  // pointer cannot be null.
  std::pair<py::object, ::google::protobuf::Message*> PyFastCppProtoMessageInstance(
      const ::google::protobuf::Descriptor* descriptor);

  // Import (and cache) a python module.
  py::module_ ImportCached(const std::string& module_name);

 private:
  GlobalState();

  const ::google::protobuf::python::PyProto_API* py_proto_api_ = nullptr;
  bool using_fast_cpp_ = false;
  py::object global_pool_;
  py::object factory_;
  py::object find_message_type_by_name_;
  py::object get_prototype_;

  absl::flat_hash_map<std::string, py::module_> import_cache;
};

GlobalState::GlobalState() {
  assert(PyGILState_Check());

  // pybind11 protobuf casting needs a dependency on proto internals to work.
  try {
    ImportCached("google.protobuf.descriptor_pb2");
    ImportCached("google.protobuf.descriptor_pool");
    ImportCached("google.protobuf.message_factory");
  } catch (py::error_already_set& e) {
    e.restore();
  }

  // Now we can check
  py_proto_api_ = static_cast<const ::google::protobuf::python::PyProto_API*>(
      PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
  if (py_proto_api_) {
    using_fast_cpp_ = true;
  } else {
    // The module implementing fast cpp protos is not loaded, so evidently
    // they are not the default. We still need the PyProto api, so attempt
    // to import the capsule.
    using_fast_cpp_ = false;

    PyErr_Clear();
    try {
      py::module_::import("google.protobuf.pyext._message");
    } catch (py::error_already_set& e) {
      // TODO(pybind11-infra): narrow down to expected exception(s).

      // e matches PyExc_ModuleNotFoundError if the module could not be loaded.
      // Ignore any errors; they will appear immediately when the capsule
      // is requested below.
      PyErr_Clear();
    }
    py_proto_api_ = static_cast<const ::google::protobuf::python::PyProto_API*>(
        PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
  }

  // When not using fast protos, we may construct protos from the default
  // pool.
  try {
    global_pool_ =
        ImportCached("google.protobuf.descriptor_pool")
            .attr("Default")();
    factory_ = ImportCached("google.protobuf.message_factory")
                   .attr("MessageFactory")(global_pool_);
    find_message_type_by_name_ = global_pool_.attr("FindMessageTypeByName");
    get_prototype_ = factory_.attr("GetPrototype");
  } catch (py::error_already_set& e) {
    // TODO(pybind11-infra): narrow down to expected exception(s).
    PyErr_Print();

    global_pool_ = {};
    factory_ = {};
    find_message_type_by_name_ = {};
    get_prototype_ = {};
  }
}

py::module_ GlobalState::ImportCached(const std::string& module_name) {
  auto cached = import_cache.find(module_name);
  if (cached != import_cache.end()) {
    return cached->second;
  }
  auto module = py::module_::import(module_name.c_str());
  import_cache[module_name] = module;
  return module;
}

py::object GlobalState::PyMessageInstance(
    const ::google::protobuf::Descriptor* descriptor) {
  auto module_name = PythonPackageForDescriptor(descriptor->file());
  if (!module_name.empty()) {
    auto cached = import_cache.find(module_name);
    if (cached != import_cache.end()) {
      return ResolveDescriptor(cached->second, descriptor)();
    }
  }

  // First attempt to construct the proto from the global pool.
  if (global_pool_) {
    try {
      auto d = find_message_type_by_name_(descriptor->full_name());
      auto p = get_prototype_(d);
      return p();
    } catch (...) {
      // TODO(pybind11-infra): narrow down to expected exception(s).
      PyErr_Clear();
    }
  }

  // If that fails, attempt to import the module.
  if (!module_name.empty()) {
    try {
      return ResolveDescriptor(ImportCached(module_name), descriptor)();
    } catch (py::error_already_set& e) {
      // TODO(pybind11-infra): narrow down to expected exception(s).
      e.restore();
      PyErr_Print();
    }
  }

  throw py::type_error("Cannot construct a protocol buffer message type " +
                       descriptor->full_name() +
                       " in python. Is there a missing dependency on module " +
                       module_name + "?");
}

std::pair<py::object, ::google::protobuf::Message*>
GlobalState::PyFastCppProtoMessageInstance(
    const ::google::protobuf::Descriptor* descriptor) {
  assert(descriptor != nullptr);

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
      py_proto_api_->DescriptorPool_FromPool(descriptor->file()->pool()));
  if (descriptor_pool.ptr() == nullptr) {
    throw py::error_already_set();
  }

  py::object result = py::reinterpret_steal<py::object>(
      py_proto_api_->NewMessage(descriptor, nullptr));
  if (result.ptr() == nullptr) {
    throw py::error_already_set();
  }
  ::google::protobuf::Message* message =
      py_proto_api_->GetMutableMessagePointer(result.ptr());
  if (message == nullptr) {
    throw py::error_already_set();
  }
  return {std::move(result), message};
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
        python_pool, py::cpp_function([this, key](py::handle weakref) {
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
    if (python_pool.is(GlobalState::instance()->global_pool())) {
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
    DescriptorPoolDatabase(py::weakref python_pool) : pool_(python_pool) {}

    // These 3 methods implement ::google::protobuf::DescriptorDatabase and delegate to
    // the Python DescriptorPool.

    // Find a file by file name.
    bool FindFileByName(const std::string& filename,
                        ::google::protobuf::FileDescriptorProto* output) override {
      try {
        auto file = pool_().attr("FindFileByName")(filename);
        return CopyToFileDescriptorProto(file, output);
      } catch (py::error_already_set& e) {
            std::cerr
            << "FindFileByName " << filename << " raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }
      return false;
    }

    // Find the file that declares the given fully-qualified symbol name.
    bool FindFileContainingSymbol(
        const std::string& symbol_name,
        ::google::protobuf::FileDescriptorProto* output) override {
      try {
        auto file = pool_().attr("FindFileContainingSymbol")(symbol_name);
        return CopyToFileDescriptorProto(file, output);
      } catch (py::error_already_set& e) {
            std::cerr
            << "FindFileContainingSymbol " << symbol_name << " raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }
      return false;
    }

    // Find the file which defines an extension extending the given message type
    // with the given field number.
    bool FindFileContainingExtension(
        const std::string& containing_type, int field_number,
        ::google::protobuf::FileDescriptorProto* output) override {
      try {
        auto descriptor =
            pool_().attr("FindMessageTypeByName")(containing_type);
        auto file = pool_()
                        .attr("FindExtensionByNymber")(descriptor, field_number)
                        .attr("file");
        return CopyToFileDescriptorProto(file, output);
      } catch (py::error_already_set& e) {
            std::cerr
            << "FindFileContainingExtension " << containing_type << " "
            << field_number << " raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }
      return false;
    }

   private:
    bool CopyToFileDescriptorProto(py::handle py_file_descriptor,
                                   ::google::protobuf::FileDescriptorProto* output) {
      try {
        py::object c_proto = py::reinterpret_steal<py::object>(
            GlobalState::instance()->py_proto_api()->NewMessageOwnedExternally(
                output, nullptr));
        if (c_proto) {
          py_file_descriptor.attr("CopyToProto")(c_proto);
        }
      } catch (py::error_already_set& e) {
            std::cerr
            << "CopyToFileDescriptorProto raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }

      py::object wire = py_file_descriptor.attr("serialized_pb");
      const char* bytes = PYBIND11_BYTES_AS_STRING(wire.ptr());
      return output->ParsePartialFromArray(bytes,
                                           PYBIND11_BYTES_SIZE(wire.ptr()));
    }

    py::weakref pool_;
  };

  // This map caches the wrapped objects, indexed by DescriptorPool address.
  absl::flat_hash_map<PyObject*, Data> pools_map;
};

}  // namespace

void InitializePybindProtoCastUtil() {
  assert(PyGILState_Check());
  GlobalState::instance();
}

void ImportProtoDescriptorModule(const ::google::protobuf::Descriptor* descriptor) {
  assert(PyGILState_Check());
  if (!descriptor) return;
  auto module_name = PythonPackageForDescriptor(descriptor->file());
  if (module_name.empty()) return;
  try {
    GlobalState::instance()->ImportCached(module_name);
  } catch (py::error_already_set& e) {
    if (e.matches(PyExc_ImportError)) {  //  PyExc_ModuleNotFoundError
          std::cerr
          << "Python module " << module_name << " unavailable." << std::endl;
    } else {
          std::cerr
          << "ImportDescriptorModule raised an error";

      // This prints and clears the error.
      e.restore();
      PyErr_Print();
    }
  }
}

const ::google::protobuf::Message* PyProtoGetCppMessagePointer(py::handle src) {
  assert(PyGILState_Check());
  auto* ptr =
      GlobalState::instance()->py_proto_api()->GetMessagePointer(src.ptr());
  if (ptr == nullptr) {
    // GetMessagePointer sets a type_error when the object is not a
    // c++ wrapped proto message.
    PyErr_Clear();
    return nullptr;
  }
  return ptr;
}

absl::optional<std::string> PyProtoDescriptorName(py::handle py_proto) {
  assert(PyGILState_Check());
  auto py_full_name = ResolveAttrs(py_proto, {"DESCRIPTOR", "full_name"});
  if (py_full_name) {
    pybind11::detail::make_caster<std::string> c;
    if (c.load(*py_full_name, false)) {
      return static_cast<std::string>(c);
    }
  }
  return absl::nullopt;
}

bool PyProtoCopyToCProto(py::handle py_proto, ::google::protobuf::Message* message) {
  assert(PyGILState_Check());
  py::object wire = py_proto.attr("SerializePartialToString")();
  const char* bytes = PYBIND11_BYTES_AS_STRING(wire.ptr());
  if (!bytes) {
    throw py::type_error("Object.SerializePartialToString failed; is this a " +
                         message->GetDescriptor()->full_name());
  }
  return message->ParsePartialFromArray(bytes, PYBIND11_BYTES_SIZE(wire.ptr()));
}

std::unique_ptr<::google::protobuf::Message> AllocateCProtoFromPythonSymbolDatabase(
    py::handle src, const std::string& full_name) {
  assert(PyGILState_Check());
  auto pool = ResolveAttrs(src, {"DESCRIPTOR", "file", "pool"});
  if (!pool) {
    throw py::type_error("Object is not a valid protobuf");
  }

  auto pool_data =
      PythonDescriptorPoolWrapper::instance()->GetPoolFromPythonPool(*pool);
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

namespace {

std::string ReturnValuePolicyName(py::return_value_policy policy) {
  switch (policy) {
    case py::return_value_policy::automatic:
      return "automatic";
    case py::return_value_policy::automatic_reference:
      return "automatic_reference";
    case py::return_value_policy::take_ownership:
      return "take_ownership";
    case py::return_value_policy::copy:
      return "copy";
    case py::return_value_policy::move:
      return "move";
    case py::return_value_policy::reference:
      return "reference";
    case py::return_value_policy::reference_internal:
      return "reference_internal";
    default:
      return "INVALID_ENUM_VALUE";
  }
}

}  // namespace

py::handle GenericPyProtoCast(::google::protobuf::Message* src,
                              py::return_value_policy policy, py::handle parent,
                              bool is_const) {
  assert(src != nullptr);
  assert(PyGILState_Check());
  auto py_proto =
      GlobalState::instance()->PyMessageInstance(src->GetDescriptor());

  auto serialized = src->SerializePartialAsString();
#if PY_MAJOR_VERSION >= 3
  auto view = py::memoryview::from_memory(serialized.data(), serialized.size());
#else
  py::bytearray view(serialized);
#endif
  py_proto.attr("MergeFromString")(view);
  return py_proto.release();
}

py::handle GenericFastCppProtoCast(::google::protobuf::Message* src,
                                   py::return_value_policy policy,
                                   py::handle parent, bool is_const) {
  assert(src != nullptr);
  assert(PyGILState_Check());
  switch (policy) {
    case py::return_value_policy::copy: {
      std::pair<py::object, ::google::protobuf::Message*> descriptor_pair =
          GlobalState::instance()->PyFastCppProtoMessageInstance(
              src->GetDescriptor());
      py::object& result = descriptor_pair.first;
      ::google::protobuf::Message* result_message = descriptor_pair.second;

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
      py::object result = py::reinterpret_steal<py::object>(
          GlobalState::instance()->py_proto_api()->NewMessageOwnedExternally(
              src, nullptr));
      if (policy == py::return_value_policy::reference_internal) {
        py::detail::keep_alive_impl(result, parent);
      }
      return result.release();
    } break;

    default:
      std::string message("pybind11_protobuf unhandled return_value_policy::");
      throw py::cast_error(message + ReturnValuePolicyName(policy));
  }
}

py::handle GenericProtoCast(::google::protobuf::Message* src,
                            py::return_value_policy policy, py::handle parent,
                            bool is_const) {
  assert(src != nullptr);
  assert(PyGILState_Check());
  if (src->GetDescriptor()->file()->pool() ==
          ::google::protobuf::DescriptorPool::generated_pool() &&
      !GlobalState::instance()->using_fast_cpp()) {
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

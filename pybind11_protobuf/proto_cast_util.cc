#include "pybind11_protobuf/proto_cast_util.h"

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/types/optional.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/dynamic_message.h"

namespace py = pybind11;

using ::google::protobuf::Descriptor;
using ::google::protobuf::DescriptorDatabase;
using ::google::protobuf::DescriptorPool;
using ::google::protobuf::DynamicMessageFactory;
using ::google::protobuf::FileDescriptor;
using ::google::protobuf::FileDescriptorProto;
using ::google::protobuf::Message;
using ::google::protobuf::MessageFactory;

namespace pybind11_protobuf {

std::string StripProtoSuffixFromDescriptorFileName(absl::string_view filename) {
  if (absl::EndsWith(filename, ".protodevel")) {
    return std::string(absl::StripSuffix(filename, ".protodevel"));
  } else {
    return std::string(absl::StripSuffix(filename, ".proto"));
  }
}

std::string InferPythonModuleNameFromDescriptorFileName(
    absl::string_view filename) {
  std::string basename = StripProtoSuffixFromDescriptorFileName(filename);
  absl::StrReplaceAll({{"-", "_"}, {"/", "."}}, &basename);
  return absl::StrCat(basename, "_pb2");
}

namespace {

// Resolves the class name of a descriptor via d->containing_type()
py::object ResolveDescriptor(py::object p, const Descriptor* d) {
  return d->containing_type() ? ResolveDescriptor(p, d->containing_type())
                                    .attr(d->name().c_str())
                              : p.attr(d->name().c_str());
}

// Returns true if an exception is an import error.
bool IsImportError(py::error_already_set& e) {
#if ((PY_MAJOR_VERSION > 3) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 6))
  return e.matches(PyExc_ImportError) || e.matches(PyExc_ModuleNotFoundError);
#else
  return e.matches(PyExc_ImportError);
#endif
}

// Resolves a sequence of python attrs starting from obj.
// If any does not exist, returns nullopt.
absl::optional<py::object> ResolveAttrs(
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

// Resolves a single attribute using the python MRO (method resolution order).
// Mimics PyObject_GetAttrString.
//
// Unfortunately the metaclass mechanism used by protos (fast_cpp_protos) does
// not leave __dict__ in a state where the default getattr functions find the
// base class methods, so we resolve those using MRO.
absl::optional<py::object> ResolveAttrMRO(py::handle obj, const char* name) {
  PyObject* attr;
  const auto* t = Py_TYPE(obj.ptr());
  if (!t->tp_mro) {
    PyObject* attr = PyObject_GetAttrString(obj.ptr(), name);
    if (attr) {
      return py::reinterpret_steal<py::object>(attr);
    }
    PyErr_Clear();
    return absl::nullopt;
  }

  auto unicode = py::reinterpret_steal<py::object>(PyUnicode_FromString(name));
  auto bases = py::reinterpret_borrow<py::tuple>(t->tp_mro);
  for (py::handle h : bases) {
    auto base = reinterpret_cast<PyTypeObject*>(h.ptr());
    if (base->tp_getattr) {
      attr = (*base->tp_getattr)(obj.ptr(), const_cast<char*>(name));
      if (attr) {
        return py::reinterpret_steal<py::object>(attr);
      }
      PyErr_Clear();
    }
    if (base->tp_getattro) {
      attr = (*base->tp_getattro)(obj.ptr(), unicode.ptr());
      if (attr) {
        return py::reinterpret_steal<py::object>(attr);
      }
      PyErr_Clear();
    }
  }
  return absl::nullopt;
}

absl::optional<std::string> CastToOptionalString(py::handle src) {
  // Avoid pybind11::cast because it throws an exeption.
  pybind11::detail::make_caster<std::string> c;
  if (c.load(src, false)) {
    return pybind11::detail::cast_op<std::string>(std::move(c));
  }
  return absl::nullopt;
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

  // Allocate a python proto message instance using the native python
  // allocations.
  py::object PyMessageInstance(const Descriptor* descriptor);

  // Allocates a fast cpp proto python object, also returning
  // the embedded c++ proto2 message type. The returned message
  // pointer cannot be null.
  std::pair<py::object, Message*> PyFastCppProtoMessageInstance(
      const Descriptor* descriptor);

  // Import (and cache) a python module.
  py::module_ ImportCached(const std::string& module_name);

 private:
  GlobalState();

  py::object global_pool_;
  py::object factory_;
  py::object find_message_type_by_name_;
  py::object get_prototype_;
  py::object get_message_class_;

  absl::flat_hash_map<std::string, py::module_> import_cache_;
};

GlobalState::GlobalState() {
  assert(PyGILState_Check());

  // pybind11_protobuf casting needs a dependency on proto internals to work.
  try {
    ImportCached("google.protobuf.descriptor");
    auto descriptor_pool =
        ImportCached("google.protobuf.descriptor_pool");
    auto message_factory =
        ImportCached("google.protobuf.message_factory");
    global_pool_ = descriptor_pool.attr("Default")();
    find_message_type_by_name_ = global_pool_.attr("FindMessageTypeByName");
    if (hasattr(message_factory, "GetMessageClass")) {
      get_message_class_ = message_factory.attr("GetMessageClass");
    } else {
      // TODO(pybind11-infra): Cleanup `MessageFactory.GetProtoType` after it
      // is deprecated. See b/258832141.
      factory_ = message_factory.attr("MessageFactory")(global_pool_);
      get_prototype_ = factory_.attr("GetPrototype");
    }
  } catch (py::error_already_set& e) {
    if (IsImportError(e)) {
      std::cerr << "Add a python dependency on "
                   "\"@com_google_protobuf//:protobuf_python\"" << std::endl;
    }

    // TODO(pybind11-infra): narrow down to expected exception(s).
    e.restore();
    PyErr_Print();

    global_pool_ = {};
    factory_ = {};
    find_message_type_by_name_ = {};
    get_prototype_ = {};
    get_message_class_ = {};
  }
}

py::module_ GlobalState::ImportCached(const std::string& module_name) {
  auto cached = import_cache_.find(module_name);
  if (cached != import_cache_.end()) {
    return cached->second;
  }
  auto module = py::module_::import(module_name.c_str());
  import_cache_[module_name] = module;
  return module;
}

py::object GlobalState::PyMessageInstance(const Descriptor* descriptor) {
  auto module_name =
      InferPythonModuleNameFromDescriptorFileName(descriptor->file()->name());
  if (!module_name.empty()) {
    auto cached = import_cache_.find(module_name);
    if (cached != import_cache_.end()) {
      return ResolveDescriptor(cached->second, descriptor)();
    }
  }

  // First attempt to construct the proto from the global pool.
  if (global_pool_) {
    try {
      auto d = find_message_type_by_name_(descriptor->full_name());
      py::object p;
      if (get_message_class_) {
        p = get_message_class_(d);
      } else {
        // TODO(pybind11-infra): Cleanup `MessageFactory.GetProtoType` after it
        // is deprecated. See b/258832141.
        p = get_prototype_(d);
      }
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
    std::unique_ptr<DescriptorDatabase> database;
    std::unique_ptr<const DescriptorPool> pool;
    std::unique_ptr<MessageFactory> factory;
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

    // An attempt at cleanup could be made by using a py::weakref to the
    // underlying python pool, and removing the map entry when the pool
    // disappears, that is fundamentally unsafe because (1) a cloned c++ object
    // may outlive the python pool, and (2) for the fast_cpp_proto case, there's
    // no support for weak references.

    auto database = absl::make_unique<DescriptorPoolDatabase>(
        py::reinterpret_borrow<py::object>(python_pool));
    auto pool = absl::make_unique<DescriptorPool>(database.get());
    auto factory = absl::make_unique<DynamicMessageFactory>(pool.get());
    // When wrapping the Python descriptor_poool.Default(), apply an important
    // optimization:
    // - the pool is based on the C++ generated_pool(), so that compiled
    //   C++ modules can be found without using the DescriptorDatabase and
    //   the Python DescriptorPool.
    // - the MessageFactory returns instances of C++ compiled messages when
    //   possible: some methods are much more optimized, and the created
    //   Message can be cast to the C++ class.  We use this last property in
    //   the proto_caster class.
    // This is done only for the Default pool, because generated C++ modules
    // and generated Python modules are built from the same .proto sources.
    if (python_pool.is(GlobalState::instance()->global_pool())) {
      pool->internal_set_underlay(DescriptorPool::generated_pool());
      factory->SetDelegateToGeneratedFactory(true);
    }

    // Cache the created objects.
    pool_entry = Data{std::move(database), std::move(pool), std::move(factory)};
    return &pool_entry;
  }

 private:
  PythonDescriptorPoolWrapper() = default;

  // Similar to DescriptorPoolDatabase: wraps a Python DescriptorPool
  // as a DescriptorDatabase.
  class DescriptorPoolDatabase : public DescriptorDatabase {
   public:
    DescriptorPoolDatabase(py::object python_pool)
        : pool_(std::move(python_pool)) {}

    // These 3 methods implement DescriptorDatabase and delegate to
    // the Python DescriptorPool.

    // Find a file by file name.
    bool FindFileByName(const std::string& filename,
                        FileDescriptorProto* output) override {
      try {
        auto file = pool_.attr("FindFileByName")(filename);
        return CopyToFileDescriptorProto(file, output);
      } catch (py::error_already_set& e) {
        std::cerr << "FindFileByName " << filename << " raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }
      return false;
    }

    // Find the file that declares the given fully-qualified symbol name.
    bool FindFileContainingSymbol(const std::string& symbol_name,
                                  FileDescriptorProto* output) override {
      try {
        auto file = pool_.attr("FindFileContainingSymbol")(symbol_name);
        return CopyToFileDescriptorProto(file, output);
      } catch (py::error_already_set& e) {
        std::cerr << "FindFileContainingSymbol " << symbol_name
                   << " raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }
      return false;
    }

    // Find the file which defines an extension extending the given message type
    // with the given field number.
    bool FindFileContainingExtension(const std::string& containing_type,
                                     int field_number,
                                     FileDescriptorProto* output) override {
      try {
        auto descriptor = pool_.attr("FindMessageTypeByName")(containing_type);
        auto file =
            pool_.attr("FindExtensionByNumber")(descriptor, field_number)
                .attr("file");
        return CopyToFileDescriptorProto(file, output);
      } catch (py::error_already_set& e) {
        std::cerr << "FindFileContainingExtension " << containing_type << " "
                   << field_number << " raised an error";

        // This prints and clears the error.
        e.restore();
        PyErr_Print();
      }
      return false;
    }

   private:
    bool CopyToFileDescriptorProto(py::handle py_file_descriptor,
                                   FileDescriptorProto* output) {

      return output->ParsePartialFromString(
          PyBytesAsStringView(py_file_descriptor.attr("serialized_pb")));
    }

    py::object pool_;  // never dereferenced.
  };

  // This map caches the wrapped objects, indexed by DescriptorPool address.
  absl::flat_hash_map<PyObject*, Data> pools_map;
};

}  // namespace

absl::string_view PyBytesAsStringView(py::bytes py_bytes) {
  return absl::string_view(PyBytes_AsString(py_bytes.ptr()),
                           PyBytes_Size(py_bytes.ptr()));
}

void InitializePybindProtoCastUtil() {
  assert(PyGILState_Check());
  GlobalState::instance();
}

void ImportProtoDescriptorModule(const Descriptor* descriptor) {
  assert(PyGILState_Check());
  if (!descriptor) return;
  auto module_name =
      InferPythonModuleNameFromDescriptorFileName(descriptor->file()->name());
  if (module_name.empty()) return;
  try {
    GlobalState::instance()->ImportCached(module_name);
  } catch (py::error_already_set& e) {
    if (IsImportError(e)) {
      std::cerr << "Python module " << module_name << " unavailable."
                 << std::endl;
    } else {
      std::cerr << "ImportDescriptorModule raised an error";
      // This prints and clears the error.
      e.restore();
      PyErr_Print();
    }
  }
}

const Message* PyProtoGetCppMessagePointer(py::handle src) {
#if !defined(PYBIND11_PROTOBUF_ASSUME_FULL_ABI_COMPATIBILITY)
  return nullptr;
#else
  // Assume that C++ proto objects are compatible as long as there is a C++
  // message pointer.
  assert(PyGILState_Check());
  if (!GlobalState::instance()->py_proto_api()) return nullptr;
  auto* ptr =
      GlobalState::instance()->py_proto_api()->GetMessagePointer(src.ptr());
  if (ptr == nullptr) {
    // Clear the type_error set by GetMessagePointer sets a type_error when
    // src was not a wrapped C++ proto message.
    PyErr_Clear();
  }
  return ptr;
#endif
}

absl::optional<std::string> PyProtoDescriptorFullName(py::handle py_proto) {
  assert(PyGILState_Check());
  auto py_full_name = ResolveAttrs(py_proto, {"DESCRIPTOR", "full_name"});
  if (py_full_name) {
    return CastToOptionalString(*py_full_name);
  }
  return absl::nullopt;
}

bool PyProtoHasMatchingFullName(py::handle py_proto,
                                const Descriptor* descriptor) {
  auto full_name = PyProtoDescriptorFullName(py_proto);
  return full_name && *full_name == descriptor->full_name();
}

py::bytes PyProtoSerializePartialToString(py::handle py_proto,
                                          bool raise_if_error) {
  static const char* serialize_fn_name = "SerializePartialToString";
  auto serialize_fn = ResolveAttrMRO(py_proto, serialize_fn_name);
  if (!serialize_fn) {
    return py::object();
  }
  auto serialized_bytes = py::reinterpret_steal<py::object>(
      PyObject_CallObject(serialize_fn->ptr(), nullptr));
  if (!serialized_bytes) {
    if (raise_if_error) {
      std::string msg = py::repr(py_proto).cast<std::string>() + "." +
                        serialize_fn_name + "() function call FAILED";
      py::raise_from(PyExc_TypeError, msg.c_str());
      throw py::error_already_set();
    }
    return py::object();
  }
  if (!PyBytes_Check(serialized_bytes.ptr())) {
    if (raise_if_error) {
      std::string msg = py::repr(py_proto).cast<std::string>() + "." +
                        serialize_fn_name +
                        "() function call is expected to return bytes, but the "
                        "returned value is " +
                        py::repr(serialized_bytes).cast<std::string>();
      throw py::type_error(msg);
    }
    return py::object();
  }
  return serialized_bytes;
}

void CProtoCopyToPyProto(Message* message, py::handle py_proto) {
  assert(PyGILState_Check());
  auto merge_fn = ResolveAttrMRO(py_proto, "MergeFromString");
  if (!merge_fn) {
    throw py::type_error("MergeFromString method not found; is this a " +
                         message->GetDescriptor()->full_name());
  }

  auto serialized = message->SerializePartialAsString();
#if PY_MAJOR_VERSION >= 3
  auto view = py::memoryview::from_memory(serialized.data(), serialized.size());
#else
  py::bytearray view(serialized);
#endif
  (*merge_fn)(view);
}

std::unique_ptr<Message> AllocateCProtoFromPythonSymbolDatabase(
    py::handle src, const std::string& full_name) {
  assert(PyGILState_Check());
  auto pool = ResolveAttrs(src, {"DESCRIPTOR", "file", "pool"});
  if (!pool) {
    throw py::type_error(py::repr(src).cast<std::string>() +
                         " object is not a valid protobuf");
  }

  auto pool_data =
      PythonDescriptorPoolWrapper::instance()->GetPoolFromPythonPool(*pool);
  // The following call will query the DescriptorDatabase, which fetches the
  // necessary Python descriptors and feeds them into the C++ pool.
  // The result stays cached as long as the Python pool stays alive.
  const Descriptor* descriptor =
      pool_data->pool->FindMessageTypeByName(full_name);
  if (!descriptor) {
    throw py::type_error("Could not find descriptor: " + full_name);
  }
  const Message* prototype = pool_data->factory->GetPrototype(descriptor);
  if (!prototype) {
    throw py::type_error("Unable to get prototype for " + full_name);
  }
  return std::unique_ptr<Message>(prototype->New());
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
#if defined(PYBIND11_HAS_RETURN_VALUE_POLICY_RETURN_AS_BYTES)
    case py::return_value_policy::_return_as_bytes:
      return "_return_as_bytes";
#endif
#if defined(PYBIND11_HAS_RETURN_VALUE_POLICY_CLIF_AUTOMATIC)
    case py::return_value_policy::_clif_automatic:
      return "_clif_automatic";
#endif
    default:
      return "INVALID_ENUM_VALUE";
  }
}

}  // namespace

py::handle GenericPyProtoCast(Message* src, py::return_value_policy policy,
                              py::handle parent, bool is_const) {
  assert(src != nullptr);
  assert(PyGILState_Check());
  auto py_proto =
      GlobalState::instance()->PyMessageInstance(src->GetDescriptor());

  CProtoCopyToPyProto(src, py_proto);
  return py_proto.release();
}

py::handle GenericProtoCast(Message* src, py::return_value_policy policy,
                            py::handle parent, bool is_const) {
  assert(src != nullptr);
  assert(PyGILState_Check());

  // Return a native python-allocated proto when:
  // 1. The binary does not have a py_proto_api instance, or
  // 2. a) the proto is from the default pool and
  //    b) the binary is not using fast_cpp_protos.
    return GenericPyProtoCast(src, policy, parent, is_const);
}

}  // namespace pybind11_protobuf

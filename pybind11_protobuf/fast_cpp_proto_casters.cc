#include "pybind11_protobuf/fast_cpp_proto_casters.h"

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <optional>
#include <stdexcept>
#include <string>

#include "glog/logging.h"
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

namespace pybind11_protobuf::fast_cpp_protos {

const ::google::protobuf::python::PyProto_API *GetPyProtoApi() {
  static auto *result = [] {
    auto *r = static_cast<const ::google::protobuf::python::PyProto_API *>(
        PyCapsule_Import(::google::protobuf::python::PyProtoAPICapsuleName(), 0));
    CHECK_NE(r, nullptr) << "Failed to obtain Python protobuf C++ API. "
                            "Did you link against "
                            "//net/proto2/python/public:use_fast_cpp_protos? "
                            "See http://go/fastpythonproto";
    return r;
  }();

  return result;
}

const ::google::protobuf::Message *GetCppProtoMessagePointer(const py::handle &src) {
  return GetPyProtoApi()->GetMessagePointer(src.ptr());
}

std::string PyObjectTypeString(PyObject *obj) {
  if (obj == nullptr) {
    return "<nullptr>";
  }
  std::string type_str = "<unknown>";
  auto *type = PyObject_Type(obj);
  if (type != nullptr) {
    auto *repr = PyObject_Repr(type);
    if (repr != nullptr) {
      type_str = py::str(py::handle(repr));
      Py_DECREF(repr);
    }
    Py_DECREF(type);
  }
  return type_str;
}

std::pair<py::object, ::google::protobuf::Message *> cpp_to_py_impl::allocate(
    const ::google::protobuf::Descriptor *src_descriptor) {
  CHECK(src_descriptor != nullptr);
  const auto *py_proto_api = fast_cpp_protos::GetPyProtoApi();
  auto *descriptor =
      py_proto_api->GetDefaultDescriptorPool()->FindMessageTypeByName(
          src_descriptor->full_name());
  py::object result = py::reinterpret_steal<py::object>(
      py_proto_api->NewMessage(descriptor, nullptr));
  if (result.ptr() == nullptr) {
    throw py::error_already_set();
  }
  ::google::protobuf::Message *message =
      py_proto_api->GetMutableMessagePointer(result.ptr());
  return {std::move(result), message};
}

py::object cpp_to_py_impl::reference(::google::protobuf::Message *src) {
  const auto *py_proto_api = fast_cpp_protos::GetPyProtoApi();
  if (!py_proto_api) {
    throw py::type_error(
        "Policy reference is only allowed for uses of "
        "//net/proto2/python/public:use_fast_cpp_protos");
  }
  return py::reinterpret_steal<py::object>(
      py_proto_api->NewMessageOwnedExternally(src, nullptr));
}

}  // namespace pybind11_protobuf::fast_cpp_protos

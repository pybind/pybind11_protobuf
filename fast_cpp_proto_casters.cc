#include "pybind11_protobuf/fast_cpp_proto_casters.h"

#include <Python.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <stdexcept>
#include <string>

#include "base/logging.h"
#include "net/proto2/python/public/proto_api.h"


void pybind11_proto_casters_collision() {
  // This symbol intentionally defined in both fast_cpp_proto_casters.cc
  // and proto_utils.cc to detect potential ODR violations in libraries.

  // Avoid mixing fast_cpp_proto_casters.h and proto_casters.h in the same
  // build target; this violates the ODR rule for type_caster<proto2::Message>
  // as well as other potential types, and can lead to hard to diagnose bugs,
  // crashes, and other mysterious bad behavior.

  // See https://github.com/pybind/pybind11/issues/2695 for more details.
}

namespace pybind11_protobuf::fast_cpp_protos {

using pybind11::error_already_set;
using pybind11::handle;
using pybind11::object;
using pybind11::reinterpret_steal;
using pybind11::str;

const proto2::python::PyProto_API &GetPyProtoApi() {
  static auto *result = [] {
    auto *r = static_cast<const ::proto2::python::PyProto_API *>(
        PyCapsule_Import(::proto2::python::PyProtoAPICapsuleName(), 0));
    CHECK_NE(r, nullptr) << "Failed to obtain Python protobuf C++ API. "
                            "Did you link against "
                            "//net/proto2/python/public:use_fast_cpp_protos? "
                            "See http://go/fastpythonproto";
    return r;
  }();

  return *result;
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
      type_str = str(handle(repr));
      Py_DECREF(repr);
    }
    Py_DECREF(type);
  }
  return type_str;
}

std::pair<object, proto2::Message *> cpp_to_py_impl::allocate(
    const proto2::Descriptor *src_descriptor) {
  const auto &py_proto_api = fast_cpp_protos::GetPyProtoApi();
  CHECK(src_descriptor != nullptr);
  auto *descriptor =
      py_proto_api.GetDefaultDescriptorPool()->FindMessageTypeByName(
          src_descriptor->full_name());
  object result =
      reinterpret_steal<object>(py_proto_api.NewMessage(descriptor, nullptr));
  if (result.ptr() == nullptr) {
    throw error_already_set();
  }
  proto2::Message *message =
      py_proto_api.GetMutableMessagePointer(result.ptr());
  return {std::move(result), message};
}

object cpp_to_py_impl::reference(proto2::Message *src) {
  const auto &py_proto_api = fast_cpp_protos::GetPyProtoApi();
  return reinterpret_steal<object>(
      py_proto_api.NewMessageOwnedExternally(src, nullptr));
}

object cpp_to_py_impl::copy(const proto2::Message &src) {
  auto [result, result_message] = allocate(src.GetDescriptor());
  result_message->CopyFrom(src);
  return result;
}

}  // namespace pybind11_protobuf::fast_cpp_protos

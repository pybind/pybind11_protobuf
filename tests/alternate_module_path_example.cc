#include <pybind11/pybind11.h>

#include "pybind11_protobuf/proto_casters.h"
#include "pybind11_protobuf/tests/test.proto.h"

namespace pybind11 {
namespace test {

PYBIND11_MODULE(alternate_module_path_example, m) {
  google::ImportProtoModule();

  m.def("make_int_message", []() { return IntMessage(); });
}

}  // namespace test
}  // namespace pybind11

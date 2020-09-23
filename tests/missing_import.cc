// A pybind11 module which uses the proto caster but does not call
// ImportProtoModule (as it should), to test the behavior in that case.
#include <pybind11/pybind11.h>

#include "pybind11_protobuf/proto_casters.h"
#include "pybind11_protobuf/tests/test.proto.h"

namespace pybind11 {
namespace test {

IntMessage ReturnsProto() { return IntMessage(); }
void TakesProto(const IntMessage&) {}

PYBIND11_MODULE(missing_import, m) {
  m.def("returns_proto", &ReturnsProto);
  m.def("takes_proto", &TakesProto);
}

}  // namespace test
}  // namespace pybind11

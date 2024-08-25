/*
The code below compiled before https://github.com/pybind/pybind11/pull/5257
was merged into pybind11k, but with `PYBIND11_SMART_HOLDER_TYPE_CASTERS`
becoming a no-op, this error appears:

.../pybind11/include/pybind11/detail/../cast.h:72:30: error: no member named
'operator RawMessage *' in 'pybind11::detail::type_caster<RawMessage>' 72 |
return std::move(caster).operator result_t(); |            ~~~~~~~~~~~~~~~~~ ^
*/

#include <pybind11/smart_holder.h>

#include "net/proto/rawmessage.h"
#include "pybind11_protobuf/native_proto_caster.h"

PYBIND11_SMART_HOLDER_TYPE_CASTERS(::RawMessage)

namespace py = pybind11;

PYBIND11_MODULE(rawmessage, m) {
  py::classh<::RawMessage> w_t(m, "RawMessage", py::is_final());
  w_t.def(py::init([]() { return std::make_unique<::RawMessage>().release(); }),
          py::return_value_policy::_clif_automatic);
  w_t.def("as_proto2_Message", [](::RawMessage* self) {
    return py::capsule(static_cast<void*>(self));
  });
}

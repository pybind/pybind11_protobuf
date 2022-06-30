// Minimal fake/stub to be used as a means to globally switch all dependents
// from proto_casters.h to native_proto_caster.h (b/229395875).

#ifndef PYBIND11_PROTOBUF_PROTO_CASTERS_H_
#define PYBIND11_PROTOBUF_PROTO_CASTERS_H_

// Satisfy existing transitive dependencies. To be cleaned up along with
// replacing the proto_casters.h includes.
#include <pybind11/functional.h>
#include <pybind11/stl.h>

// Pull in the new caster.
#include "pybind11_protobuf/native_proto_caster.h"

namespace pybind11 {
namespace google {

inline void ImportProtoModule() {
  pybind11_protobuf::ImportNativeProtoCasters();
}

template <typename ProtoType>
void RegisterProtoMessageType(module_) {
  // NOOP
}

}  // namespace google
}  // namespace pybind11

#endif  // PYBIND11_PROTOBUF_PROTO_CASTERS_H_

#include "pybind11_protobuf/check_unknown_fields.h"

namespace pybind11_protobuf::check_unknown_fields {
namespace {

static int kSetConfigDone = []() {
  ExtensionsWithUnknownFieldsPolicy::StrongSetDisallow();
  return 0;
}();

}  // namespace
}  // namespace pybind11_protobuf::check_unknown_fields

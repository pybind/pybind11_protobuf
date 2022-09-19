#ifndef PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_
#define PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_


#include "google/protobuf/message.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace pybind11_protobuf {
namespace check_unknown_fields {

void AllowUnknownFieldsFor(absl::string_view top_message_descriptor_full_name,
                           absl::string_view unknown_field_parent_message_fqn);

absl::optional<std::string> CheckAndBuildErrorMessageIfAny(
    const ::google::protobuf::Message* top_message);

}  // namespace check_unknown_fields
}  // namespace pybind11_protobuf

#endif  // PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_

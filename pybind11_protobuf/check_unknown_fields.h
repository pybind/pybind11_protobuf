#ifndef PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_
#define PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_

#include <optional>

#include "google/protobuf/message.h"
#include "absl/strings/string_view.h"

namespace pybind11_protobuf::check_unknown_fields {

void AllowUnknownFieldsFor(absl::string_view top_message_descriptor_full_name,
                           absl::string_view unknown_field_parent_message_fqn);

std::optional<std::string> CheckAndBuildErrorMessageIfAny(
    const ::google::protobuf::Message* top_message);

}  // namespace pybind11_protobuf::check_unknown_fields

#endif  // PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_

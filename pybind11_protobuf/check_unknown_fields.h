#ifndef PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_
#define PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_

#include <optional>

#include "google/protobuf/message.h"
#include "python/google/protobuf/proto_api.h"
#include "absl/strings/string_view.h"

namespace pybind11_protobuf::check_unknown_fields {

class ExtensionsWithUnknownFieldsPolicy {
  enum State {
    // Initial state.
    kWeakDisallow,

    // Primary use case: PyCLIF extensions might set this when being imported.
    kWeakEnableFallbackToSerializeParse,

    // Primary use case: `:disallow_extensions_with_unknown_fields` in `deps`
    // of a binary (or test).
    kStrongDisallow
  };

  static State& GetStateSingleton() {
    static State singleton = kWeakDisallow;
    return singleton;
  }

 public:
  static void WeakEnableFallbackToSerializeParse() {
    State& policy = GetStateSingleton();
    if (policy == kWeakDisallow) {
      policy = kWeakEnableFallbackToSerializeParse;
    }
  }

  static void StrongSetDisallow() { GetStateSingleton() = kStrongDisallow; }

  static bool UnknownFieldsAreDisallowed() {
    return GetStateSingleton() != kWeakEnableFallbackToSerializeParse;
  }
};

void AllowUnknownFieldsFor(absl::string_view top_message_descriptor_full_name,
                           absl::string_view unknown_field_parent_message_fqn);

std::optional<std::string> CheckRecursively(
    const ::google::protobuf::python::PyProto_API* py_proto_api,
    const ::google::protobuf::Message* top_message);

}  // namespace pybind11_protobuf::check_unknown_fields

#endif  // PYBIND11_PROTOBUF_CHECK_UNKNOWN_FIELDS_H_

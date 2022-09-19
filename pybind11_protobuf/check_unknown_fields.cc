#include "pybind11_protobuf/check_unknown_fields.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/unknown_field_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/optional.h"

namespace pybind11_protobuf {
namespace check_unknown_fields {
namespace {

using AllowListSet = absl::flat_hash_set<std::string>;
using MayContainExtensionsMap =
    absl::flat_hash_map<const ::google::protobuf::Descriptor*, bool>;

AllowListSet* GetAllowList() {
  static auto* allow_list = new AllowListSet();
  return allow_list;
}

std::string MakeAllowListKey(
    absl::string_view top_message_descriptor_full_name,
    absl::string_view unknown_field_parent_message_fqn) {
  return absl::StrCat(top_message_descriptor_full_name, ":",
                      unknown_field_parent_message_fqn);
}

/// Recurses through the message Descriptor class looking for valid extensions.
/// Stores the result to `memoized`.
bool MessageMayContainExtensionsRecursive(const ::google::protobuf::Descriptor* descriptor,
                                          MayContainExtensionsMap* memoized) {
  if (descriptor->extension_range_count() > 0) return true;

  auto it_inserted = memoized->try_emplace(descriptor, false);
  if (!it_inserted.second) {
    return it_inserted.first->second;
  }

  for (int i = 0; i < descriptor->field_count(); i++) {
    auto* fd = descriptor->field(i);
    if (fd->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) continue;
    if (MessageMayContainExtensionsRecursive(fd->message_type(), memoized)) {
      (*memoized)[descriptor] = true;
      return true;
    }
  }

  return false;
}

bool MessageMayContainExtensionsMemoized(const ::google::protobuf::Descriptor* descriptor) {
  static auto* memoized = new MayContainExtensionsMap();
  static absl::Mutex lock;
  absl::MutexLock l(&lock);
  return MessageMayContainExtensionsRecursive(descriptor, memoized);
}

struct HasUnknownFields {
  HasUnknownFields(const ::google::protobuf::Descriptor* root_descriptor)
      : root_descriptor(root_descriptor) {}

  std::string FieldFQN() const { return absl::StrJoin(field_fqn_parts, "."); }
  std::string FieldFQNWithFieldNumber() const {
    return field_fqn_parts.empty()
               ? absl::StrCat(unknown_field_number)
               : absl::StrCat(FieldFQN(), ".", unknown_field_number);
  }

  bool FindUnknownFieldsRecursive(const ::google::protobuf::Message* sub_message,
                                  uint32_t depth);

  std::string BuildErrorMessage() const;

  const ::google::protobuf::Descriptor* root_descriptor = nullptr;
  const ::google::protobuf::Descriptor* unknown_field_parent_descriptor = nullptr;
  std::vector<std::string> field_fqn_parts;
  int unknown_field_number;
};

/// Recurses through the message fields class looking for UnknownFields.
bool HasUnknownFields::FindUnknownFieldsRecursive(
    const ::google::protobuf::Message* sub_message, uint32_t depth) {
  const ::google::protobuf::Reflection& reflection = *sub_message->GetReflection();

  // If there are unknown fields, stop searching.
  const ::google::protobuf::UnknownFieldSet& unknown_field_set =
      reflection.GetUnknownFields(*sub_message);
  if (!unknown_field_set.empty()) {
    unknown_field_parent_descriptor = sub_message->GetDescriptor();
    field_fqn_parts.resize(depth);
    unknown_field_number = unknown_field_set.field(0).number();
    return true;
  }

  // If this message does not include submessages which allow extensions,
  // then it cannot include unknown fields.
  if (!MessageMayContainExtensionsMemoized(sub_message->GetDescriptor())) {
    return false;
  }

  // Otherwise the method has to check all present fields, including
  // extensions to determine if they include unknown fields.
  std::vector<const ::google::protobuf::FieldDescriptor*> present_fields;
  reflection.ListFields(*sub_message, &present_fields);

  for (const auto* field : present_fields) {
    if (field->cpp_type() != ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      continue;
    }
    if (field->is_repeated()) {
      int field_size = reflection.FieldSize(*sub_message, field);
      for (int i = 0; i != field_size; ++i) {
        if (FindUnknownFieldsRecursive(
                &reflection.GetRepeatedMessage(*sub_message, field, i),
                depth + 1U)) {
          field_fqn_parts[depth] = field->name();
          return true;
        }
      }
    } else if (FindUnknownFieldsRecursive(
                   &reflection.GetMessage(*sub_message, field), depth + 1U)) {
      field_fqn_parts[depth] = field->name();
      return true;
    }
  }

  return false;
}

std::string HasUnknownFields::BuildErrorMessage() const {
  assert(unknown_field_parent_descriptor != nullptr);
  assert(root_descriptor != nullptr);

  std::string emsg = absl::StrCat(  //
      "Proto Message of type ", root_descriptor->full_name(),
      " has an Unknown Field");
  if (root_descriptor != unknown_field_parent_descriptor) {
    absl::StrAppend(&emsg, " with parent of type ",
                    unknown_field_parent_descriptor->full_name());
  }
  absl::StrAppend(&emsg, ": ", FieldFQNWithFieldNumber(), " (",
                  root_descriptor->file()->name());
  if (root_descriptor->file() != unknown_field_parent_descriptor->file()) {
    absl::StrAppend(&emsg, ", ",
                    unknown_field_parent_descriptor->file()->name());
  }
  absl::StrAppend(
      &emsg,
      "). Please add the required `cc_proto_library` `deps`. "
      "Only if there is no alternative to suppressing this error, use "
      "`pybind11_protobuf::AllowUnknownFieldsFor(\"",
      root_descriptor->full_name(), "\", \"", FieldFQN(),
      "\");` (Warning: suppressions may mask critical bugs.)");

  return emsg;
}

}  // namespace

void AllowUnknownFieldsFor(absl::string_view top_message_descriptor_full_name,
                           absl::string_view unknown_field_parent_message_fqn) {
  GetAllowList()->insert(MakeAllowListKey(top_message_descriptor_full_name,
                                          unknown_field_parent_message_fqn));
}

absl::optional<std::string> CheckAndBuildErrorMessageIfAny(
    const ::google::protobuf::Message* message) {
  const auto* root_descriptor = message->GetDescriptor();
  HasUnknownFields search{root_descriptor};
  if (!search.FindUnknownFieldsRecursive(message, 0u)) {
    return absl::nullopt;
  }
  if (GetAllowList()->count(MakeAllowListKey(root_descriptor->full_name(),
                                             search.FieldFQN())) != 0) {
    return absl::nullopt;
  }
  return search.BuildErrorMessage();
}

}  // namespace check_unknown_fields
}  // namespace pybind11_protobuf

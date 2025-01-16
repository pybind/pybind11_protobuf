# Pybind11 bindings for Google's Protocol Buffers

load("@pybind11_bazel//:build_defs.bzl", "pybind_library")

licenses(["notice"])

pybind_library(
    name = "enum_type_caster",
    hdrs = ["enum_type_caster.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@com_google_protobuf//:protobuf",
    ],
)

pybind_library(
    name = "native_proto_caster",
    hdrs = ["native_proto_caster.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":check_unknown_fields",
        ":enum_type_caster",
        ":proto_cast_util",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_protobuf//:protobuf",
    ],
)

pybind_library(
    name = "proto_cast_util",
    srcs = ["proto_cast_util.cc"],
    hdrs = [
        "proto_cast_util.h",
        "proto_caster_impl.h",
    ],
    deps = [
        ":check_unknown_fields",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//python:proto_api",
    ],
)

pybind_library(
    name = "wrapped_proto_caster",
    hdrs = ["wrapped_proto_caster.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":proto_cast_util",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "check_unknown_fields",
    srcs = ["check_unknown_fields.cc"],
    hdrs = ["check_unknown_fields.h"],
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//python:proto_api",
    ],
)

cc_library(
    name = "disallow_extensions_with_unknown_fields",
    srcs = ["disallow_extensions_with_unknown_fields.cc"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":check_unknown_fields",
    ],
    alwayslink = 1,
)

# Pybind11 bindings for Google's Protocol Buffers

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
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
        "@com_google_protobuf//:protobuf",
    ],
)

# To enable PyProto_API use, specify:
# bazel build --@//:enable_pyproto_api
bool_flag(
    name = "enable_pyproto_api",
    build_setting_default = False,
)

config_setting(
    name = "enable_pyproto_api_setting",
    flag_values = {
        ":enable_pyproto_api": "True",
    },
    visibility = ["//visibility:private"],
)

pybind_library(
    name = "proto_cast_util",
    srcs = ["proto_cast_util.cc"],
    hdrs = [
        "proto_cast_util.h",
        "proto_caster_impl.h",
    ],
    local_defines = select({
        ":enable_pyproto_api_setting": ["PYBIND11_PROTOBUF_ENABLE_PYPROTO_API"],
        "//conditions:default": [],
    }),
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
    visibility = [
        "//visibility:private",
    ],
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/meta:type_traits",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//python:proto_api",
    ],
)

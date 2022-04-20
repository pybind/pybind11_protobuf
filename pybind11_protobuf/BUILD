# Pybind11 bindings for Google's Protocol Buffers

load("@pybind11_bazel//:build_defs.bzl", "pybind_extension", "pybind_library")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

licenses(["notice"])

exports_files(["LICENSE"])

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
    srcs = ["native_proto_caster.cc"],
    hdrs = ["native_proto_caster.h"],
    defines = [
        # Force a build error when build variants are mixed.
        "PYBIND11_PROTOBUF_MODE=native_protos",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
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
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:proto_api",
        "@com_google_protobuf//:protobuf",
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

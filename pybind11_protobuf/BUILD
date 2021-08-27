# Pybind11 bindings for Google's Protocol Buffers

load("@pybind11_bazel//:build_defs.bzl", "pybind_extension", "pybind_library")

licenses(["notice"])

exports_files(["LICENSE"])

pybind_library(
    name = "proto_utils",
    srcs = ["proto_utils.cc"],
    hdrs = [
        "proto_utils.h",
    ],
    visibility = [
        "//pybind11_protobuf:__subpackages__",
    ],
    deps = [
        "@com_google_protobuf//:protobuf",  # copybara:strip
    ],
)

pybind_extension(
    name = "proto",
    srcs = ["proto.cc"],
    visibility = [
        "//visibility:public",  # Scheuklappen: keep
    ],
    deps = [":proto_utils"],
)

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
        "@com_google_protobuf//:proto_api",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:python/google/protobuf/pyext/_message.so",  # fixdeps: keep
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
        "@com_google_protobuf//:protobuf",
    ],
)

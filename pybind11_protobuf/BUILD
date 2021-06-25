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
    deps = [
        "@com_google_protobuf//:protobuf",  # copybara:strip
    ],
)

pybind_library(
    name = "proto_casters",
    srcs = ["proto_casters.cc"],
    hdrs = ["proto_casters.h"],
    data = select({
        "//devtools/python/blaze:py3_mode": [],
        "//devtools/python/blaze:py2_mode": [":proto"],
    }),
    defines = [
        # Force a build error when build variants are mixed.
        "PYBIND11_PROTOBUF_MODE=pybind11_protos",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":proto_utils",
        "@com_google_protobuf//:protobuf",
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
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:proto_api",
        "@com_google_protobuf//:protobuf",
    ],
)

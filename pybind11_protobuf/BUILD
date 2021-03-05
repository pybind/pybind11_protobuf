# Pybind11 bindings for Google's Protocol Buffers

load("@pybind11_bazel//:build_defs.bzl", "pybind_extension", "pybind_library")

licenses(["notice"])

exports_files(["LICENSE"])

pybind_library(
    name = "proto_utils",
    srcs = ["proto_utils.cc"],
    hdrs = ["proto_utils.h"],
    deps = [
        "@com_google_protobuf//:protobuf",  # copybara:strip
    ],
)

pybind_library(
    name = "proto_casters",
    hdrs = ["proto_casters.h"],
    data = select({
        "//devtools/python/blaze:py3_mode": [],
        "//devtools/python/blaze:py2_mode": [":proto"],
    }),
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
    name = "fast_cpp_proto_casters",
    srcs = ["fast_cpp_proto_casters.cc"],
    hdrs = ["fast_cpp_proto_casters.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@com_google_glog//:glog",
        "@com_google_protobuf//:proto_api",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:use_fast_cpp_protos",
    ],
)

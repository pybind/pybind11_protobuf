# Tests and examples for the pybind11_protobuf package.

load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
# [internal] load cc_proto_library.bzl

licenses(["notice"])

py_library(
    name = "compare",
    srcs = ["compare.py"],
    data = ["@com_google_protobuf//:proto_api"],
    srcs_version = "PY3",
    visibility = ["//visibility:public"],
)

pybind_extension(
    name = "proto_enum_module",
    srcs = ["proto_enum_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:enum_type_caster",
    ],
)

py_test(
    name = "proto_enum_test",
    srcs = ["proto_enum_test.py"],
    data = [":proto_enum_module.so"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
    ],
)

proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
)

cc_proto_library(
    name = "test_cc_proto",
    deps = [":test_proto"],
)

py_proto_library(
    name = "test_py_pb2",
    deps = [":test_proto"],
)

proto_library(
    name = "extension_proto",
    srcs = ["extension.proto"],
    deps = [":test_proto"],
)

cc_proto_library(
    name = "extension_cc_proto",
    deps = [":extension_proto"],
)

py_proto_library(
    name = "extension_py_pb2",
    deps = [":extension_proto"],
)

# Tests for native_proto_caster

pybind_extension(
    name = "dynamic_message_module",
    srcs = ["dynamic_message_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "dynamic_message_test",
    srcs = ["dynamic_message_test.py"],
    data = [
        ":dynamic_message_module.so",
        "@com_google_protobuf//:proto_api",
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":compare",
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)

pybind_extension(
    name = "extension_module",
    srcs = ["extension_module.cc"],
    deps = [
        ":extension_cc_proto",
        ":test_cc_proto",
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "extension_test",
    srcs = ["extension_test.py"],
    data = [
        ":extension_module.so",
        ":proto_enum_module.so",
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":extension_py_pb2",
        ":test_py_pb2",  # fixdeps: keep - Direct dependency needed in open-source version, see https://github.com/grpc/grpc/issues/22811
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)

pybind_extension(
    name = "message_module",
    srcs = ["message_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "message_test",
    srcs = ["message_test.py"],
    data = [
        ":extension_module.so",
        ":message_module.so",
        ":proto_enum_module.so",
        "@com_google_protobuf//:proto_api",
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":compare",
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)

pybind_extension(
    name = "pass_by_module",
    srcs = ["pass_by_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "pass_by_test",
    srcs = ["pass_by_test.py"],
    data = [
        ":pass_by_module.so",
        "@com_google_protobuf//:proto_api",
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)

# tests for wrapped_proto_caster

pybind_extension(
    name = "wrapped_proto_module",
    srcs = ["wrapped_proto_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:wrapped_proto_caster",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "wrapped_proto_module_test",
    srcs = ["wrapped_proto_module_test.py"],
    data = [
        ":wrapped_proto_module.so",
        "@com_google_protobuf//:proto_api",
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":compare",
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
    ],
)

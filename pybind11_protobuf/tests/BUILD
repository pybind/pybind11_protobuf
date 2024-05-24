# Tests and examples for the pybind11_protobuf package.

load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")

# Placeholder: load py_proto_library
# Placeholder: load py_library
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

# [internal] load cc_proto_library.bzl
load("@pypi//:requirements.bzl", "requirement")

licenses(["notice"])

py_library(
    name = "compare",
    srcs = ["compare.py"],
    srcs_version = "PY3",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:protobuf_python"],
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

proto_library(
    name = "extension_nest_repeated_proto",
    srcs = ["extension_nest_repeated.proto"],
    deps = [":extension_proto"],
)

cc_proto_library(
    name = "extension_nest_repeated_cc_proto",
    deps = [":extension_nest_repeated_proto"],
)

py_proto_library(
    name = "extension_nest_repeated_py_pb2",
    deps = [":extension_nest_repeated_proto"],
)

proto_library(
    name = "extension_in_other_file_in_deps_proto",
    srcs = ["extension_in_other_file_in_deps.proto"],
    deps = [":extension_proto"],
)

cc_proto_library(
    name = "extension_in_other_file_in_deps_cc_proto",
    deps = [":extension_in_other_file_in_deps_proto"],
)

py_proto_library(
    name = "extension_in_other_file_in_deps_py_pb2",
    deps = [":extension_in_other_file_in_deps_proto"],
)

proto_library(
    name = "extension_in_other_file_proto",
    srcs = ["extension_in_other_file.proto"],
    deps = [":extension_proto"],
)

# Intentionally omitted (but here as a comment for easy manual validation of expected behavior):
# cc_proto_library(
#     name = "extension_in_other_file_cc_proto",
#     deps = [":extension_in_other_file_proto"],
# )

py_proto_library(
    name = "extension_in_other_file_py_pb2",
    deps = [":extension_in_other_file_proto"],
)

proto_library(
    name = "we-love-dashes_proto",
    srcs = ["we-love-dashes.proto"],
)

cc_proto_library(
    name = "we_love_dashes_cc_proto",
    deps = [":we-love-dashes_proto"],
)

# Tests for enum_type_caster

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
        "@com_google_protobuf//:protobuf_python",
        requirement("absl_py"),
    ],
)

# Tests for native_proto_caster

pybind_extension(
    name = "dynamic_message_module",
    srcs = ["dynamic_message_module.cc"],
    deps = [
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "dynamic_message_test",
    srcs = ["dynamic_message_test.py"],
    data = [":dynamic_message_module.so"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":compare",
        ":test_py_pb2",
        "@com_google_protobuf//:protobuf_python",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
        requirement("absl_py"),
    ],
)

pybind_extension(
    name = "extension_module",
    srcs = ["extension_module.cc"],
    deps = [
        ":extension_cc_proto",
        # Intentionally omitted: ":extension_in_other_file_cc_proto",
        ":extension_in_other_file_in_deps_cc_proto",
        ":extension_nest_repeated_cc_proto",
        ":test_cc_proto",
        "@com_google_protobuf//:protobuf",
        "//pybind11_protobuf:native_proto_caster",
    ],
)

EXTENSION_TEST_DEPS_COMMON = [
    ":extension_in_other_file_in_deps_py_pb2",
    ":extension_in_other_file_py_pb2",
    ":extension_nest_repeated_py_pb2",
    ":extension_py_pb2",
    ":test_py_pb2",  # fixdeps: keep - Direct dependency needed in open-source version, see https://github.com/grpc/grpc/issues/22811
    "@com_google_absl_py//absl/testing:absltest",
    "@com_google_absl_py//absl/testing:parameterized",
]

py_test(
    name = "extension_test",
    srcs = ["extension_test.py"],
    data = [
        ":extension_module.so",
        ":proto_enum_module.so",
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = EXTENSION_TEST_DEPS_COMMON + ["@com_google_protobuf//:protobuf_python"],
)

py_test(
    name = "extension_disallow_unknown_fields_test",
    srcs = ["extension_test.py"],
    data = [
        ":extension_module.so",
        ":proto_enum_module.so",
    ],
    main = "extension_test.py",
    python_version = "PY3",
    srcs_version = "PY3",
    deps = EXTENSION_TEST_DEPS_COMMON + [
        "@com_google_protobuf//:protobuf_python",
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
    ],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":compare",
        ":test_py_pb2",
        "@com_google_protobuf//:protobuf_python",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
        requirement("absl_py"),
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
    data = [":pass_by_module.so"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":test_py_pb2",
        "@com_google_protobuf//:protobuf_python",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
        requirement("absl_py"),
    ],
)

# Externally this is currently only built but not used.
pybind_extension(
    name = "pass_proto2_message_module",
    srcs = ["pass_proto2_message_module.cc"],
    deps = [
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

# tests for wrapped_proto_caster

pybind_extension(
    name = "wrapped_proto_module",
    srcs = ["wrapped_proto_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:wrapped_proto_caster",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "wrapped_proto_module_test",
    srcs = ["wrapped_proto_module_test.py"],
    data = [":wrapped_proto_module.so"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":compare",
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
        "@com_google_protobuf//:protobuf_python",
        requirement("absl_py"),
    ],
)

pybind_extension(
    name = "thread_module",
    srcs = ["thread_module.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

py_test(
    name = "thread_module_test",
    srcs = ["thread_module_test.py"],
    data = [":thread_module.so"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = [
        ":test_py_pb2",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
        "@com_google_protobuf//:protobuf_python",
        requirement("absl_py"),
    ],
)

pybind_extension(
    name = "regression_wrappers_module",
    srcs = ["regression_wrappers_module.cc"],
    deps = [
        "//pybind11_protobuf:native_proto_caster",
        "@com_google_protobuf//:protobuf",
    ],
)

py_test(
    name = "regression_wrappers_test",
    srcs = ["regression_wrappers_test.py"],
    data = [":regression_wrappers_module.so"],
    deps = [
        "@com_google_protobuf//:protobuf_python",
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_absl_py//absl/testing:parameterized",
        requirement("absl_py"),
    ],
)

pybind_extension(
    name = "we_love_dashes_cc_only_module",
    srcs = ["we_love_dashes_cc_only_module.cc"],
    deps = [
        ":we_love_dashes_cc_proto",
        "//pybind11_protobuf:native_proto_caster",
    ],
)

py_test(
    name = "we_love_dashes_cc_only_test",
    srcs = ["we_love_dashes_cc_only_test.py"],
    data = [":we_love_dashes_cc_only_module.so"],
    deps = [
        "@com_google_absl_py//absl/testing:absltest",
        "@com_google_protobuf//:protobuf_python",
        requirement("absl_py"),
    ],
)

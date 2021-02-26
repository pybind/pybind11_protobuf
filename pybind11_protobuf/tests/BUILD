# Tests and examples for the pybind11_protobuf package.

load("@com_github_grpc_grpc//bazel:python_rules.bzl", "py_proto_library")
load("//devtools/python/blaze:python3.bzl", "py2and3_strict_test")
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
# [internal] load cc_proto_library.bzl

licenses(["notice"])

pybind_extension(
    name = "proto_example",
    srcs = ["proto_example.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:proto_casters",
    ],
)

py2and3_strict_test(
    name = "proto_test",
    srcs = ["proto_test.py"],
    data = [
        ":proto_example.so",
        "//pybind11_protobuf:proto.so",
        "@com_google_protobuf//:proto_api",
    ],
    deps = [
        ":test_py_pb2",
        "//net/proto2/contrib/pyutil:compare",
        "//net/proto2/python/public",
        "//net/proto2/python/public:use_pure_python",  # Automatically added go/proto_python_default
        "//testing/pybase",
        "//testing/pybase:parameterized",
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

py2and3_strict_test(
    name = "import_order_a_test",
    srcs = ["import_order_a_test.py"],
    data = [
        ":proto_example.so",
        "//pybind11_protobuf:proto.so",
    ],
    deps = [
        "//net/proto2/python/public:use_pure_python",  # Automatically added go/proto_python_default
        "//testing/pybase",
    ],
)

py2and3_strict_test(
    name = "import_order_b_test",
    srcs = ["import_order_b_test.py"],
    data = [
        ":proto_example.so",
        "//pybind11_protobuf:proto.so",
    ],
    deps = [
        "//net/proto2/python/public:use_pure_python",  # Automatically added go/proto_python_default
        "//testing/pybase",
    ],
)

# An example (and test) of using an alternative path for the protobuf module.
# Note that "alternate_protobuf_module" is an arbitrary name; no corresponding
# "alternate_protobuf_module.py/so" file is requried.
# This test covers the case that the proto module only exists in linked symbols
# and does not exist as a pybind_extension or py_library. This feature only
# works with Python 3, therefore this test is left as a PY3-only test.
pybind_extension(
    name = "alternate_module_path_example",
    srcs = ["alternate_module_path_example.cc"],
    local_defines = ["PYBIND11_PROTOBUF_MODULE_PATH=alternate_protobuf_module"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:proto_casters",
    ],
)

py_test(
    name = "alternate_module_path_test",
    srcs = ["alternate_module_path_test.py"],
    data = [":alternate_module_path_example.so"],
    python_version = "PY3",
    srcs_version = "PY3",
    deps = ["//testing/pybase"],
)

pybind_extension(
    name = "missing_import",
    srcs = ["missing_import.cc"],
    copts = ["-UNDEBUG"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:proto_casters",
    ],
)

py2and3_strict_test(
    name = "missing_import_test",
    srcs = ["missing_import_test.py"],
    data = [":missing_import.so"],
    srcs_version = "PY2AND3",
    deps = [
        ":test_py_pb2",
        "//net/proto2/python/public:use_pure_python",  # Automatically added go/proto_python_default
        "//testing/pybase",
    ],
)

# Tests for fast_cpp_proto casters

pybind_extension(
    name = "fast_cpp_proto_example",
    srcs = ["fast_cpp_proto_example.cc"],
    deps = [
        ":test_cc_proto",
        "//pybind11_protobuf:fast_cpp_proto_casters",
        "@com_google_protobuf//:use_fast_cpp_protos",
    ],
)

py2and3_strict_test(
    name = "fast_cpp_proto_test",
    srcs = ["fast_cpp_proto_test.py"],
    srcs_version = "PY2AND3",
    deps = [
        ":fast_cpp_proto_example",
        ":test_py_pb2",
        "//net/proto2/contrib/pyutil:compare",
        "//net/proto2/python/public",
        "//net/proto2/python/public:use_fast_cpp_protos",
        "//testing/pybase",
        "//testing/pybase:parameterized",
    ],
)

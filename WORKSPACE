workspace(name = "com_google_pybind11_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_skylib",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.1.1/bazel-skylib-1.1.1.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.1.1/bazel-skylib-1.1.1.tar.gz",
    ],
    sha256 = "c6966ec828da198c5d9adbaa94c05e3a1c7f21bd012a0b29ba8ddbccb2c93b0d",
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()


http_archive(
    name = "com_google_absl",
    sha256 = "5366d7e7fa7ba0d915014d387b66d0d002c03236448e1ba9ef98122c13b35c36",  # SHARED_ABSL_SHA
    strip_prefix = "abseil-cpp-20230125.3",
    urls = [
        "https://github.com/abseil/abseil-cpp/archive/refs/tags/20230125.3.tar.gz",
    ],
)

http_archive(
    name = "com_google_absl_py",
    repo_mapping = {"@six_archive": "@six"},
    sha256 = "0be59b82d65dfa1f995365dcfea2cc57989297b065fda696ef13f30fcc6c8e5b",
    strip_prefix = "abseil-py-pypi-v0.15.0",
    urls = [
        "https://github.com/abseil/abseil-py/archive/refs/tags/pypi-v0.15.0.tar.gz",
    ],
)

## `pybind11_bazel`
# See https://github.com/pybind/pybind11_bazel
http_archive(
  name = "pybind11_bazel",
  strip_prefix = "pybind11_bazel-faf56fb3df11287f26dbc66fdedf60a2fc2c6631",
  sha256 = "a2b107b06ffe1049696e132d39987d80e24d73b131d87f1af581c2cb271232f8",
  urls = ["https://github.com/pybind/pybind11_bazel/archive/faf56fb3df11287f26dbc66fdedf60a2fc2c6631.tar.gz"],
)

# We still require the pybind library.
http_archive(
  name = "pybind11",
  build_file = "@pybind11_bazel//:pybind11.BUILD",
  strip_prefix = "pybind11-master",
  urls = ["https://github.com/pybind/pybind11/archive/refs/heads/master.tar.gz"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")
python_configure(name = "local_config_python", python_version = "3")

# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
http_archive(
    name = "com_google_protobuf",
    sha256 = "0b0395d34e000f1229679e10d984ed7913078f3dd7f26cf0476467f5e65716f4",
    strip_prefix = "protobuf-23.2",
    urls = ["https://github.com/protocolbuffers/protobuf/archive/refs/tags/v23.2.tar.gz"],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

# GRPC v1.42, for proto rules.
# For a related discussion of the pro/cons of various open-source py proto rule
# repositories, see b/189457935.
http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "9f387689b7fdf6c003fd90ef55853107f89a2121792146770df5486f0199f400",
    strip_prefix = "grpc-1.42.0",
    urls = ["https://github.com/grpc/grpc/archive/v1.42.0.zip"],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()

bind(
    name = "python_headers",
    actual = "@local_config_python//:python_headers",
)

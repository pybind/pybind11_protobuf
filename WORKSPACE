workspace(name = "com_google_pybind11_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    commit = "6df644c56f31b100bf731e27c3825069745651e3",
)

git_repository(
    name = "com_google_absl_py",
    remote = "https://github.com/abseil/abseil-py.git",
    commit = "9954557f9df0b346a57ff82688438c55202d2188",
)

# Six is a dependency of com_google_absl_py
http_archive(
    name = "six_archive",
    urls = [
        "http://mirror.bazel.build/pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
        "https://pypi.python.org/packages/source/s/six/six-1.10.0.tar.gz",
    ],
    sha256 = "105f8d68616f8248e24bf0e9372ef04d3cc10104f1980f54d57b2ce73a5ad56a",
    strip_prefix = "six-1.10.0",
    build_file = "@com_google_absl_py//third_party:six.BUILD",
)

## `pybind11_bazel`
# See https://github.com/pybind/pybind11_bazel
git_repository(
  name = "pybind11_bazel",
  remote = "https://github.com/pybind/pybind11_bazel.git",
  commit = "26973c0ff320cb4b39e45bc3e4297b82bc3a6c09",
)

# We still require the pybind library.
new_git_repository(
  name = "pybind11",
  build_file = "@pybind11_bazel//:pybind11.BUILD",
  remote = "https://github.com/pybind/pybind11.git",
  tag = "v2.7.1",
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")
python_configure(name = "local_config_python")

# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/protocolbuffers/protobuf.git",
    tag = "v3.18.0-rc1",
    patches = ["com_google_protobuf_build.patch"],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

# GRPC v1.38, for proto rules.
# For a related discussion of the pro/cons of various open-source py proto rule
# repositories, see b/189457935.
git_repository(
    name = "com_github_grpc_grpc",
    remote = "https://github.com/grpc/grpc.git",
    tag = "v1.38.0",
)
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()

bind(
    name = "python_headers",
    actual = "@local_config_python//:python_headers",
)

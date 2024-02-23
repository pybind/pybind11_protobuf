workspace(name = "com_google_pybind11_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "bazel_skylib",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.5.0/bazel-skylib-1.5.0.tar.gz"
    ],
    sha256 = "cd55a062e763b9349921f0f5db8c3933288dc8ba4f76dd9416aac68acee3cb94",
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

http_archive(
    name = "com_google_absl",
    sha256 = "59d2976af9d6ecf001a81a35749a6e551a335b949d34918cfade07737b9d93c5",  # SHARED_ABSL_SHA
    strip_prefix = "abseil-cpp-20230802.0",
    urls = [
        "https://github.com/abseil/abseil-cpp/archive/refs/tags/20230802.0.tar.gz"
    ],
)

http_archive(
    name = "com_google_absl_py",
    sha256 = "8a3d0830e4eb4f66c4fa907c06edf6ce1c719ced811a12e26d9d3162f8471758",
    strip_prefix = "abseil-py-2.1.0",
    urls = [
        "https://github.com/abseil/abseil-py/archive/refs/tags/v2.1.0.tar.gz",
    ],
)

http_archive(
    name = "rules_python",
    sha256 = "c68bdc4fbec25de5b5493b8819cfc877c4ea299c0dcb15c244c5a00208cde311",
    strip_prefix = "rules_python-0.31.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.31.0/rules_python-0.31.0.tar.gz",
)

load("@rules_python//python:repositories.bzl", "py_repositories", "python_register_multi_toolchains")

py_repositories()


DEFAULT_PYTHON = "3.11"

python_register_multi_toolchains(
    name = "python",
    default_version = DEFAULT_PYTHON,
    python_versions = [
      "3.12",
      "3.11",
      "3.10",
      "3.9",
      "3.8"
    ],
)


load("@python//:pip.bzl", "multi_pip_parse")

multi_pip_parse(
    name = "pypi",
    default_version = DEFAULT_PYTHON,
    python_interpreter_target = {
        "3.12": "@python_3_12_host//:python",
        "3.11": "@python_3_11_host//:python",
        "3.10": "@python_3_10_host//:python",
        "3.9": "@python_3_9_host//:python",
        "3.8": "@python_3_8_host//:python",
    },
    requirements_lock = {
        "3.12": "//pybind11_protobuf/requirements:requirements_lock_3_12.txt",
        "3.11": "//pybind11_protobuf/requirements:requirements_lock_3_11.txt",
        "3.10": "//pybind11_protobuf/requirements:requirements_lock_3_10.txt",
        "3.9": "//pybind11_protobuf/requirements:requirements_lock_3_9.txt",
        "3.8": "//pybind11_protobuf/requirements:requirements_lock_3_8.txt",
    },
)

load("@pypi//:requirements.bzl", "install_deps")

install_deps()

## `pybind11_bazel` (PINNED)
# https://github.com/pybind/pybind11_bazel
http_archive(
  name = "pybind11_bazel",
  strip_prefix = "pybind11_bazel-2.11.1.bzl.2",
  sha256 = "e2ba5f81f3bf6a3fc0417448d49389cc7950bebe48c42c33dfeb4dd59859b9a4",
  urls = ["https://github.com/pybind/pybind11_bazel/releases/download/v2.11.1.bzl.2/pybind11_bazel-2.11.1.bzl.2.tar.gz"],
)

## `pybind11` (FLOATING)
http_archive(
  name = "pybind11",
  build_file = "@pybind11_bazel//:pybind11.BUILD",
  strip_prefix = "pybind11-master",
  urls = ["https://github.com/pybind/pybind11/archive/refs/heads/master.tar.gz"],
)

# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
http_archive(
    name = "com_google_protobuf",
    sha256 = "d19643d265b978383352b3143f04c0641eea75a75235c111cc01a1350173180e",
    strip_prefix = "protobuf-25.3",
    urls = ["https://github.com/protocolbuffers/protobuf/releases/download/v25.3/protobuf-25.3.tar.gz"],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

# GRPC v1.42, for proto rules.
# For a related discussion of the pro/cons of various open-source py proto rule
# repositories, see b/189457935.
http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "84e31a77017911b2f1647ecadb0172671d96049ea9ad5109f02b4717c0f03702",
    strip_prefix = "grpc-1.56.3",
    urls = ["https://github.com/grpc/grpc/archive/refs/tags/v1.56.3.tar.gz"],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()

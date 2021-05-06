
workspace(name = "com_google_pybind11_protobuf")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

# To update a dependency to a new revision,
# a) update URL and strip_prefix to the new git commit hash
# b) get the sha256 hash of the commit by running:
#    curl -L https://github.com/<...>.tar.gz | sha256sum
#    and update the sha256 with the result.
http_archive(
    name = "com_google_absl",
    sha256 = "137836d52edb41891cc6d137a228278ae30e76607e3cbd6b8cdb653743c0823e",  # SHARED_ABSL_SHA
    strip_prefix = "abseil-cpp-6df644c56f31b100bf731e27c3825069745651e3",
    urls = [
        "https://github.com/abseil/abseil-cpp/archive/6df644c56f31b100bf731e27c3825069745651e3.tar.gz",
    ],
)

## `pybind11_bazel`
# See https://github.com/pybind/pybind11_bazel
http_archive(
  name = "pybind11_bazel",
  strip_prefix = "pybind11_bazel-26973c0ff320cb4b39e45bc3e4297b82bc3a6c09",
  sha256 = "8f546c03bdd55d0e88cb491ddfbabe5aeb087f87de2fbf441391d70483affe39",
  urls = ["https://github.com/pybind/pybind11_bazel/archive/26973c0ff320cb4b39e45bc3e4297b82bc3a6c09.tar.gz"],
)

# We still require the pybind library.
new_git_repository(
  name = "pybind11",
  build_file = "@pybind11_bazel//:pybind11.BUILD",
  remote = "https://github.com/pybind/pybind11.git",
  commit = "932769b03855d1b18694e7a867bbd1c24ff6170e",
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")
python_configure(name = "local_config_python")

# proto_library, cc_proto_library, and java_proto_library rules implicitly
# depend on @com_google_protobuf for protoc and proto runtimes.
# This statement defines the @com_google_protobuf repo.
git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/protocolbuffers/protobuf.git",
    tag = "v3.15.4",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

# Google logging
git_repository(
    name = "com_google_glog",
    remote = "https://github.com/google/glog.git",
    branch = "master"
)
# Dependency for glog
git_repository(
    name = "com_github_gflags_gflags",
    remote = "https://github.com/mchinen/gflags.git",
    branch = "android_linking_fix"
)

bind(
    name = "python_headers",
    actual = "@com_google_protobuf//util/python:python_headers",
)

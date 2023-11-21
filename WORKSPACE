workspace(name = "com_google_pybind11_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

## `pybind11` (FLOATING)
#http_archive(
#  name = "pybind11",
#  build_file = "@pybind11_bazel//:pybind11.BUILD",
#  strip_prefix = "pybind11-master",
#  urls = ["https://github.com/pybind/pybind11/archive/refs/heads/master.tar.gz"],
#)

http_archive(
    name = "com_google_protobuf",
    sha256 = "616bb3536ac1fff3fb1a141450fa28b875e985712170ea7f1bfe5e5fc41e2cd8",
    strip_prefix = "protobuf-24.4",
    urls = ["https://github.com/protocolbuffers/protobuf/archive/v24.4.tar.gz"],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

http_archive(
    name = "rules_proto",
    sha256 = "903af49528dc37ad2adbb744b317da520f133bc1cbbecbdd2a6c546c9ead080b",
    strip_prefix = "rules_proto-6.0.0-rc0",
    url = "https://github.com/bazelbuild/rules_proto/releases/download/6.0.0-rc0/rules_proto-6.0.0-rc0.tar.gz",
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

load("@pybind11_bazel//:python_configure.bzl", "python_configure")
python_configure(name = "local_config_python", python_version = "3")

bind(
    name = "python_headers",
    actual = "@local_config_python//:python_headers",
)
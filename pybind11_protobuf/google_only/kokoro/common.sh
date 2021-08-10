#!/bin/bash
set -ex

github_init() {
  cd "${KOKORO_ROOT}/src/github/pybind11_protobuf"
  pwd
}

bazel_init() {
  local bazel_version="$(cat .bazelversion)"
  chmod +x "${KOKORO_GFILE_DIR}"/use_bazel.sh
  "${KOKORO_GFILE_DIR}"/use_bazel.sh "${bazel_version}"
  bazel version
}

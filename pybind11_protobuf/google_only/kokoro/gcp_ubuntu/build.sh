#!/bin/bash
set -ex

main() {
  source "${KOKORO_GFILE_DIR}/common.sh"

  github_init
  bazel_init

  # TODO(b/193511932): Open source `pybind11_protobuf` unit tests.
  bazel test \
      -- \
      //pybind11_protobuf/tests/... \
      -//pybind11_protobuf/tests:dynamic_message_test \
      -//pybind11_protobuf/tests:proto_test \
      -//pybind11_protobuf/tests:extension_test \
      -//pybind11_protobuf/tests:message_test \
      -//pybind11_protobuf/tests:pass_by_test \
      -//pybind11_protobuf/tests:wrapped_proto_module_test
}

main "$@"

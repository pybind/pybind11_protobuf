#!/bin/bash
set -ex

github_init() {
  cd "${KOKORO_ROOT}/src/github/pybind11_protobuf"
  pwd
}

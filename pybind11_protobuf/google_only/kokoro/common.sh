#!/bin/bash
set -ex

github_init() {
  cd "${KOKORO_ROOT}/src/github/federated"
  pwd
}

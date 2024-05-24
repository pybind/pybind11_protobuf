#!/bin/bash

# The following script builds and runs tests

set -e  # exit when any command fails
set -x  # Prints all executed command

MYDIR="$(dirname "$(realpath "$0")")"

BAZEL=$(which bazel || true)
if [[ -z $BAZEL || ! -x $BAZEL ]]
then
  echo -e -n '\e[1m\e[93m'
  echo -n 'Bazel not found (bazel (https://bazel.build/) is needed to '
  echo -n 'compile & test). '
  echo -e 'Exiting...\e[0m'
  exit 1
fi

echo "Building and testing in $PWD using 'python' (version $PYVERSION)."

bazel clean --expunge # Force a dep update

BAZEL_CXXOPTS="-std=c++17" bazel test ... --test_output=errors "$@" --enable_bzlmod
BAZEL_CXXOPTS="-std=c++20" bazel test ... --test_output=errors "$@" --enable_bzlmod

BAZEL_CXXOPTS="-std=c++17" bazel test ... --test_output=errors "$@" --noenable_bzlmod
BAZEL_CXXOPTS="-std=c++20" bazel test ... --test_output=errors "$@" --noenable_bzlmod

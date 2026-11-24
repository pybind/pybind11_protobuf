#!/bin/bash

# The following script builds and runs tests

set -e  # exit when any command fails
set -x  # Prints all executed commands

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

bazel clean --expunge # Force a deep update

bazel test --cxxopt=-std=c++14 ... --test_output=errors "$@" --enable_bzlmod
bazel test --cxxopt=-std=c++17 ... --test_output=errors "$@" --enable_bzlmod
bazel test --cxxopt=-std=c++20 ... --test_output=errors "$@" --enable_bzlmod

bazel test --cxxopt=-std=c++14 ... --test_output=errors "$@" --noenable_bzlmod --enable_workspace
bazel test --cxxopt=-std=c++17 ... --test_output=errors "$@" --noenable_bzlmod --enable_workspace
bazel test --cxxopt=-std=c++20 ... --test_output=errors "$@" --noenable_bzlmod --enable_workspace

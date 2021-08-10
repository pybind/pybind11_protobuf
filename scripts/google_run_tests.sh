#!/bin/bash

# Build using cmake and run the tests, without using google3
# This uses copybara to copy everything and applies source-code transformations.
#
# If it fails, try deleting `~/tmp/pybind11_protobuf` before escalating.

PIPERDIR=${PWD%%google3*}

source gbash.sh || exit

DEFINE_bool cp_source true 'Whether to copy the source code or not.'
DEFINE_bool build true 'If false, it will just run copybara and exit.'

gbash::init_google "$@"

if [ ! -f scripts/google_run_tests.sh ]
then
  echo "This script must be run from the main pybind11_protobuf directory."
  exit -1
fi

# The structure is the following:
# ~/tmp/pybind11_protobuf containing all the code.
# ~/tmp/pybind11_protobuf/venv for the Python 3 virtualenv

TMPDIR="/tmp/pybind11_protobuf"
if [ ! -z "$HOME" ]; then
  TMPDIR="$HOME/tmp/pybind11_protobuf"
fi
CODEDIR="$TMPDIR"


# CopyBara delete the files, but not the directories, thus we do it here (we
# clean everything except what we want to keep).
echo -e "\e[33mDeleting files $CODEDIR, except for venv/ and build/\e[0m"
# Delete all the code, except for the virtualenv directory
top_level_directories=$(find ${CODEDIR}/* -maxdepth 0 -type d | grep -v "/venv")
top_level_files=$(find ${CODEDIR}/* -maxdepth 0 -type f)
for d in $top_level_directories
do
  echo "Deleting ${d}/*"
  rm -rf $d
done
echo ""
for d in $top_level_files
do
  echo "Deleting $d"
  rm -f $d
done
mkdir -p $TMPDIR


set -e  # Stop if any command fails

if [[ $FLAGS_cp_source ]]; then
  echo -e "\e[33mCopying source code from $PIPERDIR to $CODEDIR with copybara.\e[0m"
  /google/data/ro/teams/copybara/copybara \
  $PIPERDIR/google3/pybind11_protobuf/copy.bara.sky \
  local_install $PIPERDIR --folder-dir $CODEDIR --ignore-noop
else
  echo -e "\e[33mReusing source code from $CODEDIR.\e[0m"
fi

if [[ $FLAGS_build == "false" ]]; then
  echo -e "\e[33mFlag --build=false passed. Exiting now.\e[0m"
  exit 1
fi

pushd $CODEDIR

echo -e "\e[33mRunning $CODEDIR/scripts/build_and_run_tests.sh from $PWD\e[0m"

echo -e "\e[32m*****************************\e[0m"
echo -e "\e[32mBuilding and testing in $PWD.\e[0m"
echo -e "\e[32m*****************************\e[0m"
echo ""

bash "$CODEDIR/scripts/build_and_run_tests.sh"

cd ..

popd > /dev/null

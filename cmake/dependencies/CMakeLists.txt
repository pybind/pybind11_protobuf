include(FetchContent)

# ============================================================================
# Declare all dependencies first

find_package(absl ${_absl_version} EXACT QUIET)
if(NOT absl_FOUND)
  set(ABSL_PROPAGATE_CXX_STD ON)
  FetchContent_Declare(
    absl
    GIT_REPOSITORY ${_absl_repository}
    GIT_TAG ${_absl_tag})
endif()

# https://stackoverflow.com/questions/63309544/cmake-protobuf-external-to-application-code
# https://cmake.org/cmake/help/latest/policy/CMP0077.html
# https://gitlab.kitware.com/cmake/cmake/-/merge_requests/7565/diffs
find_package(Protobuf ${_protobuf_version} EXACT QUIET)
if(NOT Protobuf_FOUND)
  set(protobuf_BUILD_TESTS
      OFF
      CACHE INTERNAL "")
  FetchContent_Declare(
    Protobuf
    GIT_REPOSITORY ${_protobuf_repository}
    GIT_TAG ${_protobuf_tag}
    GIT_SUBMODULES "")
endif()

find_package(pybind11 ${_pybind11_version} EXACT QUIET)
if(NOT pybind11_FOUND)
  set(PYBIND11_TEST OFF)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY ${_pybind11_repository}
    GIT_TAG ${_pybind11_tag})
endif()

# ============================================================================
# Make dependencies avaialble

if(NOT absl_FOUND)
  message(CHECK_START "Fetching Abseil-cpp")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(absl)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

if(NOT Protobuf_FOUND)
  message(CHECK_START "Fetching Protobuf")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(Protobuf)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

if(NOT pybind11_FOUND)
  message(CHECK_START "Fetching pybind11")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(pybind11)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

include(FetchContent)

#============================================================================
# Declare all dependencies first

if(BUILD_absl)
  FetchContent_Declare(
    absl
    GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
    GIT_TAG "20220623.1"
  )
endif()

# https://stackoverflow.com/questions/63309544/cmake-protobuf-external-to-application-code
# https://cmake.org/cmake/help/latest/policy/CMP0077.html
# https://gitlab.kitware.com/cmake/cmake/-/merge_requests/7565/diffs
if(BUILD_protobuf)
  set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "")
  FetchContent_Declare(
    protobuf
    GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
    GIT_TAG "v21.5"
    GIT_SUBMODULES ""
  )
endif()

if(BUILD_pybind11)
  set(PYBIND11_TEST OFF)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
    GIT_TAG "v2.10.1"
  )
endif()

#============================================================================
# Make dependencies avaialble

if(BUILD_absl)
  message(CHECK_START "Fetching Abseil-cpp")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(absl)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

if(BUILD_protobuf)
  message(CHECK_START "Fetching protobuf")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(protobuf)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

if(BUILD_pybind11)
  message(CHECK_START "Fetching pybind11")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(pybind11)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()


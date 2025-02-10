include(FetchContent)
set(BUILD_SHARED_LIBS ON)
set(BUILD_TESTING OFF)

message(CHECK_START "Checking for external dependencies")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

if(NOT TARGET absl::base)
  if(USE_SYSTEM_ABSEIL)
    # Version omitted, as absl only allows EXACT version matches
    find_package(absl REQUIRED)
  else()
    message(CHECK_START "Fetching Abseil-cpp")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    # ensure that abseil also installs itself, since we are using it in our
    # public API
    set(ABSL_ENABLE_INSTALL ON)
    set(ABSL_PROPAGATE_CXX_STD ON)
    set(ABSL_USE_SYSTEM_INCLUDES ON)
    set(ABSL_BUILD_TESTING OFF)
    FetchContent_Declare(
      absl
      GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
      GIT_TAG "20240722.0"
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(absl)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
    message(CHECK_PASS "fetched")
  endif()
endif()

if(NOT TARGET protobuf::libprotobuf)
  if(USE_SYSTEM_PROTOBUF)
    find_package(protobuf 5.29.2 REQUIRED)
  else()
    message(CHECK_START "Fetching Protobuf")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    set(protobuf_BUILD_TESTS OFF)
    FetchContent_Declare(
      Protobuf
      GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
      GIT_TAG "v29.2"
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Protobuf)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
    message(CHECK_PASS "fetched")
  endif()
endif()

if(NOT TARGET pybind11::pybind11_headers)
  if(USE_SYSTEM_PYBIND)
    find_package(pybind11 2.13.6 REQUIRED)
  else()
    message(CHECK_START "Fetching pybind11")
    list(APPEND CMAKE_MESSAGE_INDENT "  ")
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
      GIT_TAG "v2.13.6"
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(pybind11)
    list(POP_BACK CMAKE_MESSAGE_INDENT)
    message(CHECK_PASS "fetched")
  endif()
endif()

if(PYBIND11_PROTOBUF_BUILD_TESTING AND NOT TARGET pybind11_abseil::absl_casters)
  message(CHECK_START "Fetching pybind11_abseil")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_Declare(
    pybind11_abseil
    GIT_REPOSITORY "https://github.com/pybind/pybind11_abseil.git"
    # TODO(mizux) use a release version tag once available.
    GIT_TAG "master"
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(pybind11_abseil)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "all dependencies found")

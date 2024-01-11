include(FetchContent)

# ============================================================================
# Declare all dependencies first

if(NOT absl_FOUND)
  set(ABSL_PROPAGATE_CXX_STD ON)
  set(ABSL_ENABLE_INSTALL ON)
  FetchContent_Declare(
    absl
    GIT_REPOSITORY ${_absl_repository}
    GIT_TAG ${_absl_tag})
endif()

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

if(NOT pybind11_FOUND)
  message(CHECK_START "Fetching pybind11")
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  FetchContent_MakeAvailable(pybind11)
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "fetched")
endif()

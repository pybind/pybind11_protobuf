if(NOT BUILD_absl)
  find_package(absl REQUIRED)
endif()

if(NOT BUILD_protobuf)
  find_package(protobuf REQUIRED)
endif()

if(NOT BUILD_pybind11)
  find_package(pybind11 REQUIRED)
endif()


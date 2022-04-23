cmake_minimum_required(VERSION 3.8)
project(HTTPRequest CXX)

# Header only library will be interface. All properties get passed to targets that 'link' to it
add_library(HTTPRequest INTERFACE)
target_compile_features(HTTPRequest INTERFACE cxx_std_17)

target_include_directories(HTTPRequest
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Optionally build unit tests
option(BUILD_TESTING "Build Unit Tests" OFF)
if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()


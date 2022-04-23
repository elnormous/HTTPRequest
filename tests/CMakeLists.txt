add_executable(HTTPRequest_tests
  main.cpp
  encoding.cpp
  parsing.cpp
)

target_link_libraries(HTTPRequest_tests
  PRIVATE
    HTTPRequest
)

target_include_directories(HTTPRequest_tests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/external/Catch2/single_include
)

# Enable use of 'make test'
add_test(test HTTPRequest_tests)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS HTTPRequest_tests)


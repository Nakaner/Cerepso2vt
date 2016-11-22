enable_testing()

message(STATUS "Configuring unit tests")

include(CMakeParseArguments)
include_directories(include)
include_directories(../src)

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")

# Otherwise GCC throws a lot of warnings for REQUIRE(...) from Catch v.1.2.1
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-parentheses)
endif()

add_executable(test_array_parser t/test_array_parser.cpp)
target_link_libraries(test_array_parser testlib)
add_test(NAME test_array_parser
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_array_parser)

add_executable(test_hstore_parser t/test_hstore_parser.cpp ../src/hstore_parser.cpp)
target_link_libraries(test_hstore_parser testlib)
add_test(NAME test_hstore_parser
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_hstore_parser)

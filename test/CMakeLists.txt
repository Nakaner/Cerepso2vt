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

add_executable(test_bbox_conversion t/test_bbox_conversion.cpp ../src/bounding_box.cpp)
target_link_libraries(test_bbox_conversion testlib ${PROJ_LIBRARY})
add_test(NAME test_bbox_conversion
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_bbox_conversion)

add_executable(test_item_type_conversion t/test_item_type_conversion.cpp)
target_link_libraries(test_item_type_conversion testlib ${PROJ_LIBRARY})
add_test(NAME test_item_type_conversion
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_item_type_conversion)

cmake_minimum_required (VERSION 3.5)
project (leptjson_test1 C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall")
endif()

add_library(leptjson1 leptjson.c)
add_executable(leptjson_test1 test.c)
target_link_libraries(leptjson_test1 leptjson1)

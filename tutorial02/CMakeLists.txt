cmake_minimum_required (VERSION 3.5)
project (leptjson_test2 C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall")
endif()

add_library(leptjson2 leptjson.c)
add_executable(leptjson_test2 test.c)
target_link_libraries(leptjson_test2 leptjson2)

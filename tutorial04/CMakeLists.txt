cmake_minimum_required (VERSION 3.5)
project (leptjson_test4 C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall")
endif()

add_library(leptjson4 leptjson.c)
target_link_libraries(leptjson4 m)
add_executable(leptjson_test4 test.c)
target_link_libraries(leptjson_test4 leptjson4)

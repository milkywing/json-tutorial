cmake_minimum_required (VERSION 3.5)
project (leptjson_test7 C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall")
endif()

add_library(leptjson7 leptjson.c)
add_executable(leptjson_test7 test.c)
target_link_libraries(leptjson_test7 leptjson7)

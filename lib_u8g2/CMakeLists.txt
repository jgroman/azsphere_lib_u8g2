cmake_minimum_required (VERSION 3.8)
project (lib_u8g2 C)

add_library(lib_u8g2 lib_u8g2.c)

file(GLOB_RECURSE u8g2_SRC "${CMAKE_CURRENT_LIST_DIR}/u8g2/csrc/*.c")
add_library(u8g2 ${u8g2_SRC})

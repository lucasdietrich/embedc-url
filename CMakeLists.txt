cmake_minimum_required(VERSION 3.2)
project(embedc-url VERSION 1.0)

file(GLOB_RECURSE APP_SOURCES src/*.c)

add_library(embedc-url ${APP_SOURCES})

target_include_directories(embedc-url PUBLIC include)

add_subdirectory(samples)
add_subdirectory(tests)
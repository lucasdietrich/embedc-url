cmake_minimum_required(VERSION 3.2)
project(embedcutils VERSION 1.0)

file(GLOB_RECURSE APP_SOURCES src/*.c)

add_library(embedcutils ${APP_SOURCES})

target_include_directories(embedcutils PUBLIC include)

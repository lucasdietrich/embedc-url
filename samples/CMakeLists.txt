cmake_minimum_required(VERSION 3.2)

set(exe sample_url)

add_executable(${exe} main.c routes_g.c handlers.c)

target_include_directories(${exe} PUBLIC .)

target_link_libraries(${exe} PUBLIC embedc-url)
cmake_minimum_required(VERSION 3.24)
project(simple_synth C)

set(CMAKE_C_STANDARD 23)

add_executable(simple_synth src/main.c src/ctx.c)

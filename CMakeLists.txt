cmake_minimum_required(VERSION 3.31)
project(ComputergrafikLabor_A3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-g -Wall -Wextra -Wpedantic)

add_executable(ComputergrafikLabor_A3 raytracer.cc math.cc geometry.cc)

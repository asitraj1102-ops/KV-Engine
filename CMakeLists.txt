cmake_minimum_required(VERSION 3.10)
project(KVEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -O3)
include_directories(include)

add_executable(kv_engine
    src/main.cpp
    src/KVEngine.cpp
)

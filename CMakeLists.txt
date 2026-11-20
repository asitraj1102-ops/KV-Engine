cmake_minimum_required(VERSION 3.16)
project(KVEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

add_executable(kv_engine main.cpp)
target_include_directories(kv_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kv_engine PRIVATE -Wall -Wextra)

enable_testing()
add_executable(kv_tests tests/test_kv_engine.cpp)
target_include_directories(kv_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kv_tests PRIVATE -Wall -Wextra)
add_test(NAME KVEngineUnitTests COMMAND kv_tests)

add_executable(kv_bench benchmarks/benchmark.cpp)
target_include_directories(kv_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kv_bench PRIVATE -Wall -Wextra)

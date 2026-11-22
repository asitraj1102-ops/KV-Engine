cmake_minimum_required(VERSION 3.16)
project(KVEngine VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ENABLE_ASAN and ENABLE_TSAN cannot be enabled at the same time.")
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()

add_executable(kv_engine main.cpp)
target_include_directories(kv_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kv_engine PRIVATE -Wall -Wextra)
target_link_libraries(kv_engine PRIVATE Threads::Threads)

enable_testing()
add_executable(kv_tests tests/test_kv_engine.cpp)
target_include_directories(kv_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kv_tests PRIVATE -Wall -Wextra)
target_link_libraries(kv_tests PRIVATE Threads::Threads)
add_test(NAME KVEngineUnitTests COMMAND kv_tests)

add_executable(kv_bench benchmarks/benchmark.cpp)
target_include_directories(kv_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kv_bench PRIVATE -Wall -Wextra)
target_link_libraries(kv_bench PRIVATE Threads::Threads)

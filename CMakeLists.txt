cmake_minimum_required(VERSION 3.20)

project(TitanServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TitanServer
    src/main.cpp
    src/Logger.cpp
    src/ThreadPool.cpp
)

target_include_directories(TitanServer PRIVATE include)




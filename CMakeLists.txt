# Copyright (c) 2026, Alexey Gavrilov


cmake_minimum_required(VERSION 3.28)
project(gerritutils
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gerritprutil)

target_compile_definitions(gerritprutil
    PUBLIC
        VERSION=\"${gerritutils_VERSION}\"
)

add_subdirectory(src)

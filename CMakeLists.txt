# Copyright (c) 2026, Alexey Gavrilov


cmake_minimum_required(VERSION 3.28)
project(gerritutils
    VERSION 1.0.0
    LANGUAGES CXX
)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 063de7e9578f82b369302001269680b4b1553359
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(gerritprutil)

target_compile_definitions(gerritprutil
    PUBLIC
        VERSION=\"${gerritutils_VERSION}\"
)

add_subdirectory(src)
add_subdirectory(test)

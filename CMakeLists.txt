# Copyright (c) 2026, Alexey Gavrilov


cmake_minimum_required(VERSION 3.28)
project(gerritutils
    VERSION 1.0.0
    LANGUAGES CXX
)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_libraries($<$<CXX_COMPILER_ID:GCC>:pthread>)

include(FetchContent)

FetchContent_Declare(libgit2
    GIT_REPOSITORY https://github.com/libgit2/libgit2.git
    GIT_TAG v1.9.7
)

set(BUILD_EXAMPLES OFF CACHE BOOL INTERNAL)
set(BUILD_TESTING OFF CACHE BOOL INTERNAL)
set(BUILD_CLI OFF CACHE BOOL INTERNAL)
FetchContent_MakeAvailable(libgit2)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 063de7e9578f82b369302001269680b4b1553359
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

find_package(OpenSSL REQUIRED)

fetchcontent_declare(jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.2
)

set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
fetchcontent_makeavailable(jwt-cpp)

add_executable(gerritprutil)

add_library(libc INTERFACE)
target_link_libraries(gerritprutil
    PRIVATE
        libc
        nlohmann_json::nlohmann_json
        libgit2package
        OpenSSL::SSL
        OpenSSL::Crypto
        jwt-cpp::jwt-cpp
)

target_compile_definitions(gerritprutil
    PUBLIC
        VERSION=\"${gerritutils_VERSION}\"
)

target_include_directories(gerritprutil
    PRIVATE
        ${LIBGIT2_INCLUDES}
        ${JWT_INCLUDE_PATH}
)

add_subdirectory(src)
add_subdirectory(test)

cmake_minimum_required(VERSION 3.22)
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "No Toolchain File was passed in during CMake configure. Defaulting to x86_64-gcc11-toolchain.cmake")
    include(cmake/toolchains/x86_64-gcc11-toolchain.cmake)
endif ()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(os_low_level_sdk_client)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Add CLI11
FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.1
)
FetchContent_MakeAvailable(cli11)

add_subdirectory(cmake/dependencies/grpc)

add_subdirectory(protos)

set(SOURCES
    main.cpp
    client.cpp
    ${PROTO_SRCS}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(grpc_client ${SOURCES})

target_link_libraries(grpc_client PUBLIC
    llsdk-grpc-gen
    CLI11::CLI11
)

target_include_directories(grpc_client PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/oslowlevelsdk>
)

cmake_minimum_required(VERSION 3.22)
project(space-optics-sample VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.74.0 COMPONENTS system serialization REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
        flatbuffers
        URL https://github.com/google/flatbuffers/archive/refs/tags/v1.11.0.tar.gz
)
FetchContent_MakeAvailable(flatbuffers)

include(${FlatBuffers_SOURCE_DIR}/CMake/BuildFlatBuffers.cmake)

FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

set(SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers")
file(GLOB FB_SCHEMAS "${SCHEMA_DIR}/*.fbs")

build_flatbuffers(
        "${FB_SCHEMAS}"        # flatbuffers_schemas
        "${SCHEMA_DIR}"        # schema_include_dirs (use a quoted list if multiple)
        fb_schemas             # custom_target_name
        ""                     # additional_dependencies
        "${CMAKE_CURRENT_BINARY_DIR}"  # generated_includes_dir
        ""                     # binary_schemas_dir (leave empty if you don't want .bfbs)
        ""                     # copy_text_schemas_dir
)

add_executable(capture-single-image capture-single-image.cpp)

add_dependencies(capture-single-image fb_schemas)

target_link_libraries(capture-single-image PRIVATE 
        Boost::system
        Boost::serialization
        flatbuffers
        nlohmann_json::nlohmann_json
)

target_include_directories(capture-single-image PRIVATE 
        ${Boost_INCLUDE_DIR} 
        ${CMAKE_BINARY_DIR}
)


add_executable(stream-frames stream-frames.cpp)

add_dependencies(stream-frames fb_schemas)

target_link_libraries(stream-frames PRIVATE 
        Boost::system
        Boost::serialization
        flatbuffers
        nlohmann_json::nlohmann_json
)

target_include_directories(stream-frames PRIVATE 
        ${Boost_INCLUDE_DIR} 
        ${CMAKE_BINARY_DIR}
)

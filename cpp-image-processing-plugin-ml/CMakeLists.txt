cmake_minimum_required(VERSION 3.14)
project(custom-image-processing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Custom find modules ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# --- Qualcomm SNPE SDK ---
find_package(SNPESDK REQUIRED)

# --- cfitsio (FITS image I/O) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(CFITSIO REQUIRED IMPORTED_TARGET cfitsio)

# --- Dependencies via FetchContent ---
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(asio)

# Expose standalone Asio headers so Crow's find_package(asio) succeeds
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
# Tell Crow to use standalone Asio (no Boost)
set(CROW_FEATURES "" CACHE STRING "" FORCE)

# Point Crow's Findasio.cmake at the fetched headers
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "" FORCE)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(Crow nlohmann_json)

# --- Server executable ---
add_executable(${PROJECT_NAME}
    consumer/cpp/src/main.cpp
    consumer/cpp/src/routes.cpp
    consumer/cpp/src/image_processor.cpp
    consumer/cpp/src/snpe_worker.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE consumer/cpp/src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Crow::Crow
        nlohmann_json::nlohmann_json
        SNPE::SNPE
        PkgConfig::CFITSIO
)

cmake_minimum_required(VERSION 3.16)
project(compute_hfd_demo)

# Find OpenCV package
find_package(OpenCV REQUIRED)

add_subdirectory(sep)

# Demo executable for Source Extractor EE50 computation
add_executable(compute_hfd_demo
        demo.cpp
        src/SourceExtractorMeasurement.cpp
)

target_include_directories(compute_hfd_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(compute_hfd_demo PRIVATE
        ${OpenCV_LIBS}
        sep
)

target_compile_features(compute_hfd_demo PRIVATE cxx_std_17)

set_target_properties(compute_hfd_demo PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}"
        BUILD_RPATH_USE_ORIGIN TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        SKIP_BUILD_RPATH FALSE
)

target_link_options(compute_hfd_demo PRIVATE
        "-Wl,--disable-new-dtags"
)

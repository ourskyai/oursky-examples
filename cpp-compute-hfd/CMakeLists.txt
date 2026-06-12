cmake_minimum_required(VERSION 3.16)
project(compute_hfd_demo)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Demo executable for HFD computation
# NOTE: This is a standalone demo that simulates star data
#       It only requires OpenCV, not the full os_image_proc library
add_executable(compute_hfd_demo
        demo.cpp
)

target_include_directories(compute_hfd_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(compute_hfd_demo PRIVATE
        ${OpenCV_LIBS}
)

set_target_properties(compute_hfd_demo PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}"
        BUILD_RPATH_USE_ORIGIN TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        SKIP_BUILD_RPATH FALSE
)

target_link_options(compute_hfd_demo PRIVATE
        "-Wl,--disable-new-dtags"
)

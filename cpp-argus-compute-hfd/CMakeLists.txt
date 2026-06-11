cmake_minimum_required(VERSION 3.16)
project(argus_demo)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Demo executable for ARGUS HFD computation
# NOTE: This is a standalone demo that simulates star data
#       It only requires OpenCV, not the full os_image_proc library
add_executable(demo_argus
        demo_argus.cpp
)

target_include_directories(demo_argus PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(demo_argus PRIVATE
        ${OpenCV_LIBS}
)

set_target_properties(demo_argus PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}"
        BUILD_RPATH_USE_ORIGIN TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        SKIP_BUILD_RPATH FALSE
)

target_link_options(demo_argus PRIVATE
        "-Wl,--disable-new-dtags"
)

# Speed up build by disabling unused plugins
set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_CSHARP_EXT OFF CACHE BOOL "" FORCE)

set(gRPC_SSL_PROVIDER "package" CACHE STRING "" FORCE)

set(ABSL_PROPAGATE_CXX_STD ON)

include(ExternalProject)

FetchContent_Declare(
        grpc
        GIT_REPOSITORY https://github.com/grpc/grpc.git
        GIT_TAG v1.46.7
)

FetchContent_MakeAvailable(grpc)

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc> PARENT_SCOPE)
set(_GRPC_GRPCPP grpc++ PARENT_SCOPE)
if (CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else ()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin> PARENT_SCOPE)
endif ()

# Ignore warnings from gRPC
target_compile_options(grpc PRIVATE -w)
target_compile_options(grpc++ PRIVATE -w)
target_compile_options(libprotobuf PRIVATE -w)
target_compile_options(libprotobuf-lite PRIVATE -w)
target_compile_options(libprotoc PRIVATE -w)
target_compile_options(grpc++_reflection PRIVATE -w)
target_compile_options(grpc_unsecure PRIVATE -w)
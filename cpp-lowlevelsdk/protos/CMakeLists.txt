project("os-low-level-sdk")

get_filename_component(OSLOWLEVELSDK_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/oslowlevelsdk.proto" ABSOLUTE)
get_filename_component(OSLOWLEVELSDK_PROTO_PATH "${OSLOWLEVELSDK_PROTO}" PATH)

set(OSLOWLEVELSDK_PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/oslowlevelsdk.pb.cc")
set(OSLOWLEVELSDK_PROTO_HEADER "${CMAKE_CURRENT_BINARY_DIR}/oslowlevelsdk.pb.h")
set(OSLOWLEVELSDK_GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/oslowlevelsdk.grpc.pb.cc")
set(OSLOWLEVELSDK_GRPC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/oslowlevelsdk.grpc.pb.h")

add_custom_command(
        OUTPUT "${OSLOWLEVELSDK_PROTO_SRC}" "${OSLOWLEVELSDK_PROTO_HEADER}" "${OSLOWLEVELSDK_GRPC_SRC}" "${OSLOWLEVELSDK_GRPC_HEADER}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${OSLOWLEVELSDK_PROTO_PATH}"
        -I "${grpc_SOURCE_DIR}/third_party/protobuf/src"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${OSLOWLEVELSDK_PROTO}"
        DEPENDS "${OSLOWLEVELSDK_PROTO}"
)

# Let CMake know where the generated headers will be located.
set(LLSDK_PROTO_GEN_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

add_library(llsdk-grpc-gen STATIC
        ${OSLOWLEVELSDK_GRPC_SRC}
        ${OSLOWLEVELSDK_GRPC_HEADER}
        ${OSLOWLEVELSDK_PROTO_SRC}
        ${OSLOWLEVELSDK_PROTO_HEADER}
)

target_include_directories(llsdk-grpc-gen PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(llsdk-grpc-gen
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
)

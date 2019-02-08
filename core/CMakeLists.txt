# This project assume that grpc, protobuf are all installed

cmake_minimum_required(VERSION 3.10)
project(camera_backend C CXX)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
add_definitions( -DBOOST_ALL_NO_LIB )
add_definitions( -DBOOST_ALL_DYN_LINK )
add_definitions( -DBOOST_TEST_DYN_LINK  )
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")
set(DESTINATION_LIB camera_backend)
set(DESTINATION_APP camera_backend)

set(BUILD_PATH ${CMAKE_BINARY_DIR}/build)
set(CMAKE_INSTALL_PREFIX ${BUILD_PATH})

set(PROTO_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/src/grpc_server/protos)
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})
set(PROTO_DIR protos/MicroVision-proto/camera-backend)
file(GLOB PROTO_SOURCES ${PROTO_DIR}/*.proto)

find_program (PROTOC_COMPILER_PLUGIN grpc_cpp_plugin)
add_custom_command(
        OUTPUT ${PROTO_OUTPUT_DIR}/camera_service.pb.cc
	COMMAND protoc --plugin=protoc-gen-grpc=${PROTOC_COMPILER_PLUGIN} -I/usr/local/include -I/usr/include -I${CMAKE_SOURCE_DIR}/${PROTO_DIR} --cpp_out=${PROTO_OUTPUT_DIR} --grpc_out=${PROTO_OUTPUT_DIR} ${PROTO_SOURCES}
        DEPENDS ${PROTO_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Compile proto"
        VERBATIM
)

add_custom_target(proto DEPENDS ${PROTO_OUTPUT_DIR}/camera_service.pb.cc)

include_directories(inc)
add_subdirectory(src)


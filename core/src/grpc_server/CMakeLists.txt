cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE PROTO_SOURCES protos/*.cc)

file(GLOB_RECURSE LIBRARY_SOURCES src/*.c src/*.cpp src/*.cc)
list(FILTER LIBRARY_SOURCES EXCLUDE REGEX "main.cpp")

find_package(Boost COMPONENTS system unit_test_framework thread REQUIRED)

set(THIRD_PARTY_LIBRARIES  ${Boost_SYSTEM_LIBRARY} grpc++_unsecure protobuf -Wl,--no-as-needed grpc++_reflection )
include_directories(src . ../camera_driver/src ../shared/src)


add_library(grpc_server SHARED ${LIBRARY_SOURCES} ${PROTO_SOURCES})
target_link_libraries(grpc_server camera_driver ${THIRD_PARTY_LIBRARIES})
INSTALL(TARGETS grpc_server LIBRARY DESTINATION ${DESTINATION_LIB})
add_dependencies(grpc_server proto)
add_executable(grpc_server_app src/main.cpp)
INSTALL(TARGETS grpc_server_app RUNTIME DESTINATION ${DESTINATION_APP})

target_link_libraries(grpc_server_app grpc_server)

add_executable(test_grpc_server test/test_grpc_server.cpp)
add_dependencies(test_grpc_server grpc_server)

target_link_libraries(
        test_grpc_server
        grpc_server
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        boost_test_exec_monitor
        ${THIRD_PARTY_LIBRARIES})

INSTALL(TARGETS test_grpc_server RUNTIME DESTINATION ${DESTINATION_LIB})

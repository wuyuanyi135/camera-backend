cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE SOURCES src/*.c src/*.cpp src/*.cc)

include_directories(src)
add_library(shared SHARED ${SOURCES})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS log_setup log REQUIRED)
target_link_libraries(shared
        ${Boost_LOG_SETUP_LIBRARY}
        ${Boost_LOG_LIBRARY}
        )

INSTALL(TARGETS shared LIBRARY DESTINATION ${DESTINATION_LIB})
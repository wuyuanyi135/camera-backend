cmake_minimum_required(VERSION 3.10)
file(GLOB SOURCES *.cpp)


# Aravis uses pkg-config
find_package(PkgConfig)

pkg_check_modules(ARAVIS REQUIRED aravis-0.6)

include_directories(${ARAVIS_INCLUDE_DIRS})

add_library(aravis_adapter STATIC ${SOURCES})
set_property(TARGET aravis_adapter PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(aravis_adapter ${ARAVIS_LIBRARIES} shared)

find_package(Boost COMPONENTS system unit_test_framework REQUIRED)



add_executable(test_aravis test/test_aravis_driver.cpp)

target_link_libraries(test_aravis
        aravis_adapter
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        boost_test_exec_monitor
        )
INSTALL(TARGETS test_aravis RUNTIME DESTINATION ${DESTINATION_APP})


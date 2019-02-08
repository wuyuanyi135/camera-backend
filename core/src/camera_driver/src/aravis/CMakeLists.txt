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


file(GLOB TEST_SOURCES test/*.cpp)

foreach(TEST_ENTRY ${TEST_SOURCES})
    get_filename_component(TARGET ${TEST_ENTRY} NAME)

    add_executable(${TARGET} ${TEST_ENTRY})

    target_link_libraries(${TARGET}
            camera_driver
            ${Boost_SYSTEM_LIBRARY}
            ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
            boost_test_exec_monitor
            )
    INSTALL(TARGETS ${TARGET} RUNTIME DESTINATION ${DESTINATION_APP})

endforeach(TEST_ENTRY)
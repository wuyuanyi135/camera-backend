cmake_minimum_required(VERSION 3.10)

include_directories(src ../shared/src)

# implementations
add_subdirectory(src/aravis)


# Boost packages: Boost_XXX
find_package(Boost COMPONENTS system unit_test_framework regex REQUIRED)

set(LINK_LIBRARIES
        shared
        ${Boost_REGEX_LIBRARY}
        pthread
        aravis_adapter
        )

add_library(camera_driver SHARED src/framework.cpp)
INSTALL(TARGETS camera_driver LIBRARY DESTINATION ${DESTINATION_LIB})
target_link_libraries( camera_driver
        ${LINK_LIBRARIES}
        ${THIRD_PARTY_LIBRARIES}
        )


file(GLOB TEST_SOURCES src/test/*.cpp)

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

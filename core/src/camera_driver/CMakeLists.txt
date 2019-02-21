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
target_link_libraries(camera_driver
        ${LINK_LIBRARIES}
        ${THIRD_PARTY_LIBRARIES}
        )

add_executable(test_framework src/test/test_framework.cpp)

target_link_libraries(test_framework
        camera_driver
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        boost_test_exec_monitor
        )
INSTALL(TARGETS test_framework RUNTIME DESTINATION ${DESTINATION_APP})


file(GLOB SOURCES *.c)
add_executable(vimba_backend ${SOURCES})
target_link_libraries(vimba_backend VimbaC)


set(SOURCES
  main.c
  aux.c)
set(HEADERS
  aux.h)

find_package(libudev)
find_package(Threads)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
include_directories(
  ${LIBUDEV_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}
  ${LIBUDEV_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT})

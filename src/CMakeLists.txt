cmake_minimum_required (VERSION 2.8)
project (mxcontrol)

# Set compile parameters
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -pedantic")

set (SOURCES
  main.c
  aux.c)
set (HEADERS
  aux.h)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

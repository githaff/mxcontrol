### MX Master control driver
## This utility allows to make some custom configurations to the MX Master
## mouse, monitor and react to its events (namely, forward/backward/thumb button
## press/release).
## Made by: haff

cmake_minimum_required(VERSION 2.8)
project(mxcontrol)

### Project directories
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(SOURCES_DIR "${CMAKE_SOURCE_DIR}/src")

# Set compile parameters
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -Wextra -pedantic")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -DDEBUG")

add_subdirectory(${SOURCES_DIR})

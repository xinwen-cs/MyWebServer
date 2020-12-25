cmake_minimum_required(VERSION 3.10)

project(webserver)

set(CMAKE_BUILD_TYPE Debug)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DDEBUG)
endif()

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

file(GLOB PROJECT_SOURCES
    ${PROJECT_SOURCE_DIR}/config/*.h
    ${PROJECT_SOURCE_DIR}/config/*.cpp

    ${PROJECT_SOURCE_DIR}/threadpool/*.h

    ${PROJECT_SOURCE_DIR}/locker/*.h

    ${PROJECT_SOURCE_DIR}/webserver/*.h
    ${PROJECT_SOURCE_DIR}/webserver/*.cpp

    ${PROJECT_SOURCE_DIR}/http/*.h
    ${PROJECT_SOURCE_DIR}/http/*.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} pthread)
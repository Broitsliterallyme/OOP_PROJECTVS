cmake_minimum_required(VERSION 3.24)
project(TEST)

find_package(raylib REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} raylib)
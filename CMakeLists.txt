cmake_minimum_required(VERSION 3.30)
project(TimeKeeper)

set(CMAKE_CXX_STANDARD 23)


set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)


add_executable(tike
        ${SRC_DIR}/main.cpp
        ${SRC_DIR}/ArgParser.cpp
        ${SRC_DIR}/Database.cpp)

target_include_directories(tike PRIVATE ${INCLUDE_DIR})

find_package(SQLite3 REQUIRED)

target_link_libraries(tike PRIVATE SQLite::SQLite3)

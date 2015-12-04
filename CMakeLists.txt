cmake_minimum_required(VERSION 3.3)
project(chudoserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(./include)

set(SOURCE_FILES
        src/main.cpp
        src/http_server.cpp
        src/http_request.cpp
)

add_executable(chudoserver ${SOURCE_FILES})

target_link_libraries(chudoserver -levent -lpthread)
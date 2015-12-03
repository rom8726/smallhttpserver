cmake_minimum_required(VERSION 3.3)
project(chudoserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(./include)

set(SOURCE_FILES
        include/http_content_type.h
        include/http_headers.h
        include/static_counter.h
        include/string_constant.h
        include/type_registry.h

        src/main.cpp
        src/http_server.cpp
        src/http_request.cpp
)

add_executable(chudoserver ${SOURCE_FILES})

target_link_libraries(chudoserver -levent -lpthread)
cmake_minimum_required(VERSION 3.3)
project(chudoserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(./include)

set(SOURCE_FILES
        src/main.cpp
        src/http_server.cpp
        src/http_request.cpp
        src/app_config.cpp
)


set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.36.0 COMPONENTS )
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(chudoserver ${SOURCE_FILES})
    target_link_libraries(chudoserver LINK_PUBLIC ${Boost_LIBRARIES} -levent -lpthread)
endif()
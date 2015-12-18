cmake_minimum_required(VERSION 3.3)
project(smallhttpserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

include_directories(./include)

set(SOURCE_FILES
        src/main.cpp
        src/http_server.cpp
        src/http_request.cpp
        src/app_config.cpp
        src/app_services.cpp
        src/cache_service.cpp
        src/logger.cpp
        src/console_logger.cpp
        src/syslog_logger.cpp
        src/null_logger.cpp
        src/demonizer.cpp
        src/tools.cpp
)

# set defines for proj_defs.h
set(DAEMON_NAME smallhttpserver)
set(USE_BOOST_FILEPATH false) # true/false, for getConfigPath() in main.cpp
set(ADD_MY_BOOST_LIBS)
if (USE_BOOST_FILEPATH)
    # list of used boost libs
    set(ADD_MY_BOOST_LIBS filesystem system)
endif ()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.36.0 COMPONENTS ${ADD_MY_BOOST_LIBS})
if (Boost_FOUND)
    # generate proj_defs.h
    configure_file(
            "${PROJECT_SOURCE_DIR}/proj_defs.h.in"
            "${PROJECT_BINARY_DIR}/proj_defs.h"
    )
    include_directories("${PROJECT_BINARY_DIR}")

    # add boost include dirs
    include_directories(${Boost_INCLUDE_DIRS})

    # executable
    add_executable(smallhttpserver ${SOURCE_FILES})

    # libraries
    target_link_libraries(smallhttpserver LINK_PUBLIC ${Boost_LIBRARIES} -levent -lpthread -lmemcached)
else()
    # boost not found
    message(FATAL_ERROR "Boost library not found, CMake will exit.")
endif ()